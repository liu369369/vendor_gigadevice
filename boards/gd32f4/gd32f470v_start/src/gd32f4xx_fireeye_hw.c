/****************************************************************************
 * vendor/gigadevice/boards/gd32f4/gd32f470v_start/src/gd32f4xx_fireeye_hw.c
 *
 * FireEye board hardware: ADC sampling + alarm outputs + button + I2C1.
 *
 * Why this lives in the board directory:
 *   openvela/nuttx has no GD32F4 ADC driver and no ADC register header yet.
 *   Registers are driven directly per GD32F4xx User Manual Rev3.4 ch. 14;
 *   the app only sees simple sample()/output() style interfaces.
 *
 * Register references (UM Rev3.4):
 *   14.7.1  ADC_STAT   offset 0x000 : bit5 ROVF / bit4 STRC / bit1 EOC / bit0 WDE
 *   14.7.2  ADC_CTL0   offset 0x004 : bit25:24 DRES(00=12bit) / bit8 SM / bit5 EOCIE
 *   14.7.3  ADC_CTL1   offset 0x008 : bit30 SWRCST / bit11 DAL / bit10 EOCM /
 *                                     bit8 DMA / bit3 RSTCLB / bit2 CLB / bit1 CTN /
 *                                     bit0 ADCON
 *   14.7.4  ADC_SAMPT0 offset 0x00C : channels 10..18 sample time (3 bits)
 *   14.7.5  ADC_SAMPT1 offset 0x010 : channels 0..9 sample time (3 bits)
 *                                     000=3 001=15 010=28 011=56 100=84 101=112
 *                                     110=144 111=480 CK_ADC cycles
 *   14.7.8  ADC_RSQ0   offset 0x02C : bit23:20 RL[3:0], count = RL + 1
 *   14.7.10 ADC_RSQ2   offset 0x034 : bit4:0 RSQ0[4:0], first channel
 *   14.7.11 ADC_RDATA  offset 0x04C : bit15:0 conversion result
 *   14.7.14 ADC_SYNCCTL offset 0x304: bit18:16 ADCCK[2:0]，000=PCLK2/2 … 011=PCLK2/8
 *   4.3.14  RCU_APB2EN offset 0x044 : bit8 ADC0EN
 *   14.4.1  Calibration: ADCON=1, delay, RSTCLB then CLB self-clear
 *
 * Wiring (see the board wiring document):
 *   PA4 = ADC0_IN4  current (ACS712 output divided by 2)
 *   PA6 = ADC0_IN6  temperature (10k pull-up + NTC to ground)
 *   PB0 = relay IN
 *   PB1 = buzzer +
 *   PA0 = on-board key K2 (low when pressed)
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/i2c/i2c_master.h>

#include <arch/board/board.h>

#include "gd32f4xx_gpio.h"
#include "gd32f4xx_i2c.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* ADC0 and related registers (UM 14.7) */

#define FIREEYE_ADC0_BASE        0x40012000u
#define FIREEYE_ADC_STAT         (FIREEYE_ADC0_BASE + 0x000)
#define FIREEYE_ADC_CTL0         (FIREEYE_ADC0_BASE + 0x004)
#define FIREEYE_ADC_CTL1         (FIREEYE_ADC0_BASE + 0x008)
#define FIREEYE_ADC_SAMPT0       (FIREEYE_ADC0_BASE + 0x00c)
#define FIREEYE_ADC_SAMPT1       (FIREEYE_ADC0_BASE + 0x010)
#define FIREEYE_ADC_RSQ0         (FIREEYE_ADC0_BASE + 0x02c)
#define FIREEYE_ADC_RSQ2         (FIREEYE_ADC0_BASE + 0x034)
#define FIREEYE_ADC_RDATA        (FIREEYE_ADC0_BASE + 0x04c)
#define FIREEYE_ADC_SYNCCTL      (FIREEYE_ADC0_BASE + 0x304)

/* RCU: ADC0 clock enable (UM 4.3.14, offset 0x44, bit8) */

#define FIREEYE_RCU_APB2EN       0x40023844u
#define FIREEYE_RCU_ADC0EN       (1u << 8)

/* ADC_STAT bits */

#define FIREEYE_ADC_STAT_EOC     (1u << 1)

/* ADC_CTL0 bits */

#define FIREEYE_ADC_CTL0_DRES12  (0u << 24)   /* 12-bit resolution */

/* ADC_CTL1 bits */

#define FIREEYE_ADC_CTL1_ADCON   (1u << 0)
#define FIREEYE_ADC_CTL1_CLB     (1u << 2)
#define FIREEYE_ADC_CTL1_RSTCLB  (1u << 3)
#define FIREEYE_ADC_CTL1_SWRCST  (1u << 30)

/* ADC_SYNCCTL：ADCCK[2:0]，001 = PCLK2 / 4 */

#define FIREEYE_ADC_ADCCK_SHIFT  16
#define FIREEYE_ADC_ADCCK_DIV4   (1u << FIREEYE_ADC_ADCCK_SHIFT)

/* Sample time code: 111 = 480 CK_ADC cycles (longest, 5k divider) */

#define FIREEYE_ADC_SPT_480      7u

/* Poll timeout: fail loudly instead of continuing silently */

#define FIREEYE_ADC_TIMEOUT      200000

/* Pins: PA4/PA6 analog in, PB0 relay, PB1 buzzer, PA0 key */

#define GPIO_FIREEYE_CURRENT     (GPIO_CFG_MODE_ANALOG | GPIO_CFG_PUPD_NONE | \
                                  GPIO_CFG_PORT_A | GPIO_CFG_PIN_4)
#define GPIO_FIREEYE_TEMP        (GPIO_CFG_MODE_ANALOG | GPIO_CFG_PUPD_NONE | \
                                  GPIO_CFG_PORT_A | GPIO_CFG_PIN_6)
#define GPIO_FIREEYE_RELAY       (GPIO_CFG_MODE_OUTPUT | GPIO_CFG_PUPD_NONE | \
                                  GPIO_CFG_PP | GPIO_CFG_SPEED_2MHZ | \
                                  GPIO_CFG_PORT_B | GPIO_CFG_PIN_0)
#define GPIO_FIREEYE_BUZZER      (GPIO_CFG_MODE_OUTPUT | GPIO_CFG_PUPD_NONE | \
                                  GPIO_CFG_PP | GPIO_CFG_SPEED_2MHZ | \
                                  GPIO_CFG_PORT_B | GPIO_CFG_PIN_1)
#define GPIO_FIREEYE_KEY         (GPIO_CFG_MODE_INPUT | GPIO_CFG_PUPD_PULLUP | \
                                  GPIO_CFG_PORT_A | GPIO_CFG_PIN_0)

/* Alarm lamp module (3 pins: VCC 5V / GND / IN) signal pin */

#define GPIO_FIREEYE_ALARM_LED   (GPIO_CFG_MODE_OUTPUT | GPIO_CFG_PUPD_NONE | \
                                  GPIO_CFG_PP | GPIO_CFG_SPEED_2MHZ | \
                                  GPIO_CFG_PORT_D | GPIO_CFG_PIN_9)

/* Alarm lamp polarity: 1 = active high; set 0 if measured inverted */

#define FIREEYE_LED_ACTIVE_HIGH  1

/* Relay module (JQC-3FF-S-Z, 5V coil) is usually active low:
 * low output -> coil energised -> NO closes / NC opens.
 * Set the macro below to 0 if the measured behaviour is opposite. */

/* Measured 2026-09-13: fan stops at NORMAL, so low-level trigger = 1 */
#define FIREEYE_RELAY_ACTIVE_LOW  1

/* Buzzer is high-active on this active buzzer module */

/* Measured 2026-09-13: the buzzer module is low-active, so it is set to 0 */
#define FIREEYE_BUZZER_ACTIVE_HIGH  0

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* arm_internal.h is awkward here: do the register access locally */

static uint32_t fireeye_reg_read(uintptr_t addr)
{
  return *(volatile uint32_t *)addr;
}

static void fireeye_reg_write(uintptr_t addr, uint32_t value)
{
  *(volatile uint32_t *)addr = value;
}

static void fireeye_reg_setbits(uintptr_t addr, uint32_t bits)
{
  fireeye_reg_write(addr, fireeye_reg_read(addr) | bits);
}

static void fireeye_reg_clrset(uintptr_t addr, uint32_t clr, uint32_t set)
{
  fireeye_reg_write(addr, (fireeye_reg_read(addr) & ~clr) | set);
}

/* Wait until hardware clears a bit, with timeout (fail fast) */

static int fireeye_wait_clear(uintptr_t addr, uint32_t bits)
{
  int i;

  for (i = 0; i < FIREEYE_ADC_TIMEOUT; i++)
    {
      if ((fireeye_reg_read(addr) & bits) == 0)
        {
          return 0;
        }
    }

  return -ETIMEDOUT;
}

static void fireeye_led(bool on)
{
  /* On-board LED2: PC6, active high (schematic GD32407V-START-V1.1) */

  gd32_gpio_write(GPIO_CFG_MODE_OUTPUT | GPIO_CFG_PUPD_NONE | GPIO_CFG_PP |
                  GPIO_CFG_SPEED_2MHZ | GPIO_CFG_PORT_C | GPIO_CFG_PIN_6, on);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: gd32_fireeye_hw_initialize
 *
 * Description:
 *   Initialize FireEye board hardware: ADC0 (current/temperature), alarm
 *   outputs (buzzer/relay), key input and the on-board LED.
 *
 * Returned Value:
 *   0 on success, a negated errno on failure.
 *
 ****************************************************************************/

int gd32_fireeye_hw_initialize(void)
{
  int ret;
  uint32_t ctl1;

  /* --- 1) ADC0 clock and prescaler --- */

  fireeye_reg_setbits(FIREEYE_RCU_APB2EN, FIREEYE_RCU_ADC0EN);

  /* PCLK2 = 84MHz, ADCCK = PCLK2/4 = 21MHz (datasheet limit 40MHz) */

  fireeye_reg_clrset(FIREEYE_ADC_SYNCCTL, (7u << FIREEYE_ADC_ADCCK_SHIFT),
                     FIREEYE_ADC_ADCCK_DIV4);

  /* --- 2) Pins: PA4/PA6 analog in, PB0/PB1 output, PA0 input --- */

  gd32_gpio_config(GPIO_FIREEYE_CURRENT);
  gd32_gpio_config(GPIO_FIREEYE_TEMP);
  gd32_gpio_config(GPIO_FIREEYE_RELAY);
  gd32_gpio_config(GPIO_FIREEYE_BUZZER);
  gd32_gpio_config(GPIO_FIREEYE_KEY);
  gd32_gpio_config(GPIO_FIREEYE_ALARM_LED);

  /* Default outputs: relay off, buzzer silent */

  gd32_fireeye_set_relay(false);
  gd32_fireeye_set_buzzer(false);
  gd32_fireeye_set_alarm_led(false);

  /* --- 3) ADC basics: 12-bit, no scan, single conversion --- */

  fireeye_reg_write(FIREEYE_ADC_CTL0, FIREEYE_ADC_CTL0_DRES12);
  fireeye_reg_write(FIREEYE_ADC_CTL1, 0);

  /* --- 4) ADC power on --- */

  fireeye_reg_setbits(FIREEYE_ADC_CTL1, FIREEYE_ADC_CTL1_ADCON);
  usleep(2000);

  /* --- 5) Foreground calibration: RSTCLB then CLB (UM 14.4.1) --- */

  ctl1 = fireeye_reg_read(FIREEYE_ADC_CTL1);
  fireeye_reg_write(FIREEYE_ADC_CTL1, ctl1 | FIREEYE_ADC_CTL1_RSTCLB);
  ret = fireeye_wait_clear(FIREEYE_ADC_CTL1, FIREEYE_ADC_CTL1_RSTCLB);
  if (ret < 0)
    {
      return ret;
    }

  fireeye_reg_setbits(FIREEYE_ADC_CTL1, FIREEYE_ADC_CTL1_CLB);
  ret = fireeye_wait_clear(FIREEYE_ADC_CTL1, FIREEYE_ADC_CTL1_CLB);
  if (ret < 0)
    {
      return ret;
    }

  fireeye_led(false);
  return 0;
}

/****************************************************************************
 * Name: gd32_fireeye_adc_sample
 *
 * Description:
 *   Single-channel single conversion, returns the raw 12-bit code.
 *
 * Input Parameters:
 *   channel - ADC channel (0..18). Current = 4 (PA4), temp = 6 (PA6).
 *
 * Returned Value:
 *   Non-negative: conversion result; negative: error code.
 *
 ****************************************************************************/

int gd32_fireeye_adc_sample(int channel)
{
  uintptr_t sampt;
  uint32_t shift;
  int i;

  if (channel < 0 || channel > 18)
    {
      return -EINVAL;
    }

  /* Sample time: 480 cycles */

  if (channel < 10)
    {
      sampt = FIREEYE_ADC_SAMPT1;
      shift = (uint32_t)(3 * channel);
    }
  else
    {
      sampt = FIREEYE_ADC_SAMPT0;
      shift = (uint32_t)(3 * (channel - 10));
    }

  fireeye_reg_clrset(sampt, (7u << shift), (FIREEYE_ADC_SPT_480 << shift));

  /* Sequence length = 1 (RL=0), first channel = channel */

  fireeye_reg_clrset(FIREEYE_ADC_RSQ0, (0xfu << 20), 0);
  fireeye_reg_clrset(FIREEYE_ADC_RSQ2, 0x1fu, (uint32_t)channel);

  /* Clear flags then software trigger (EOC/STRC are rc_w0) */

  fireeye_reg_write(FIREEYE_ADC_STAT, 0);
  fireeye_reg_setbits(FIREEYE_ADC_CTL1, FIREEYE_ADC_CTL1_SWRCST);

  for (i = 0; i < FIREEYE_ADC_TIMEOUT; i++)
    {
      if ((fireeye_reg_read(FIREEYE_ADC_STAT) & FIREEYE_ADC_STAT_EOC) != 0)
        {
          /* Reading RDATA also clears EOC */

          return (int)(fireeye_reg_read(FIREEYE_ADC_RDATA) & 0xffffu);
        }
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: gd32_fireeye_set_buzzer / gd32_fireeye_set_relay
 *
 * Description:
 *   Control the buzzer and relay outputs (polarity handled here).
 *
 ****************************************************************************/

void gd32_fireeye_set_buzzer(bool on)
{
#if FIREEYE_BUZZER_ACTIVE_HIGH
  gd32_gpio_write(GPIO_FIREEYE_BUZZER, on);
#else
  gd32_gpio_write(GPIO_FIREEYE_BUZZER, !on);
#endif
}

void gd32_fireeye_set_relay(bool energized)
{
  /* energized = true closes the relay (cuts the load) */

#if FIREEYE_RELAY_ACTIVE_LOW
  gd32_gpio_write(GPIO_FIREEYE_RELAY, !energized);
#else
  gd32_gpio_write(GPIO_FIREEYE_RELAY, energized);
#endif
}

/****************************************************************************
 * Name: gd32_fireeye_key_pressed
 *
 * Description:
 *   Read the on-board key (PA0): 10k pull-up, low when pressed.
 *
 ****************************************************************************/

bool gd32_fireeye_key_pressed(void)
{
  return !gd32_gpio_read(GPIO_FIREEYE_KEY);
}

/****************************************************************************
 * Name: gd32_fireeye_pin_raw
 *
 * Description:
 *   Drive an output pin directly (no polarity logic) for bring-up checks.
 *
 * Input Parameters:
 *   which - 0=buzzer (PB1), 1=relay (PB0), 2=alarm lamp (PD9)
 *   high  - true drives high, false drives low
 *
 ****************************************************************************/

void gd32_fireeye_pin_raw(int which, bool high)
{
  switch (which)
    {
      case 0:
        gd32_gpio_write(GPIO_FIREEYE_BUZZER, high);
        break;

      case 1:
        gd32_gpio_write(GPIO_FIREEYE_RELAY, high);
        break;

      case 2:
        gd32_gpio_write(GPIO_FIREEYE_ALARM_LED, high);
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: gd32_fireeye_set_alarm_led
 *
 * Description:
 *   Control the alarm lamp module (PD9 = JP5 pin 31, 3-pin module).
 *
 ****************************************************************************/

void gd32_fireeye_set_alarm_led(bool on)
{
#if FIREEYE_LED_ACTIVE_HIGH
  gd32_gpio_write(GPIO_FIREEYE_ALARM_LED, on);
#else
  gd32_gpio_write(GPIO_FIREEYE_ALARM_LED, !on);
#endif
}


/****************************************************************************
 * Name: gd32_fireeye_i2c_initialize
 *
 * Description:
 *   Register the I2C1 character device (/dev/i2c1) for the SSD1306 OLED.
 *   I2C0 shares PB6/PB7 with the USART0 console, so the OLED uses I2C1.
 *
 * Returned Value:
 *   0 on success, a negated errno on failure.
 *
 ****************************************************************************/

int gd32_fireeye_i2c_initialize(void)
{
  struct i2c_master_s *i2c;

  i2c = gd32_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  return i2c_register(i2c, 1);
}
