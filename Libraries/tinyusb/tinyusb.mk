################################################################################
 # Copyright (C) 2023 Maxim Integrated Products, Inc., All Rights Reserved.
 #
 # Permission is hereby granted, free of charge, to any person obtaining a
 # copy of this software and associated documentation files (the "Software"),
 # to deal in the Software without restriction, including without limitation
 # the rights to use, copy, modify, merge, publish, distribute, sublicense,
 # and/or sell copies of the Software, and to permit persons to whom the
 # Software is furnished to do so, subject to the following conditions:
 #
 # The above copyright notice and this permission notice shall be included
 # in all copies or substantial portions of the Software.
 #
 # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 # IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 # OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 # ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 # OTHER DEALINGS IN THE SOFTWARE.
 #
 # Except as contained in this notice, the name of Maxim Integrated
 # Products, Inc. shall not be used except as stated in the Maxim Integrated
 # Products, Inc. Branding Policy.
 #
 # The mere transfer of this software does not imply any licenses
 # of trade secrets, proprietary technology, copyrights, patents,
 # trademarks, maskwork rights, or any other form of intellectual
 # property whatsoever. Maxim Integrated Products, Inc. retains all
 # ownership rights.
 #
 ###############################################################################

################################################################################
# This file can be included in a project makefile to build the library for the 
# project.
###############################################################################

ifeq "$(LIB_TUSB_DIR)" ""
# If CLI_DIR is not specified, this Makefile will locate itself.
LIB_TUSB_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
endif

include ${LIB_TUSB_DIR}src/tinyusb_srcs.mk

IPATH += ${LIB_TUSB_DIR}src/
IPATH += ${LIB_TUSB_DIR}hw/bsp

IPATH += ${LIB_TUSB_DIR}/src/device
IPATH += ${LIB_TUSB_DIR}/src/common
IPATH += ${LIB_TUSB_DIR}/src/host
IPATH += ${LIB_TUSB_DIR}/src/osal
IPATH += ${LIB_TUSB_DIR}/src/typec
IPATH += ${LIB_TUSB_DIR}/src/class/audio
IPATH += ${LIB_TUSB_DIR}/src/class/bth
IPATH += ${LIB_TUSB_DIR}/src/class/cdc
IPATH += ${LIB_TUSB_DIR}/src/class/dfu
IPATH += ${LIB_TUSB_DIR}/src/class/hid
IPATH += ${LIB_TUSB_DIR}/src/class/midi
IPATH += ${LIB_TUSB_DIR}/src/class/msc
IPATH += ${LIB_TUSB_DIR}/src/class/net
IPATH += ${LIB_TUSB_DIR}/src/class/usbtmc
IPATH += ${LIB_TUSB_DIR}/src/class/vendor
IPATH += ${LIB_TUSB_DIR}/src/class/video

VPATH += ${LIB_TUSB_DIR}/src/
VPATH += ${LIB_TUSB_DIR}/src/common/
VPATH += ${LIB_TUSB_DIR}/src/device/
VPATH += ${LIB_TUSB_DIR}/src/host/
VPATH += ${LIB_TUSB_DIR}/src/osal/
VPATH += ${LIB_TUSB_DIR}/src/typec/

VPATH += ${LIB_TUSB_DIR}src/class/audio
VPATH += ${LIB_TUSB_DIR}src/class/bth
VPATH += ${LIB_TUSB_DIR}src/class/cdc
VPATH += ${LIB_TUSB_DIR}src/class/dfu
VPATH += ${LIB_TUSB_DIR}src/class/hid
VPATH += ${LIB_TUSB_DIR}src/class/midi
VPATH += ${LIB_TUSB_DIR}src/class/msc
VPATH += ${LIB_TUSB_DIR}src/class/net
VPATH += ${LIB_TUSB_DIR}src/class/usbtmc
VPATH += ${LIB_TUSB_DIR}src/class/vendor
VPATH += ${LIB_TUSB_DIR}src/class/video
VPATH += ${LIB_TUSB_DIR}src/portable/${TARGET_UC}
VPATH += ${LIB_TUSB_DIR}hw/bsp

SRCS += ${LIB_TUSB_DIR}/src/portable/${TARGET_UC}/dcd_${TARGET_LC}.c
SRCS += ${LIB_TUSB_DIR}/src/device/usbd.c
SRCS += ${TINYUSB_SRC_C}
SRCS += ${LIB_TUSB_DIR}hw/bsp/board.c
SRCS += ${LIB_TUSB_DIR}hw/bsp/board_${TARGET_LC}.c


PROJ_CFLAGS += -I${LIB_TUSB_DIR}/src









# Use absolute paths if building within eclipse environment.
ifeq "$(ECLIPSE)" "1"
SRCS := $(abspath $(SRCS))
endif
