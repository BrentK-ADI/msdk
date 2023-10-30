#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "usb_hwopt.h"
#include "mxc_errors.h"
#include "mcr_regs.h"
#include "mxc_sys.h"
#include "mxc_delay.h"
#include "board.h"
#include "led.h"
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"
#include "cdc_acm.h"
#include "descriptors.h"
#include "tusb_option.h"
#include "device/dcd.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h" // to use defer function helper
void delay_us(unsigned int usec)
{
    /* mxc_delay() takes unsigned long, so can't use it directly */
    MXC_Delay(usec);
}
/******************************************************************************/
int usbStartupCallback()
{
    MXC_SYS_ClockSourceEnable(MXC_SYS_CLOCK_IPO);
    MXC_MCR->ldoctrl |= MXC_F_MCR_LDOCTRL_0P9EN;
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_USB);
    MXC_SYS_Reset_Periph(MXC_SYS_RESET0_USB);

    return E_NO_ERROR;
}

/******************************************************************************/
int usbShutdownCallback()
{
    //return MXC_SYS_USBHS_Shutdown();
    MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_USB);

    return E_NO_ERROR;
}
void dcd_init(uint8_t)
{
    maxusb_cfg_options_t usb_opts;

    usb_opts.enable_hs = 1;
    usb_opts.delay_us = delay_us; /* Function which will be used for delays */
    usb_opts.init_callback = usbStartupCallback;
    usb_opts.shutdown_callback = usbShutdownCallback;

    MXC_USB_Init(&usb_opts);
}
void dcd_int_enable(uint8_t rhport)
{
  (void) rhport;
  NVIC_EnableIRQ(USB_IRQn);
}

void dcd_int_disable(uint8_t rhport)
{
  (void) rhport;
  NVIC_DisableIRQ(USB_IRQn);
}



void dcd_set_address (uint8_t rhport, uint8_t dev_addr)
{

}

void dcd_remote_wakeup(uint8_t rhport)
{
  (void) rhport;

  // Bring controller out of low power mode
  // will start wakeup when USBWUALLOWED is set
  
}
// disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport)
{
  (void) rhport;
  

  // Disable Pull-up does not trigger Power USB Removed, in fact it have no
  // impact on the USB Power status at all -> need to submit unplugged event to the stack.
  dcd_event_bus_signal(0, DCD_EVENT_UNPLUGGED, false);
}

void dcd_disconnect(uint8_t rhport)
{
  (void) rhport;
  

  // Disable Pull-up does not trigger Power USB Removed, in fact it have no
  // impact on the USB Power status at all -> need to submit unplugged event to the stack.
  dcd_event_bus_signal(0, DCD_EVENT_UNPLUGGED, false);
}
void dcd_connect(uint8_t rhport)
{
  (void) rhport;
  
}
