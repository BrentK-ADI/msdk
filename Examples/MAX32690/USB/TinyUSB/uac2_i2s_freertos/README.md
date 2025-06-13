## Description

The example demonstrates the use of USB UAC2 Device Class as a audio input device
(i.e microphone/Line Input, etc). The example reads I2S data from the MAX32690
EvKit's on-board MAX9867 Codec and sends it via UAC2 to the host over USB.

Currently the example is fixed a single channel (mono) UAC2 device, and a fixed
48kHz audio sampling rate.  The volume/mute controls via USB are recorded
however are not being utilized yet.

### Future Features
 - Adjustable (compile time or run-time) Sampling Rate
 - Stereo audio (UAC2)
 - Volume/Mute control
 - I2S Data Processing/Filtering?

## Software
### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

tusb_config.h by default has logging disabled via the CFG_TUSB_DEBUG definition.
For TinyUSB console logging, set the debug level to 1, 2, or 3, with a higher value
indicating more verbose logging.

## Required Connections

This project is only available on the MAX32690EVKIT

If using the MAX32690EVKIT:
-   Connect a USB cable between the PC and the CN2 (USB/PWR/UART) connector.
-   Connect a Audio source to the Line In jack of the EvKit
-   Connect a USB cable between the PC and the CN1 (USB/PWR) connector.

## Expected Output

Audio from the LineIn jack will be available for listening or recording on the
host PC.
