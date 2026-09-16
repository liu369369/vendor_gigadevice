/****************************************************************************
 * vendor/gigadevice/boards/gd32f4/gd32f470v_start/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __VENDOR_GD32F4_BOARDS_GD32F470V_START_INCLUDE_BOARD_H
#define __VENDOR_GD32F4_BOARDS_GD32F470V_START_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#endif

#define GD32_BOARD_SYSCLK_PLL_HXTAL

/* Do not include GD32F4 header files here */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The GD32F470V-START board features a single 25MHz crystal.
 *
 * This is the default configuration:
 *   System clock source           : PLL (HXTAL)
 *   SYSCLK(Hz)                    : 168000000    Determined by PLL config
 *   HCLK(Hz)                      : 168000000    (GD32_SYSCLK_FREQUENCY)
 *   AHB Prescaler                 : 1            (GD32_RCU_CFG0_AHB_PSC)
 *   APB2 Prescaler                : 2            (GD32_RCU_CFG0_APB2_PSC)
 *   APB1 Prescaler                : 4            (GD32_RCU_CFG0_APB1_PSC)
 *   HXTAL value(Hz)               : 25000000     (GD32_BOARD_XTAL)
 *   PLLM                          : 25           (GD32_PLL_PLLM)
 *   PLLN                          : 336          (RCU_PLL_PLLN)
 *   PLLP                          : 2            (GD32_PLL_PLLP)
 *   PLLQ                          : 7            (GD32_PLL_PLLQ)
 */

/* IRC16M - 16 MHz RC factory-trimmed
 * IRC32K - 32 KHz RC
 * HXTAL  - On-board crystal frequency is 25MHz
 * LXTAL  - 32.768 kHz
 */

#ifndef CONFIG_GD32F470V_START_HXTAL_VALUE
#  define GD32_BOARD_HXTAL       25000000ul
#else
#  define GD32_BOARD_HXTAL       CONFIG_GD32F470V_START_HXTAL_VALUE
#endif

#define GD32_IRC16M_VALUE      16000000ul
#define GD32_IRC32K_VALUE      32000u
#define GD32_HXTAL_VALUE       GD32_BOARD_HXTAL
#define GD32_LXTAL_VALUE       32768u

#if defined(CONFIG_GD32F470V_START_200MHZ)

/* Main PLL Configuration.
 *
 * PLL source is HXTAL
 * PLL_VCO = (GD32_HXTAL_VALUE / PLLM) * PLLN
 *         = (25,000,000 / 25) * 400
 *         = 400,000,000
 * SYSCLK  = PLL_VCO / PLLP
 *         = 400,000,000 / 2 = 200,000,000
 * USB, SDIO and RNG Clock
 *         =  PLL_VCO / PLLQ
 *         = 48,000,000
 */

#define GD32_PLL_PLLPSC            RCU_PLL_PLLPSC(25)
#define GD32_PLL_PLLN              RCU_PLL_PLLN(400)
#define GD32_PLL_PLLP              RCU_PLL_PLLP(2)
#define GD32_PLL_PLLQ              RCU_PLL_PLLQ(8)

/* When SYSCLK is 200 MHz, USB can use IRC48M as clock source */
#define GD32_SYSCLK_FREQUENCY      200000000ul

#elif defined(CONFIG_GD32F470V_START_168MHZ)

/* Main PLL Configuration.
 *
 * PLL source is HXTAL
 * PLL_VCO = (GD32_HXTAL_VALUE / PLLM) * PLLN
 *         = (25,000,000 / 25) * 336
 *         = 336,000,000
 * SYSCLK  = PLL_VCO / PLLP
 *         = 336,000,000 / 2 = 168,000,000
 * USB, SDIO and RNG Clock
 *         =  PLL_VCO / PLLQ
 *         = 48,000,000
 */

#define GD32_PLL_PLLPSC            RCU_PLL_PLLPSC(25)
#define GD32_PLL_PLLN              RCU_PLL_PLLN(336)
#define GD32_PLL_PLLP              RCU_PLL_PLLP(2)
#define GD32_PLL_PLLQ              RCU_PLL_PLLQ(7)

#define GD32_SYSCLK_FREQUENCY      168000000ul

#elif defined(CONFIG_GD32F470V_START_240MHZ)

/* Main PLL Configuration.
 *
 * PLL source is HXTAL
 * PLL_VCO = (GD32_HXTAL_VALUE / PLLM) * PLLN
 *         = (25,000,000 / 25) * 480
 *         = 480,000,000
 * SYSCLK  = PLL_VCO / PLLP
 *         = 480,000,000 / 2 = 240,000,000
 * USB, SDIO and RNG Clock
 *         =  PLL_VCO / PLLQ
 *         = 48,000,000
 */

#define GD32_PLL_PLLPSC            RCU_PLL_PLLPSC(25)
#define GD32_PLL_PLLN              RCU_PLL_PLLN(480)
#define GD32_PLL_PLLP              RCU_PLL_PLLP(2)
#define GD32_PLL_PLLQ              RCU_PLL_PLLQ(10)

#define GD32_SYSCLK_FREQUENCY      240000000ul

#else

/* Default: 240 MHz */

#define GD32_PLL_PLLPSC            RCU_PLL_PLLPSC(25)
#define GD32_PLL_PLLN              RCU_PLL_PLLN(480)
#define GD32_PLL_PLLP              RCU_PLL_PLLP(2)
#define GD32_PLL_PLLQ              RCU_PLL_PLLQ(10)

#define GD32_SYSCLK_FREQUENCY      240000000ul

#endif

/* AHB clock (HCLK) is SYSCLK */

#define GD32_RCU_CFG0_AHB_PSC      RCU_CFG0_AHBPSC_CKSYS_DIV1  /* HCLK  = SYSCLK / 1 */
#define GD32_HCLK_FREQUENCY        GD32_SYSCLK_FREQUENCY

/* APB2 clock (PCLK2) is HCLK/2 */

#define GD32_RCU_CFG0_APB2_PSC     RCU_CFG0_APB2PSC_CKAHB_DIV2 /* PCLK2 = HCLK / 2 */
#define GD32_PCLK2_FREQUENCY       (GD32_HCLK_FREQUENCY/2)

/* APB1 clock (PCLK1) is HCLK/4 */

#define GD32_RCU_CFG0_APB1_PSC     RCU_CFG0_APB1PSC_CKAHB_DIV4 /* PCLK1 = HCLK / 4 */
#define GD32_PCLK1_FREQUENCY       (GD32_HCLK_FREQUENCY/4)

/* Timers driven from APB1 will be twice PCLK1 */

#define GD32_APB1_TIMER2_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER3_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER4_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER5_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER6_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER7_CLKIN   (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER12_CLKIN  (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER13_CLKIN  (2*GD32_PCLK1_FREQUENCY)
#define GD32_APB1_TIMER14_CLKIN  (2*GD32_PCLK1_FREQUENCY)

/* Timers driven from APB2 will be twice PCLK2 */

#define GD32_APB2_TIMER1_CLKIN   (2*GD32_PCLK2_FREQUENCY)
#define GD32_APB2_TIMER8_CLKIN   (2*GD32_PCLK2_FREQUENCY)
#define GD32_APB2_TIMER9_CLKIN   (2*GD32_PCLK2_FREQUENCY)
#define GD32_APB2_TIMER10_CLKIN  (2*GD32_PCLK2_FREQUENCY)
#define GD32_APB2_TIMER11_CLKIN  (2*GD32_PCLK2_FREQUENCY)

/* Timer Frequencies, if APBx is set to 1, frequency is same to APBx
 * otherwise frequency is 2xAPBx.
 * Note: TIMER1,8 are on APB2, others on APB1
 */

#define BOARD_TIMER1_FREQUENCY    GD32_HCLK_FREQUENCY
#define BOARD_TIMER2_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER3_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER4_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER5_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER6_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER7_FREQUENCY    (GD32_HCLK_FREQUENCY/2)
#define BOARD_TIMER8_FREQUENCY    GD32_HCLK_FREQUENCY

/* LED definitions **********************************************************/

/* The GD32F470V-START board has three LEDs: LED1 (PD4), LED2 (PD5),
 * LED3 (PG3), controlled by GPIO.
 */

typedef enum
{
    BOARD_LED1 = 0,
    BOARD_LED2 = 1,
    BOARD_LED3 = 2,
    BOARD_LEDS
} led_typedef_enum;

#define BOARD_LED1_BIT    (1 << BOARD_LED1)
#define BOARD_LED2_BIT    (1 << BOARD_LED2)
#define BOARD_LED3_BIT    (1 << BOARD_LED3)

#define LED_STARTED        0 /* NuttX has been started   OFF    OFF   OFF  */
#define LED_HEAPALLOCATE   1 /* Heap has been allocated  ON     OFF   OFF  */
#define LED_IRQSENABLED    2 /* Interrupts enabled       OFF    ON    OFF  */
#define LED_STACKCREATED   3 /* Idle stack created       OFF    OFF   ON   */
#define LED_INIRQ          4 /* In an interrupt          ON     ON    OFF  */
#define LED_SIGNAL         5 /* In a signal handler      ON     OFF   ON   */
#define LED_ASSERTION      6 /* An assertion failed      OFF    ON    ON   */
#define LED_PANIC          7 /* The system has crashed   FLASH  ON    ON   */
#define LED_IDLE           8 /* MCU is in sleep mode     OFF    FLASH OFF  */

/* Button definitions *******************************************************/

/* The GD32F470V-START board supports three user buttons:
 * Wakeup (PA0), Tamper (PC13), User (PB14).
 */

typedef enum
{
    BUTTON_WAKEUP = 0,
    BUTTON_TAMPER = 1,
    BUTTON_USER   = 2,
    NUM_BUTTONS
} key_typedef_enum;

#define BUTTON_WAKEUP_BIT    (1 << BUTTON_WAKEUP)
#define BUTTON_TAMPER_BIT    (1 << BUTTON_TAMPER)
#define BUTTON_USER_BIT      (1 << BUTTON_USER)

/* Alternate function pin selections ****************************************/

/* USART0: RX=PB7, TX=PB6 */

#define GPIO_USART0_RX  GPIO_USART0_RX_3
#define GPIO_USART0_TX  GPIO_USART0_TX_3

#if defined(CONFIG_SERIAL_IFLOWCONTROL) && defined(CONFIG_USART0_IFLOWCONTROL)
#  define GPIO_USART0_RTS GPIO_USART0_RTS_1
#endif
#if defined(CONFIG_SERIAL_OFLOWCONTROL) && defined(CONFIG_USART0_OFLOWCONTROL)
#  define GPIO_USART0_CTS GPIO_USART0_CTS_1
#endif

#if CONFIG_GD32F4_USART0_TXDMA
#  define DMA_CHANNEL_USART0_TX    DMA_REQ_USART0_TX
#endif
#if CONFIG_GD32F4_USART0_RXDMA
#  define DMA_CHANNEL_USART0_RX    DMA_REQ_USART0_RX_1
#endif

#if defined(CONFIG_GD32F4_USART_RXDMA) || defined(CONFIG_GD32F4_USART_TXDMA)
#  define USART_DMA_INTEN  (DMA_CHXCTL_SDEIE | DMA_CHXCTL_TAEIE | DMA_CHXCTL_FTFIE)
#endif

/* USART3: RX=PC11, TX=PC10 (USB virtual COM port) */

#if defined(CONFIG_GD32F470V_START_CONSOLE_VIRTUAL)
#  define GPIO_USART3_RX GPIO_USART3_RX_3
#  define GPIO_USART3_TX GPIO_USART3_TX_3
#endif

/* I2C0: SCL=PB6, SDA=PB7 */

#define GPIO_I2C0_SCL   GPIO_I2C0_SCL_1
#define GPIO_I2C0_SDA   GPIO_I2C0_SDA_1

/* I2C1: SCL=PB10, SDA=PB11
 *
 * FireEye OLED uses I2C1 (PB10=SCL / PB11=SDA, on the JP5 header).
 * I2C0 is not usable: its PB6/PB7 pins are the USART0 console pins.
 */

#define GPIO_I2C1_SCL   GPIO_I2C1_SCL_1
#define GPIO_I2C1_SDA   GPIO_I2C1_SDA_2

/* FireEye board hardware interface, in gd32f4xx_fireeye_hw.c ***************/

#include <stdbool.h>

int  gd32_fireeye_hw_initialize(void);
int  gd32_fireeye_adc_sample(int channel);
void gd32_fireeye_set_buzzer(bool on);
void gd32_fireeye_set_relay(bool energized);
bool gd32_fireeye_key_pressed(void);
int  gd32_fireeye_i2c_initialize(void);
void gd32_fireeye_set_alarm_led(bool on);
void gd32_fireeye_pin_raw(int which, bool high);
int  gd32_fireeye_w5500_fixup(FAR const uint8_t *mac);

/* SPI5 flash: MISO=PG12, MOSI=PG14, SCK=PG13, CS=PG9 */


/* SPI1 chip selects: PB12 = W25Q64 flash, PE7 = W5500 Ethernet *************/

/* SPI1 signals: SCK=PB13, MISO=PB14, MOSI=PB15 (W5500/W25Q64 shared) */

#define GPIO_SPI1_SCK        GPIO_SPI1_SCK_3
#define GPIO_SPI1_MISO       GPIO_SPI1_MISO_1
#define GPIO_SPI1_MOSI       GPIO_SPI1_MOSI_1

#define GPIO_SPI1_SCK_PIN    ((GPIO_SPI1_SCK  & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)
#define GPIO_SPI1_MISO_PIN   ((GPIO_SPI1_MISO & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)
#define GPIO_SPI1_MOSI_PIN   ((GPIO_SPI1_MOSI & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)


#define GPIO_SPI1_FLASH_CSPIN   (GPIO_CFG_PORT_B | GPIO_PIN12_OUTPUT)
#define GPIO_SPI1_CSPIN         GPIO_SPI1_FLASH_CSPIN
#define GPIO_SPI1_ETH_CSPIN     (GPIO_CFG_PORT_E | GPIO_PIN7_OUTPUT)

/* W5500 control pins: RST = PE9, INT = PE11 (active low, falling edge) */

#define GPIO_W5500_RESET        (GPIO_CFG_PORT_E | GPIO_PIN9_OUTPUT)
#define GPIO_W5500_INTR         (GPIO_CFG_MODE_INPUT | GPIO_CFG_PUPD_PULLUP | \
                                 GPIO_CFG_PORT_E | GPIO_CFG_PIN_11)

#define GPIO_SPI5_CSPIN     (GPIO_CFG_PORT_G | GPIO_PIN9_OUTPUT)

#define GPIO_SPI5_MISO_PIN  ((GPIO_SPI5_MISO & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)
#define GPIO_SPI5_MOSI_PIN  ((GPIO_SPI5_MOSI & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)
#define GPIO_SPI5_SCK_PIN   ((GPIO_SPI5_SCK  & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)

#define GPIO_SPI5_IO2_PIN   ((GPIO_SPI5_IO2 & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)
#define GPIO_SPI5_IO3_PIN   ((GPIO_SPI5_IO3 & ~GPIO_CFG_SPEED_MASK) | GPIO_CFG_SPEED_25MHZ)

/* Ethernet RMII pins */

#define GPIO_ENET_RMII_TX_EN   GPIO_ENET_RMII_TX_EN_2
#define GPIO_ENET_RMII_TXD0    GPIO_ENET_RMII_TXD0_2
#define GPIO_ENET_RMII_TXD1    GPIO_ENET_RMII_TXD1_2

#ifdef CONFIG_GD32F4_ENET_PTP
#  define GPIO_ENET_PPS_OUT    GPIO_ENET_PPS_OUT_1
#endif

#ifdef CONFIG_GD32F4_RMII_CKOUT0
#  define BOARD_CFG_CKOUT0_SOURCE    RCU_CFG0_CKOUT0SEL_PLLP
#  define BOARD_CFG_CKOUT0_DIVIDER   RCU_CFG0_CKOUT0_DIV4
#endif

/* SDIO: CMD=PD2, CLK=PC12, D0=PC8, D1=PC9, D2=PC10, D3=PC11 */

#define GPIO_SDIO_CMD_PIN   GPIO_SDIO_CMD_2
#define GPIO_SDIO_CLK_PIN   GPIO_SDIO_CK_2
#define GPIO_SDIO_DAT0_PIN  GPIO_SDIO_D0_2
#define GPIO_SDIO_DAT1_PIN  GPIO_SDIO_D1_3
#define GPIO_SDIO_DAT2_PIN  GPIO_SDIO_D2_3
#define GPIO_SDIO_DAT3_PIN  GPIO_SDIO_D3

#ifdef CONFIG_GD32F4_SDIO_DMA
#  define SDIO_DMA_INTEN    (DMA_CHXCTL_SDEIE | DMA_CHXCTL_TAEIE | DMA_CHXCTL_FTFIE)
#endif

#ifdef CONFIG_GD32F4_TLI

/* LCD
 *
 * The GD32F4 board contains an onboard TFT LCD connected to the
 * TLI interface of the uC.
 * The LCD is 480x272 pixels.
 * Define the parameters of the LCD and the interface here.
 */

/* Panel configuration
 *
 * LCD Panel is 4.3 inch TFT (480x272)
 *
 * PLLSAI settings
 * PLLSAIN                : 192
 * PLLSAIP                : 2
 * PLLSAIR                : 3
 *
 * Timings
 * Horizontal Front Porch :  2   (GD32_TLI_HFP)
 * Horizontal Back Porch  :  2   (GD32_TLI_HBP)
 * Vertical Front Porch   :  2   (GD32_TLI_VFP)
 * Vertical Back Porch    :  2   (GD32_TLI_VBP)
 *
 * Horizontal Sync        : 41   (GD32_TLI_HSYNC)
 * Vertical Sync          : 10   (GD32_TLI_VSYNC)
 *
 * Active Width           : 480  (BOARD_TLI_WIDTH)
 * Active Height          : 272  (BOARD_TLI_HEIGHT)
 */

/* TLI PLL configuration
 *
 * PLLSAI shares PLLM with the main PLL (PLLM = 25 for this board).
 *
 * PLLSAI_VCO = GD32_HXTAL_VALUE / PLLM * PLLSAIN
 *            = 25,000,000 / 25 * 192
 *            = 192,000,000
 *
 * PLL LCD clock output
 *            = PLLSAI_VCO / PLLSAIR / PLLSAIRDIV
 *            = 192,000,000 / 3 / 8
 *            = 8,000,000
 */

/* Defined panel settings */

#  define BOARD_TLI_WIDTH              480
#  define BOARD_TLI_HEIGHT             272

#  define BOARD_TLI_OUTPUT_BPP           16
#  define BOARD_TLI_HFP                  2
#  define BOARD_TLI_HBP                  2
#  define BOARD_TLI_VFP                  2
#  define BOARD_TLI_VBP                  2
#  define BOARD_TLI_HSYNC                41
#  define BOARD_TLI_VSYNC                10

#  define BOARD_TLI_PLLSAIN              192
#  define BOARD_TLI_PLLSAIP              2
#  define BOARD_TLI_PLLSAIR              3
/* #  define BOARD_TLI_PLLSAIQ              3 */

/* Division factor for LCD clock */

/* #  define GD32_RCU_DCKCFGR_PLLSAIDIVR    RCC_DCKCFGR_PLLSAIDIVR_DIV8 */

#  define GD32_RCU_CFG1_PLLSAIRDIV    RCU_CFG1_PLLSAIRDIV_DIV_8

/* Pixel Clock Polarity */

#  define BOARD_TLI_GCR_PCPOL            0 /* !TLI_GCR_PCPOL */

/* Data Enable Polarity */

#  define BOARD_TLI_GCR_DEPOL            0 /* !TLI_GCR_DEPOL */

/* Vertical Sync Polarity */

#  define BOARD_TLI_GCR_VSPOL            0 /* !TLI_GCR_VSPOL */

/* Horizontal Sync Polarity */

#  define BOARD_TLI_GCR_HSPOL            0 /* !TLI_GCR_HSPOL */

#  define BOARD_TLI_GCR_BCR              0xFF /* LI_GCR_BCR */

#  define BOARD_TLI_GCR_BCG              0xFF /* LI_GCR_BCG */

#  define BOARD_TLI_GCR_BCB              0xFF /* LI_GCR_BCB */

/* GPIO pinset */

#define GPIO_TLI_PINS                  28 /* 18-bit display */
#define GPIO_TLI_R2                    GPIO_TLI_R2_2
#define GPIO_TLI_R3                    GPIO_TLI_R3_2
#define GPIO_TLI_R4                    GPIO_TLI_R4_2
#define GPIO_TLI_R5                    GPIO_TLI_R5_2
#define GPIO_TLI_R6                    GPIO_TLI_R6_3
#define GPIO_TLI_R7                    GPIO_TLI_R7_2
#define GPIO_TLI_G2                    GPIO_TLI_G2_2
#define GPIO_TLI_G3                    GPIO_TLI_G3_3
#define GPIO_TLI_G4                    GPIO_TLI_G4_2
#define GPIO_TLI_G5                    GPIO_TLI_G5_2
#define GPIO_TLI_G6                    GPIO_TLI_G6_2
#define GPIO_TLI_G7                    GPIO_TLI_G7_2
#define GPIO_TLI_B2                    GPIO_TLI_B2_2
#define GPIO_TLI_B3                    GPIO_TLI_B3_2
#define GPIO_TLI_B4                    GPIO_TLI_B4_3
#define GPIO_TLI_B5                    GPIO_TLI_B5_2
#define GPIO_TLI_B6                    GPIO_TLI_B6_2
#define GPIO_TLI_B7                    GPIO_TLI_B7_2

#define GPIO_TLI_VSYNC                 GPIO_TLI_VSYNC_2  /* PI9 */
#define GPIO_TLI_HSYNC                 GPIO_TLI_HSYNC_2  /* PI10 */
#define GPIO_TLI_DE                    GPIO_TLI_DE_2
#define GPIO_TLI_CLK                   GPIO_TLI_PIXCLK_2 /* PG7 */
#define GPIO_LCD_PWM                   (GPIO_CFG_MODE_OUTPUT | \
                                        GPIO_CFG_PUPD_PULLUP | \
                                        GPIO_CFG_PP | \
                                        GPIO_CFG_SPEED_50MHZ | \
                                        GPIO_CFG_PORT_B | \
                                        GPIO_CFG_PIN_15)

/* Alternate GPIO pin settings (18-bit display)
 *
 * #  define GPIO_TLI_PINS                  18
 *
 * #  define GPIO_TLI_R2                    GPIO_TLI_R2_1
 * #  define GPIO_TLI_R3                    GPIO_TLI_R3_1
 * #  define GPIO_TLI_R4                    GPIO_TLI_R4_1
 * #  define GPIO_TLI_R5                    GPIO_TLI_R5_1
 * #  define GPIO_TLI_R6                    GPIO_TLI_R6_1
 * #  define GPIO_TLI_R7                    GPIO_TLI_R7_1
 *
 * #  define GPIO_TLI_G2                    GPIO_TLI_G2_1
 * #  define GPIO_TLI_G3                    GPIO_TLI_G3_1
 * #  define GPIO_TLI_G4                    GPIO_TLI_G4_1
 * #  define GPIO_TLI_G5                    GPIO_TLI_G5_1
 * #  define GPIO_TLI_G6                    GPIO_TLI_G6_1
 * #  define GPIO_TLI_G7                    GPIO_TLI_G7_1
 *
 * #  define GPIO_TLI_B2                    GPIO_TLI_B2_1
 * #  define GPIO_TLI_B3                    GPIO_TLI_B3_1
 * #  define GPIO_TLI_B4                    GPIO_TLI_B4_1
 * #  define GPIO_TLI_B5                    GPIO_TLI_B5_1
 * #  define GPIO_TLI_B6                    GPIO_TLI_B6_1
 * #  define GPIO_TLI_B7                    GPIO_TLI_B7_1
 *
 * #  define GPIO_TLI_VSYNC                 GPIO_TLI_VSYNC_1
 * #  define GPIO_TLI_HSYNC                 GPIO_TLI_HSYNC_1
 * #  define GPIO_TLI_DE                    GPIO_TLI_DE_1
 * #  define GPIO_TLI_CLK                   GPIO_TLI_CLK_1
 */

/* Configure PLLSAI */

/* Alternate PLLSAI settings
 *
 * #define GD32_RCU_PLLSAICFGR_PLLSAIN
 *         RCC_PLLSAICFGR_PLLSAIN(BOARD_TLI_PLLSAIN)
 * #define GD32_RCU_PLLSAICFGR_PLLSAIR
 *         RCC_PLLSAICFGR_PLLSAIR(BOARD_TLI_PLLSAIR)
 * #define GD32_RCU_PLLSAICFGR_PLLSAIQ
 *         RCC_PLLSAICFGR_PLLSAIQ(BOARD_TLI_PLLSAIQ)
 */

#define GD32_RCU_PLLSAI_PLLSAIN    RCU_PLLSAI_PLLSAIN(BOARD_TLI_PLLSAIN)
#define GD32_RCU_PLLSAI_PLLSAIP    RCU_PLLSAI_PLLSAIP(BOARD_TLI_PLLSAIP)
#define GD32_RCU_PLLSAI_PLLSAIR    RCU_PLLSAI_PLLSAIR(BOARD_TLI_PLLSAIR)
/* #define GD32_RCU_PLLSAI_PLLSAIQ    RCU_PLLSAI_PLLSAIQ(BOARD_TLI_PLLSAIQ) */

#if defined(CONFIG_GD32F4_TLI) && defined(CONFIG_HEAP2_BASE)
#  if (CONFIG_HEAP2_BASE + CONFIG_HEAP2_SIZE) > CONFIG_GD32F4_TLI_FB_BASE
#    error "HEAP2 region overlaps TLI framebuffer"
#  endif
#endif

#endif /* CONFIG_GD32F4_TLI */

/* DCI (Digital Camera Interface) *******************************************/

#ifdef CONFIG_GD32F4_DCI

/* DCI GPIO pin selections for GD32F470IK-EVAL + OV2640 (8-bit parallel)
 *
 *   GD32F470IK-EVAL       OV2640
 *   GPIO     SIGNAL       PIN
 *   -------- ------------ --------
 *   PA4      DCI_HSYNC    HREF
 *   PA6      DCI_PIXCLK   PCLK
 *   PG9      DCI_VSYNC    VSYNC
 *   PC6      DCI_D0       D0
 *   PC7      DCI_D1       D1
 *   PC8      DCI_D2       D2
 *   PC9      DCI_D3       D3
 *   PC11     DCI_D4       D4
 *   PD3      DCI_D5       D5
 *   PB8      DCI_D6       D6
 *   PB9      DCI_D7       D7
 *
 * OV2640 control via I2C0 (SCCB):
 *   PB6      I2C0_SCL     SIOC
 *   PB7      I2C0_SDA     SIOD
 *
 * OV2640 master clock:
 *   PA8      CKOUT0       XCLK
 */

#define BOARD_DCI_D0       GPIO_DCI_D0_2     /* PC6  */
#define BOARD_DCI_D1       GPIO_DCI_D1_2     /* PC7  */
#define BOARD_DCI_D2       GPIO_DCI_D2_1     /* PC8  */
#define BOARD_DCI_D3       GPIO_DCI_D3_1     /* PC9  */
#define BOARD_DCI_D4       GPIO_DCI_D4_1     /* PC11 */
#define BOARD_DCI_D5       GPIO_DCI_D5_2     /* PD3  */
#define BOARD_DCI_D6       GPIO_DCI_D6_1     /* PB8  */
#define BOARD_DCI_D7       GPIO_DCI_D7_1     /* PB9  */
#define BOARD_DCI_HSYNC    GPIO_DCI_HSYNC_1  /* PA4  */
#define BOARD_DCI_VSYNC    GPIO_DCI_VSYNC_2  /* PG9  */
#define BOARD_DCI_PIXCLK   GPIO_DCI_PIXCLK   /* PA6  */

/* DCI clock polarity: capture at rising edge of pixel clock */

#define BOARD_DCI_CK_POLARITY_RISING  1

/* OV2640 SCCB (I2C) configuration */

#define BOARD_DCI_I2C_BUS        0          /* I2C0 for OV2640 SCCB */
#define BOARD_OV2640_I2C_ADDR    0x30       /* OV2640 7-bit I2C address */

/* OV2640 master clock via CKOUT0 on PA8, aligned with the vendor demo */

#define BOARD_OV2640_MCLK_PIN    (GPIO_CFG_MODE_AF | GPIO_CFG_PUPD_PULLUP | \
                                  GPIO_CFG_PP | GPIO_CFG_SPEED_200MHZ | \
                                  GPIO_CFG_AF_0 | GPIO_CFG_PORT_A | \
                                  GPIO_CFG_PIN_8)
#define BOARD_OV2640_MCLK_SOURCE  RCU_CFG0_CKOUT0SEL_HXTAL
#define BOARD_OV2640_MCLK_DIVIDER RCU_CFG0_CKOUT0_DIV2

#endif /* CONFIG_GD32F4_DCI */

#endif /* __VENDOR_GD32F4_BOARDS_GD32F470V_START_INCLUDE_BOARD_H */
