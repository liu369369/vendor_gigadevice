# ##############################################################################
# vendor/gigadevice/boards/gd32f4/gd32f470v_start/src/CMakeLists.txt
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

# Vendor chip extension sources directory
set(VENDOR_CHIP_SRCDIR
    ${NUTTX_BOARD_ABS_DIR}/../../../chips/gd32f4/src)

set(SRCS gd32f4xx_boot.c)

if(CONFIG_BOARDCTL)
  list(APPEND SRCS gd32f4xx_appinit.c gd32f4xx_bringup.c)
elseif(CONFIG_BOARD_LATE_INITIALIZE)
  list(APPEND SRCS gd32f4xx_bringup.c)
endif()

if(CONFIG_ARCH_LEDS)
  list(APPEND SRCS gd32f4xx_autoleds.c)
else()
  list(APPEND SRCS gd32f4xx_userleds.c)
endif()

# FireEye board self-test: LED heartbeat (no console required)
list(APPEND SRCS gd32f4xx_selftest.c)

# FireEye board hardware: ADC sampling / alarm outputs / button / I2C1
list(APPEND SRCS gd32f4xx_fireeye_hw.c)

# FireEye W5500 Ethernet (built only when CONFIG_NET_W5500 is enabled)
if(CONFIG_NET_W5500)
  list(APPEND SRCS gd32f4xx_w5500.c)
endif()

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS gd32f4xx_buttons.c)
endif()

if(CONFIG_DEV_GPIO)
  list(APPEND SRCS gd32f4xx_gpio.c)
endif()

if(CONFIG_SPI)
  list(APPEND SRCS gd32f4xx_spi.c)
endif()

if(CONFIG_MMCSD)
  list(APPEND SRCS gd32f4xx_sdio.c)
endif()

if(CONFIG_MTD_GD25)
  list(APPEND SRCS gd32f4xx_gd25.c)
endif()

if(CONFIG_MTD_AT24XX)
  list(APPEND SRCS gd32f4xx_at24.c)
endif()

# EXMC (External Memory Controller) driver + board-level SDRAM init
if(CONFIG_GD32F4_EXMC)
  list(APPEND SRCS ${VENDOR_CHIP_SRCDIR}/gd32f4xx_exmc.c)
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/gd32f4xx_extmem.c)
endif()

# TLI (TFT-LCD Interface / LCD controller) driver + board-level LCD init
if(CONFIG_GD32F4_TLI)
  list(APPEND SRCS ${VENDOR_CHIP_SRCDIR}/gd32f4xx_tli.c)
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/gd32f4xx_lcd.c)
endif()

if(NOT CONFIG_GD32F4_TLI AND CONFIG_VIDEO_FB)
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/gd32f4xx_lcd.c)
endif()

# IPA (Image Processing Accelerator / DMA2D) driver
if(CONFIG_GD32F4_IPA)
  list(APPEND SRCS ${VENDOR_CHIP_SRCDIR}/gd32f4xx_ipa.c)
endif()

# DCI (Digital Camera Interface) driver
if(CONFIG_GD32F4_DCI)
  list(APPEND SRCS ${VENDOR_CHIP_SRCDIR}/gd32f4xx_dci.c)
  list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/gd32f4xx_dci_ov2640.c)
endif()

if(CONFIG_BOARDCTL_RESET)
  list(APPEND SRCS gd32f4xx_reset.c)
endif()

target_sources(board PRIVATE ${SRCS})

target_include_directories(
  board BEFORE
  PRIVATE ${VENDOR_CHIP_SRCDIR} ${VENDOR_CHIP_SRCDIR}/hardware)
