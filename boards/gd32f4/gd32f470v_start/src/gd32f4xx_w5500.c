/****************************************************************************
 * vendor/gigadevice/boards/gd32f4/gd32f470v_start/src/gd32f4xx_w5500.c
 *
 * FireEye W5500 Ethernet board interface.
 *
 * Wiring (SPI1 shared with the W25Q64 flash, separate chip selects):
 *   SCK  = PB13 (JP5-27)      MISO = PB14 (JP5-30)
 *   MOSI = PB15 (JP5-29)      CS   = PE7  (JP5-15)
 *   RST  = PE9  (JP5-17)      INT  = PE11 (JP5-19)
 *   VCC  = 3.3V               GND  = common ground
 *
 * Notes:
 *   - INT must be wired: the NuttX W5500 driver takes RX from the interrupt
 *     (txavail only polls for TX). INT is active low, falling edge here;
 *   - the chip select is decoded in gd32_spi1select() by devid; this file
 *     configures the CS / RST / INT pins during initialization.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <debug.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/spinlock.h>
#include <nuttx/spi/spi.h>
#include <nuttx/net/w5500.h>
#include <nuttx/net/netdev.h>

#include <arch/board/board.h>

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_exti.h"
#include "gd32f4xx_spi.h"
#include "gd32f470v_start.h"
#include <nuttx/kthread.h>
#include <syslog.h>
#include <nuttx/spi/spi.h>

#ifdef CONFIG_NET_W5500

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define W5500_SPI_PORTNO   1      /* W5500 is on SPI1 */
#define W5500_DEVNO        0      /* single device */
#define W5500_SPI_FREQ     10000000

/* netdev name registered through NET_LL_ETHERNET */
#define W5500_NETDEV_NAME  "eth0"

/* Poll fallback: poll RX every 10ms when INT is absent (1 = enabled) */

#define W5500_POLL_FALLBACK   1
#define W5500_POLL_INTERVAL   10000

/* Diagnostic use: W5500 block select bits (same as the driver) */

#define W5500_DBG_BSB_COMMON_REGS     0x00u
#define W5500_DBG_BSB_SOCKET_REGS(n)  (((n & 0x7u) << 5) | 0x08u)
#define W5500_DBG_RWB_READ            0x00u
#define W5500_DBG_RWB_WRITE           0x04u

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct gd32_w5500_lower_s
{
  const struct w5500_lower_s lower;   /* generic lower half */
  xcpt_t                     handler; /* W5500 interrupt handler */
  FAR void                  *arg;     /* interrupt argument */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint8_t  w5500_dbg_read8(FAR struct spi_dev_s *spi, uint8_t bsb,
                                uint16_t addr);
static void     w5500_dbg_write8(FAR struct spi_dev_s *spi, uint8_t bsb,
                                 uint16_t addr, uint8_t value);
static uint16_t w5500_dbg_read16(FAR struct spi_dev_s *spi, uint8_t bsb,
                                 uint16_t addr);
static int  gd32_w5500_isr(int irq, FAR void *context, FAR void *arg);
static int  gd32_w5500_attach(const struct w5500_lower_s *lower,
                              xcpt_t handler, FAR void *arg);
static void gd32_w5500_enable(const struct w5500_lower_s *lower, bool enable);
static void gd32_w5500_reset(const struct w5500_lower_s *lower, bool reset);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Interrupt counter: verifies that INT really fires */
static volatile uint32_t g_w5500_irq_count;

/* SIR poll statistics (diagnostics) */
static volatile uint32_t g_w5500_sir_hits;
static volatile uint8_t  g_w5500_last_sir;

/* Board default MAC (watchdog; keep in sync with app FIREYEYE_NET_MAC) */
static const uint8_t g_w5500_default_mac[6] =
  { 0x02, 0x00, 0x00, 0x46, 0x49, 0x52 };

static struct gd32_w5500_lower_s g_w5500_lower =
{
  .lower =
  {
    .frequency = W5500_SPI_FREQ,
    .spidevid  = W5500_DEVNO,
    .mode      = SPIDEV_MODE0,
    .attach    = gd32_w5500_attach,
    .enable    = gd32_w5500_enable,
    .reset     = gd32_w5500_reset,
  },
  .handler = NULL,
  .arg     = NULL,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gd32_w5500_isr
 *
 * Description:
 *   Wraps the handler registered by the driver and counts interrupts.
 *   RX depends on this interrupt: a stuck counter means INT is not wired.
 *
 ****************************************************************************/

static volatile bool g_w5500_int_hooked;

#if W5500_POLL_FALLBACK
static int gd32_w5500_poll_task(int argc, FAR char *argv[])
{
  FAR struct spi_dev_s *spi;
  int ticks_mac  = 0;   /* MAC watchdog ticks */
  int ticks_dump = 0;   /* chip register dump ticks */

  syslog(LOG_INFO, "W5500 poll thread started\n");

  spi = gd32_spibus_initialize(W5500_SPI_PORTNO);

  for (; ; )
    {
      usleep(W5500_POLL_INTERVAL);

      /* Notify the driver about RX/TX completion events.
       *
       * This is how RX was validated on hardware (PC ping gets a reply);
       * the driver queues work, so repeated notifications are safe. */

      if (g_w5500_int_hooked && g_w5500_lower.handler != NULL)
        {
          g_w5500_sir_hits++;
          gd32_w5500_isr(0, NULL, g_w5500_lower.arg);
        }

      /* MAC watchdog: every 10s, restore SHAR if the chip lost it. */

      if (spi != NULL && ++ticks_mac  >= (10000000 / W5500_POLL_INTERVAL))
        {
          uint8_t m0;
          uint8_t m1;
          uint8_t m2;

          ticks_mac  = 0;
          m0 = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x0009);
          m1 = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000a);
          m2 = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000b);

          if (m0 == 0 && m1 == 0 && m2 == 0)
            {
              syslog(LOG_WARNING, "W5500 watchdog: MAC lost, restoring\n");
              gd32_fireeye_w5500_fixup(g_w5500_default_mac);
            }
        }

      /* Every 10s read chip registers directly to check RX activity */

      if (spi != NULL && ++ticks_dump >= (10000000 / W5500_POLL_INTERVAL))
        {
          uint8_t verr;
          uint8_t phycfgr;
          uint8_t snmr;
          uint8_t snir;
          uint8_t snsr;
          uint8_t rxbuf;
          uint8_t txbuf;
          uint16_t rxrsr;
          uint8_t mac0;
          uint8_t mac1;
          uint8_t mac2;
          uint8_t mac3;
          uint8_t mac4;
          uint8_t mac5;

          ticks_dump = 0;
          verr    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x0039);
          phycfgr = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x002e);
          snmr    = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0000);
          snir    = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0002);
          snsr    = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0003);
          rxbuf   = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x001e);
          txbuf   = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x001f);
          rxrsr   = w5500_dbg_read16(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0026);
          mac0    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x0009);
          mac1    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000a);
          mac2    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000b);
          mac3    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000c);
          mac4    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000d);
          mac5    = w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000e);

          syslog(LOG_INFO,
                 "W5500 chip: VERR=%02x PHY=%02x MR=%02x SR=%02x IR=%02x "
                 "RXBUF=%u TXBUF=%u RSR=%u MAC=%02x:%02x:%02x:%02x:%02x:%02x "
                 "sir=%02x hits=%lu int_pin=%d\n",
                 verr, phycfgr, snmr, snsr, snir,
                 (unsigned)rxbuf, (unsigned)txbuf, (unsigned)rxrsr,
                 mac0, mac1, mac2, mac3, mac4, mac5,
                 g_w5500_last_sir, (unsigned long)g_w5500_sir_hits,
                 (int)gd32_gpio_read(GPIO_W5500_INTR));
        }
    }

  return 0;
}
#endif

/* ---- Diagnostics: read W5500 registers with the driver SPI frame ---- */

static uint8_t w5500_dbg_read8(FAR struct spi_dev_s *spi, uint8_t bsb,
                               uint16_t addr)
{
  uint8_t cntl[3];
  uint8_t value = 0;

  cntl[0] = (uint8_t)(addr >> 8);
  cntl[1] = (uint8_t)(addr & 0xff);
  cntl[2] = bsb | W5500_DBG_RWB_READ;

  SPI_LOCK(spi, true);
  SPI_SELECT(spi, SPIDEV_ETHERNET(0), true);
  SPI_SETMODE(spi, SPIDEV_MODE0);
  SPI_SETBITS(spi, 8);
  SPI_SETFREQUENCY(spi, W5500_SPI_FREQ);
  SPI_SNDBLOCK(spi, cntl, sizeof(cntl));
  SPI_RECVBLOCK(spi, &value, 1);
  SPI_SELECT(spi, SPIDEV_ETHERNET(0), false);
  SPI_LOCK(spi, false);

  return value;
}

static void w5500_dbg_write8(FAR struct spi_dev_s *spi, uint8_t bsb,
                             uint16_t addr, uint8_t value)
{
  uint8_t cntl[3];

  cntl[0] = (uint8_t)(addr >> 8);
  cntl[1] = (uint8_t)(addr & 0xff);
  cntl[2] = bsb | W5500_DBG_RWB_WRITE;

  SPI_LOCK(spi, true);
  SPI_SELECT(spi, SPIDEV_ETHERNET(0), true);
  SPI_SETMODE(spi, SPIDEV_MODE0);
  SPI_SETBITS(spi, 8);
  SPI_SETFREQUENCY(spi, W5500_SPI_FREQ);
  SPI_SNDBLOCK(spi, cntl, sizeof(cntl));
  SPI_SNDBLOCK(spi, &value, 1);
  SPI_SELECT(spi, SPIDEV_ETHERNET(0), false);
  SPI_LOCK(spi, false);
}

static uint16_t w5500_dbg_read16(FAR struct spi_dev_s *spi, uint8_t bsb,
                                 uint16_t addr)
{
  uint8_t hi = w5500_dbg_read8(spi, bsb, addr);
  uint8_t lo = w5500_dbg_read8(spi, bsb, addr + 1);

  return (uint16_t)((hi << 8) | lo);
}

static int gd32_w5500_isr(int irq, FAR void *context, FAR void *arg)
{
  uint32_t count = ++g_w5500_irq_count;

  if (count <= 5 || (count % 500) == 0)
    {
      _info("W5500 IRQ count = %lu\n", (unsigned long)count);
    }

  return g_w5500_lower.handler(irq, context, arg);
}

static int gd32_w5500_attach(const struct w5500_lower_s *lower,
                             xcpt_t handler, FAR void *arg)
{
  FAR struct gd32_w5500_lower_s *priv = (FAR struct gd32_w5500_lower_s *)lower;

  /* Only record the handler; the EXTI bind happens in enable() */

  priv->handler = handler;
  priv->arg     = arg;
  return OK;
}

static void gd32_w5500_enable(const struct w5500_lower_s *lower, bool enable)
{
  FAR struct gd32_w5500_lower_s *priv = (FAR struct gd32_w5500_lower_s *)lower;
  irqstate_t flags;
  uint8_t gpio_irq;
  uint8_t gpio_irqnum;
  int ret;

  DEBUGASSERT(priv->handler != NULL);

  flags = enter_critical_section();

  if (enable)
    {
      ret = gd32_gpio_exti_irqnum_get(GPIO_W5500_INTR, &gpio_irqnum);
      if (ret < 0)
        {
          leave_critical_section(flags);
          return;
        }

      /* W5500 INT is active low, so use the falling edge */

      ret = gd32_exti_gpioirq_init(GPIO_W5500_INTR, EXTI_INTERRUPT,
                                   EXTI_TRIG_FALLING, &gpio_irq);
      if (ret < 0)
        {
          leave_critical_section(flags);
          return;
        }

      gd32_exti_gpio_irq_attach(gpio_irq, gd32_w5500_isr, priv->arg);
      up_enable_irq(gpio_irqnum);
      g_w5500_int_hooked = true;
    }
  else
    {
      ret = gd32_gpio_exti_irqnum_get(GPIO_W5500_INTR, &gpio_irqnum);
      if (ret >= 0)
        {
          up_disable_irq(gpio_irqnum);
          irq_detach(gpio_irqnum);
        }
    }

  leave_critical_section(flags);
}

static void gd32_w5500_reset(const struct w5500_lower_s *lower, bool reset)
{
  /* W5500 reset pin is active low */

  gd32_gpio_write(GPIO_W5500_RESET, !reset);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gd32_fireeye_w5500_initialize
 *
 * Description:
 *   Initialize the W5500: configure CS/RST/INT pins, bind the NuttX driver.
 *   Called from the board bringup when CONFIG_NET_W5500 is enabled.
 *
 * Returned Value:
 *   0 on success; a negated errno on failure.
 *
 ****************************************************************************/

int gd32_fireeye_w5500_initialize(void)
{
  FAR struct spi_dev_s *spi;
  int ret;

  /* 1) Pins: CS (high = deselected), RST (high = run), INT (input) */

  gd32_gpio_config(GPIO_SPI1_ETH_CSPIN);
  gd32_gpio_write(GPIO_SPI1_ETH_CSPIN, true);

  gd32_gpio_config(GPIO_W5500_RESET);
  gd32_gpio_write(GPIO_W5500_RESET, true);

  gd32_gpio_config(GPIO_W5500_INTR);

  /* 2) Reset pulse: hold low for 10ms, then release */

  gd32_gpio_write(GPIO_W5500_RESET, false);
  usleep(10000);
  gd32_gpio_write(GPIO_W5500_RESET, true);
  usleep(50000);

  /* 3) Get the SPI1 bus and bind the driver */

  spi = gd32_spibus_initialize(W5500_SPI_PORTNO);
  if (spi == NULL)
    {
      _err("ERROR: Failed to initialize SPI port %d\n", W5500_SPI_PORTNO);
      return -ENODEV;
    }

  ret = w5500_initialize(spi, &g_w5500_lower.lower, W5500_DEVNO);
  if (ret < 0)
    {
      _err("ERROR: Failed to bind SPI%d to W5500: %d\n", W5500_SPI_PORTNO, ret);
      return ret;
    }

  _info("W5500 bound to SPI%d\n", W5500_SPI_PORTNO);

  /* Store the MAC in the netdev right after registration, before ifup.
   *
   * The NuttX W5500 driver has no default MAC: w5500_unfence() (run on
   * every ifup) copies netdev->d_mac into SHAR, and d_mac starts as all
   * zero -- only SIOCSIFHWADDR (netlib_setmacaddr) fills it in. On this
   * board netinit (CONFIG_NSH_NETINIT) calls ifup first and only sets a
   * MAC when CONFIG_NETINIT_NOMAC is enabled.
   *
   * The result is SHAR == 0 with the Sn_MR MAC filter (MFEN=1) enabled,
   * so the chip drops every frame: link up, RX_RSR always 0, no data.
   *
   * Writing the MAC here means any later ifup (netinit, the app or a
   * manual ifup eth0) programs a valid MAC; the app fixup stays as backstop.
   */

  {
    FAR struct net_driver_s *dev = netdev_findbyname(W5500_NETDEV_NAME);

    if (dev != NULL)
      {
        memcpy(dev->d_mac.ether.ether_addr_octet, g_w5500_default_mac,
               sizeof(g_w5500_default_mac));

        syslog(LOG_INFO, "W5500: %s MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
              W5500_NETDEV_NAME,
              g_w5500_default_mac[0], g_w5500_default_mac[1],
              g_w5500_default_mac[2], g_w5500_default_mac[3],
              g_w5500_default_mac[4], g_w5500_default_mac[5]);
      }
    else
      {
        syslog(LOG_ERR, "ERROR: W5500: netdev %s not found\n", W5500_NETDEV_NAME);
      }
  }

#if W5500_POLL_FALLBACK
  /* Start the poll fallback thread: RX works even without INT */

  ret = kthread_create("w5500_poll", 120, 1536, gd32_w5500_poll_task, NULL);
  if (ret < 0)
    {
      _err("ERROR: failed to start W5500 poll thread: %d\n", ret);
    }
  else
    {
      _info("W5500 polling fallback enabled (%d us)\n", W5500_POLL_INTERVAL);
    }
#endif

  return OK;
}

/****************************************************************************
 * Name: arm_netinitialize
 *
 * Description:
 *   Network init hook called by the kernel during up_initialize().
 *   The board binds the driver here; the app sets the static IP later.
 *
 ****************************************************************************/

void arm_netinitialize(void)
{
  /* This runs during up_initialize(): the scheduler and system tick are
   * not running yet, so no usleep() or interrupt/work-queue init here.
   * The real W5500 init lives in the board bringup (gd32_bringup),
   * see gd32_fireeye_w5500_initialize(). */
}

/****************************************************************************
 * Name: gd32_fireeye_w5500_fixup
 *
 * Description:
 *   Measured: after driver init SHAR (MAC) is still all zero while the Sn_MR
 *   MAC filter (MFEN) is on -- the hardware drops every received frame,
 *   i.e. link up but no packets (RX_RSR always 0, no ARP reply).
 *   This helper rewrites the MAC after ifup and disables MAC filtering.
 *
 * Input Parameters:
 *   mac - 6-byte MAC address
 *
 * Returned Value:
 *   0 on success; a negated errno on failure
 *
 ****************************************************************************/

int gd32_fireeye_w5500_fixup(FAR const uint8_t *mac)
{
  FAR struct spi_dev_s *spi;
  uint8_t mr;
  int i;

  if (mac == NULL)
    {
      return -EINVAL;
    }

  spi = gd32_spibus_initialize(W5500_SPI_PORTNO);
  if (spi == NULL)
    {
      return -ENODEV;
    }

  /* 1) Write SHAR0..5 (the chip source MAC) */

  for (i = 0; i < 6; i++)
    {
      w5500_dbg_write8(spi, W5500_DBG_BSB_COMMON_REGS,
                       (uint16_t)(0x0009 + i), mac[i]);
    }

  /* 2) Disable MAC filtering (Sn_MR bit7) to accept all frames */

  mr = w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0000);
  mr = (uint8_t)(mr & ~0x80);
  w5500_dbg_write8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0000, mr);

  /* 3) Read back for confirmation */

  syslog(LOG_INFO,
         "W5500 fixup: MR=%02x MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
         w5500_dbg_read8(spi, W5500_DBG_BSB_SOCKET_REGS(0), 0x0000),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x0009),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000a),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000b),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000c),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000d),
         w5500_dbg_read8(spi, W5500_DBG_BSB_COMMON_REGS, 0x000e));

  /* Mark the link up (IFF_RUNNING): the NuttX driver never does it, and
   * without it route lookup fails and outbound connect() returns -101. */

  {
    FAR struct net_driver_s *dev = netdev_findbyname("eth0");

    if (dev != NULL)
      {
        netdev_carrier_on(dev);
        syslog(LOG_INFO, "W5500 fixup: carrier on (flags=0x%04x)\n",
               (unsigned)dev->d_flags);
      }
    else
      {
        syslog(LOG_ERR, "W5500 fixup: eth0 not found\n");
      }
  }

  return OK;
}

#endif /* CONFIG_NET_W5500 */
