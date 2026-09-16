/****************************************************************************
 * vendor/gigadevice/boards/gd32f4/gd32f470v_start/src/gd32f4xx_selftest.c
 *
 * FireEye board self-test: prove liveness with the LED, not the console.
 *
 * When the console stays silent, two cases must be told apart:
 *   (a) the chip never started (PLL not locked, boot failure);
 *   (b) the chip runs but the console path (pins/wiring/USB-TTL) is bad.
 *
 * Board references (schematic GD32407V-START-V1.1):
 *   - LED2: PC6 - R10(470R) - LED2 - GND, active high;
 *   - key K2: PA0, 10k pull-up to 3V3, low when pressed.
 *
 * LED patterns, in order after reset:
 *   1) 2 fast blinks: clocks configured, early board init reached;
 *   2) 5 fast blinks: application init reached (heartbeat thread up);
 *   3) 10 very fast blinks: UART loopback OK (PB6/PB7 shorted);
 *   4) then a 1Hz heartbeat (0.5s on / 0.5s off): fully up;
 *      time it: a clearly slower blink means the crystal does not match;
 *   5) hold K2: the LED blinks at 5Hz, proving GPIO input works.
 *
 * Console helper (disabled by default):
 *   with FIREYE_SELFTEST_UART=1 the code drives USART0 registers directly:
 *   it sends "FIREYE
" each heartbeat and runs a loopback test (RX IRQ
 *   disabled, polled RX) to prove the UART path when PB6/PB7 are shorted).
 *   It competes with the console driver: use only while debugging, and
 *   keep it 0 once the console works (otherwise the NSH banner repeats).
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include <nuttx/board.h>
#include <nuttx/kthread.h>

#include "gd32f4xx_gpio.h"
#include "hardware/gd32f4xx_uart.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* On-board LED: PC6, active high */

#define FIREYE_LED_PIN  (GPIO_CFG_MODE_OUTPUT | GPIO_CFG_PUPD_NONE | GPIO_CFG_PP | \
                         GPIO_CFG_SPEED_50MHZ | GPIO_CFG_PORT_C | GPIO_CFG_PIN_6)

/* On-board key: PA0, low when pressed */

#define FIREYE_KEY_PIN  (GPIO_CFG_MODE_INPUT | GPIO_CFG_PUPD_PULLUP | \
                         GPIO_CFG_PORT_A | GPIO_CFG_PIN_0)

#define SELFTEST_THREAD_PRIO   100
#define SELFTEST_THREAD_STACK  2048

#define SELFTEST_LINE          "FIREYE\r\n"
#define SELFTEST_LINE_LEN      8

/* 1 = enable the direct USART0 register diagnostics (fights the console);
 * the console works, so keep this at 0. */

#define FIREYE_SELFTEST_UART   0

/****************************************************************************
 * Private Data
 ****************************************************************************/

#if FIREYE_SELFTEST_UART
static bool g_loopback_ok;
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void fireye_led(bool on)
{
  gd32_gpio_write(FIREYE_LED_PIN, on);
}

static void fireye_led_flash(int times, useconds_t on_us, useconds_t off_us)
{
  int i;

  for (i = 0; i < times; i++)
    {
      fireye_led(true);
      usleep(on_us);
      fireye_led(false);
      usleep(off_us);
    }
}

/* Early busy wait: no scheduler or system tick yet, so no usleep() */

static void fireye_busy_wait(volatile uint32_t loops)
{
  while (loops-- > 0)
    {
    }
}

#if FIREYE_SELFTEST_UART

/* Register access done locally (arm_internal.h is awkward in board code) */

static uint32_t fireye_reg_read(uintptr_t addr)
{
  return *(volatile uint32_t *)addr;
}

static void fireye_reg_write(uintptr_t addr, uint32_t value)
{
  *(volatile uint32_t *)addr = value;
}

/* Send one byte by driving USART0 registers (bypasses the NuttX stack) */

static void fireye_raw_putc(char ch)
{
  uintptr_t base = GD32_USART0_BASE;
  uint32_t ctl0;

  /* Set only TEN: keep the baud rate and frame format set by the console */

  ctl0 = fireye_reg_read(base + GD32_USART_CTL0_OFFSET);
  if ((ctl0 & USART_CTL0_TEN) == 0)
    {
      fireye_reg_write(base + GD32_USART_CTL0_OFFSET, ctl0 | USART_CTL0_TEN);
    }

  while ((fireye_reg_read(base + GD32_USART_STAT0_OFFSET) & USART_STAT0_TBE) == 0)
    {
    }

  fireye_reg_write(base + GD32_USART_DATA_OFFSET, (uint32_t)(uint8_t)ch);
}

static void fireye_raw_puts(const char *str)
{
  while (*str != '\0')
    {
      fireye_raw_putc(*str++);
    }
}

/* UART loopback: with PB6/PB7 shorted we must receive what we sent */

static void fireye_loopback_test(void)
{
  uintptr_t base = GD32_USART0_BASE;
  uint32_t ctl0 = fireye_reg_read(base + GD32_USART_CTL0_OFFSET);
  int received = 0;
  int round;
  int guard;

  /* Disable RX interrupts and poll here so the driver cannot steal data */

  fireye_reg_write(base + GD32_USART_CTL0_OFFSET,
                   (ctl0 | USART_CTL0_REN) & ~USART_CTL0_RBNEIE);

  for (round = 0; round < 4; round++)
    {
      fireye_raw_puts(SELFTEST_LINE);

      for (guard = 0; guard < 100; guard++)
        {
          if ((fireye_reg_read(base + GD32_USART_STAT0_OFFSET) &
               USART_STAT0_RBNE) != 0)
            {
              (void)fireye_reg_read(base + GD32_USART_DATA_OFFSET);
              received++;
            }

          usleep(1000);
        }
    }

  /* Restore the original control register value */

  fireye_reg_write(base + GD32_USART_CTL0_OFFSET, ctl0);

  g_loopback_ok = (received >= SELFTEST_LINE_LEN);
}

#endif /* FIREYE_SELFTEST_UART */

/* Heartbeat thread: LED patterns + key handling + raw UART output */

static int fireye_selftest_thread(int argc, char *argv[])
{
  gd32_gpio_config(FIREYE_KEY_PIN);
  gd32_gpio_config(FIREYE_LED_PIN);

  /* 5 fast blinks: application init reached */

  fireye_led_flash(5, 100000, 100000);

#if FIREYE_SELFTEST_UART
  /* UART loopback (~0.4s); on success blink 10 more times quickly */

  fireye_loopback_test();
  if (g_loopback_ok)
    {
      fireye_led_flash(10, 30000, 30000);
    }
#endif

  for (; ; )
    {
      if (!gd32_gpio_read(FIREYE_KEY_PIN))
        {
          /* Key pressed: 5Hz blink */

          fireye_led(true);
          usleep(100000);
          fireye_led(false);
          usleep(100000);
        }
      else
        {
          /* Normal heartbeat: 1Hz */

          fireye_led(true);
          usleep(500000);
          fireye_led(false);
          usleep(500000);
        }
    }

  return 0;  /* not reached */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gd32_selftest_early
 *
 * Description:
 *   Called from gd32_boardinitialize: two blinks prove that the clocks
 *   are configured and the firmware did not stall during boot.
 *
 ****************************************************************************/

void gd32_selftest_early(void)
{
  int i;

  gd32_gpio_config(FIREYE_LED_PIN);

  for (i = 0; i < 2; i++)
    {
      fireye_led(true);
      fireye_busy_wait(4000000);
      fireye_led(false);
      fireye_busy_wait(4000000);
    }
}

/****************************************************************************
 * Name: gd32_selftest_start
 *
 * Description:
 *   Start the heartbeat thread (called at the end of gd32_bringup).
 *
 * Returned Value:
 *   Returns a non-negative thread PID, or a negated errno on failure.
 *
 ****************************************************************************/

int gd32_selftest_start(void)
{
  return kthread_create("fireye_led", SELFTEST_THREAD_PRIO,
                        SELFTEST_THREAD_STACK, fireye_selftest_thread, NULL);
}
