# This file can be used to set build configuration
# variables.  These variables are defined in a file called 
# "Makefile" that is located next to this one.

# For instructions on how to use this system, see
# https://analog-devices-msdk.github.io/msdk/USERGUIDE/#build-system

# **********************************************************

# If you have secure version of MCU, set SBT=1 to generate signed binary
# For more information on how sing process works, see
# https://www.analog.com/en/education/education-library/videos/6313214207112.html
SBT=0

# Enable MAXUSB library
LIB_MAXUSB = 1
LIB_TUSB = 1

PROJ_CFLAGS += -DCFG_TUSB_MCU=MAX32690
PROJ_CFLAGS += -DCFG_TUD_ENABLED=1
PROJ_CFLAGS += -DTUP_DCD_ENDPOINT_MAX=15
