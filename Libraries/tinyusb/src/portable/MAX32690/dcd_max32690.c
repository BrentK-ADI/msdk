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

#define BYTE_HIGH(val) (((val) >> 8) & 0xFF)
#define BYTE_LOW(val) ((val) & 0xFF)

static void write_callback(void *cbdata);
static void read_callback(void *cbdata);

static volatile MXC_USB_Req_t writereq = { .callback = write_callback };
static volatile MXC_USB_Req_t readreq = { .callback = read_callback };
static volatile bool write_done = false;
static volatile bool read_done = false;

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
    (void)data;
    int err = E_NO_ERROR;

    switch (evt) {
    case MAXUSB_EVENT_BRST:
        puts("Bus Reset");
        dcd_event_bus_signal(0, DCD_EVENT_BUS_RESET, true);
        break;
    case MAXUSB_EVENT_DPACT:
        dcd_event_bus_signal(0, DCD_EVENT_SOF, true);
        break;
    case MAXUSB_EVENT_SUSP:
        dcd_event_bus_signal(0, DCD_EVENT_SUSPEND, true);
    case MAXUSB_EVENT_SUDAV: {
        MXC_USB_SetupPkt sud;
        if (MXC_USB_GetSetup(&sud) != E_NO_ERROR) {
            err = E_FAIL;
            break;
        }
        puts("Setup Data Available");

        uint8_t setup[sizeof(MXC_USB_SetupPkt)];
        memcpy(setup, &sud, sizeof(setup));

        dcd_event_xfer_complete(0, readreq.ep, readreq.actlen, XFER_RESULT_SUCCESS, true);
        dcd_event_setup_received(0, setup, true);

        break;
    }
    case MAXUSB_EVENT_BRSTDN:
        dcd_event_bus_signal(0, DCD_EVENT_BUS_RESET, true);
        break;

    case MAXUSB_EVENT_RWUDN:
    case MAXUSB_EVENT_BACT:
    case MAXUSB_EVENT_NOVBUS:
    case MAXUSB_EVENT_VBUS:
    default:
        printf("OTHER EVENT");
        break;
    }

    MXC_ASSERT(MXC_USB_EventClear(evt) == E_NO_ERROR);

    return err;
}
void dcd_init(uint8_t rhport)
{
    puts("Initializing USB\n");
    const maxusb_cfg_options_t usb_opts = {
        .enable_hs = 0,
        .delay_us = delay_us,
        .init_callback = usbStartupCallback,
        .shutdown_callback = usbShutdownCallback,
    };

    MXC_ASSERT(MXC_USB_Init((maxusb_cfg_options_t *)&usb_opts) == E_NO_ERROR);

    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_BRST) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_BRST, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_DPACT) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_DPACT, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_SUSP) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, event_callback, NULL) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventClear(MAXUSB_EVENT_SUDAV) == E_NO_ERROR);
    MXC_ASSERT(MXC_USB_EventEnable(MAXUSB_EVENT_SUDAV, event_callback, NULL) == E_NO_ERROR);

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
    (void)rhport;
    printf("DCD - SET ADDR %u\n", dev_addr);

    static const uint8_t ep_addr = 0x80;

    MXC_ASSERT(MXC_USB_SetFuncAddr(dev_addr) == E_NO_ERROR);
    MXC_USBHS->csr0 |= MXC_F_USBHS_CSR0_SERV_OUTPKTRDY;
    while (MXC_USBHS->incsrl & MXC_F_USBHS_INCSRL_INPKTRDY)
        ;

    dcd_edpt_xfer(rhport, ep_addr, NULL, 0);
    while (!write_done)
        ;
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

static void write_callback(void *cbdata)
{
    (void)cbdata;
    MXC_ASSERT(!(MXC_USBHS->incsrl & MXC_F_USBHS_INCSRL_INPKTRDY));
    write_done = true;
    puts("WRITE DONE");
    dcd_event_xfer_complete(0, writereq.ep | TUSB_DIR_IN_MASK, writereq.actlen, XFER_RESULT_SUCCESS,
                            true);
}

static void read_callback(void *cbdata)
{
    (void)cbdata;

    read_done = true;
    puts("READ DONE");
    dcd_event_xfer_complete(0, readreq.ep, readreq.actlen, XFER_RESULT_SUCCESS, true);
}

bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t *buffer, uint16_t total_bytes)
{
    const uint8_t dir = tu_edpt_dir(ep_addr);
    const uint8_t epnum = tu_edpt_number(ep_addr);

    printf("XFER %s\n", dir == TUSB_DIR_IN ? "IN" : "OUT");

    int32_t err;
    if (dir == TUSB_DIR_IN) {
        write_done = false;
        writereq.data = buffer;
        writereq.reqlen = total_bytes;
        writereq.ep = epnum;

        err = MXC_USB_WriteEndpoint((MXC_USB_Req_t *)&writereq);
    } else {
        read_done = false;
        readreq.data = buffer;
        readreq.ep = epnum;
        readreq.reqlen = total_bytes;

        err = MXC_USB_ReadEndpoint((MXC_USB_Req_t *)&readreq);
    }

    if (err != E_NO_ERROR) {
        printf("Error %d\n", err);
    }

    return err == E_NO_ERROR;
}
void dcd_edpt_close_all(uint8_t rhport) {}

void dcd_int_handler(uint8_t rhport)
{
    MXC_USB_EventHandler();
}
