#############################################################################
# vendor/gigadevice/boards/gd32f4/gd32f470v_start/src/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
#############################################################################

include $(TOPDIR)/Make.defs

# Vendor chip extension sources directory (relative to nuttx TOPDIR)
VENDOR_CHIP_SRCDIR = $(TOPDIR)$(DELIM)..$(DELIM)vendor$(DELIM)gigadevice$(DELIM)chips$(DELIM)gd32f4$(DELIM)src

# Add vendor chip src/ and hardware/ to the include path. Keep src/ ahead of
# hardware/ so interface headers such as gd32f4xx_exmc.h and gd32f4xx_tli.h
# resolve to the driver declarations instead of the register-only hardware
# headers. The hardware directory still needs to shadow pinmap headers.
CFLAGS += ${INCDIR_PREFIX}"$(VENDOR_CHIP_SRCDIR)"
CFLAGS += ${INCDIR_PREFIX}"$(VENDOR_CHIP_SRCDIR)$(DELIM)hardware"

CSRCS = gd32f4xx_boot.c

ifeq ($(CONFIG_BOARDCTL),y)
CSRCS += gd32f4xx_appinit.c gd32f4xx_bringup.c
else ifeq ($(CONFIG_BOARD_LATE_INITIALIZE),y)
CSRCS += gd32f4xx_bringup.c
endif

ifeq ($(CONFIG_ARCH_LEDS),y)
CSRCS += gd32f4xx_autoleds.c
else
CSRCS += gd32f4xx_userleds.c
endif

# FireEye board self-test: LED heartbeat (no console required)
CSRCS += gd32f4xx_selftest.c

# FireEye board hardware: ADC sampling / alarm outputs / button / I2C1
CSRCS += gd32f4xx_fireeye_hw.c

# FireEye W5500 Ethernet
ifeq ($(CONFIG_NET_W5500),y)
CSRCS += gd32f4xx_w5500.c
endif

ifeq ($(CONFIG_ARCH_BUTTONS),y)
CSRCS += gd32f4xx_buttons.c
endif

ifeq ($(CONFIG_DEV_GPIO),y)
CSRCS += gd32f4xx_gpio.c
endif

ifeq ($(CONFIG_SPI),y)
CSRCS += gd32f4xx_spi.c
endif

ifeq ($(CONFIG_MMCSD),y)
CSRCS += gd32f4xx_sdio.c
endif

ifeq ($(CONFIG_MTD_GD25),y)
CSRCS += gd32f4xx_gd25.c
endif

ifeq ($(CONFIG_MTD_AT24XX),y)
CSRCS += gd32f4xx_at24.c
endif

ifeq ($(CONFIG_GD32F4_EXMC),y)
CSRCS += gd32f4xx_exmc.c gd32f4xx_extmem.c
VPATH += $(VENDOR_CHIP_SRCDIR)
DEPPATH += --dep-path $(VENDOR_CHIP_SRCDIR)
endif

ifeq ($(CONFIG_GD32F4_TLI),y)
CSRCS += gd32f4xx_tli.c gd32f4xx_lcd.c
VPATH += $(VENDOR_CHIP_SRCDIR)
DEPPATH += --dep-path $(VENDOR_CHIP_SRCDIR)
endif

ifeq ($(CONFIG_GD32F4_IPA),y)
CSRCS += gd32f4xx_ipa.c
VPATH += $(VENDOR_CHIP_SRCDIR)
DEPPATH += --dep-path $(VENDOR_CHIP_SRCDIR)
endif

ifneq ($(CONFIG_GD32F4_TLI),y)
ifeq ($(CONFIG_VIDEO_FB), y)
CSRCS += gd32f4xx_lcd.c
endif
endif

ifeq ($(CONFIG_GD32F4_DCI), y)
CSRCS += gd32f4xx_dci.c
VPATH += $(VENDOR_CHIP_SRCDIR)
DEPPATH += --dep-path $(VENDOR_CHIP_SRCDIR)
CSRCS += gd32f4xx_dci_ov2640.c
endif

ifeq ($(CONFIG_GD32F4_CAMPREVIEW), y)
CSRCS += gd32f4xx_campreview.c
endif

ifeq ($(CONFIG_BOARDCTL_RESET),y)
CSRCS += gd32f4xx_reset.c
endif

include $(TOPDIR)/boards/Board.mk
