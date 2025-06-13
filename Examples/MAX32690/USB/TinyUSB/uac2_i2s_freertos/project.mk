# This file can be used to set build configuration
# variables.  These variables are defined in a file called
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analogdevicesinc.github.io/msdk/USERGUIDE/#build-system

# Enable TINYUSB library and FreeRTOS
LIB_TINYUSB=1
LIB_FREERTOS=1
TINYUSB_CONFIG_DIR = ./
MFLOAT_ABI = hard
MXC_OPTIMIZE_CFLAGS = -O3

PROJ_CFLAGS += -DLOGGING_UART=2 -DGLOBAL_LOG_LEVEL=LOG_LEVEL_INFO
PROJ_CFLAGS += -DCFG_TUSB_MCU=OPT_MCU_MAX32690 -DBOARD_TUD_MAX_SPEED=OPT_MODE_HIGH_SPEED

ifneq ($(BOARD),EvKit_V1)
$(error ERR_NOTSUPPORTED: This project is only supported on the MAX32690 EvKit_V1 board.  See https://analogdevicesinc.github.io/msdk/USERGUIDE/#board-support-packages)
endif
