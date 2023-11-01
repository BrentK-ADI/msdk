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
    MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_USB);

    return E_NO_ERROR;
}

static int event_callback(maxusb_event_t evt, void *data)
{
    (void) data;
    int err = E_NO_ERROR;

    switch (evt) {
    case MAXUSB_EVENT_BRST:
        dcd_event_bus_signal(0, DCD_EVENT_BUS_RESET, true);
        break;
    case MAXUSB_EVENT_DPACT:
        dcd_event_bus_signal(0, DCD_EVENT_SOF, true);
        break;
    case MAXUSB_EVENT_SUSP:
      dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
    case MAXUSB_EVENT_SUDAV:
    {
      MXC_USB_SetupPkt sud;

      uint8_t setup[sizeof(MXC_USB_SetupPkt)];

      memcpy(setup, &sud, sizeof(setup));

      if(MXC_USB_GetSetup(&sud) != E_NO_ERROR)
      {
        err = E_FAIL;
        break;
      }
      dcd_event_setup_received(0, setup, true);
      break;
    

    }
    default:
        break;
    }

    return err;
}
void dcd_init(uint8_t rhport)
{
    const maxusb_cfg_options_t usb_opts = {
        .enable_hs = 1,
        .delay_us = delay_us,
        .init_callback = usbStartupCallback,
        .shutdown_callback = usbShutdownCallback,
    };

    MXC_ASSERT(MXC_USB_Init((maxusb_cfg_options_t*)&usb_opts) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_BRST) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_BRST, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_DPACT) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_DPACT, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_SUSP) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_SUDAV) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_SUDAV, event_callback, NULL) == E_NO_ERROR);
    // MXC_ASSERT(enum_init() == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_Connect() == E_NO_ERROR);
}
void dcd_int_enable(uint8_t rhport)
{
    (void)rhport;
    NVIC_EnableIRQ(USB_IRQn);
}

void dcd_int_disable(uint8_t rhport)
{
    (void)rhport;
    NVIC_DisableIRQ(USB_IRQn);
}

void dcd_edpt_stall(uint8_t rhport, uint8_t ep_addr)
{

    const uint8_t epnum = tu_edpt_number(ep_addr);

    MXC_USB_Stall(epnum);
}
void dcd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr)
{
    const uint8_t epnum = tu_edpt_number(ep_addr);

    MXC_USB_Unstall(epnum);
}

void dcd_set_address(uint8_t rhport, uint8_t dev_addr)
{
    MXC_USB_SetFuncAddr(dev_addr);
}

void dcd_remote_wakeup(uint8_t rhport)
{
    (void)rhport;

    MXC_USB_Wakeup();
}
// disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport)
{
    (void)rhport;

    MXC_USB_Disconnect();

    dcd_event_bus_signal(0, DCD_EVENT_UNPLUGGED, false);
}

void dcd_connect(uint8_t rhport)
{
    (void)rhport;

    MXC_USB_Connect();
}
bool dcd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const *desc_ep)
{
    return TRUE;
}
bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes)
{

    const uint8_t  epnum = tu_edpt_number(ep_addr);
    const uint8_t  dir   = tu_edpt_dir(ep_addr);

    static MXC_USB_Req_t usbreq;
    usbreq.reqlen = total_bytes;
    usbreq.data = buffer;
    usbreq.ep = epnum;



    if(dir == TUSB_DIR_IN)
    {
      MXC_USB_WriteEndpoint(&usbreq);
    }
    else
    {
      MXC_USB_ReadEndpoint(&usbreq);
    }


    return true;
}
void dcd_edpt_close_all(uint8_t rhport) {}

void dcd_int_handler(uint8_t rhport)
{
    MXC_USB_EventHandler();
}