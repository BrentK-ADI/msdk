/******************************************************************************
 *
 * Copyright (C) 2022-2023 Maxim Integrated Products, Inc. (now owned by 
 * Analog Devices, Inc.),
 * Copyright (C) 2023-2024 Analog Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

/**
  * @file main.c
  * @brief USB composite device with Mass Storage driver class and HID driver class.
*/

#include <stdio.h>
#include <stddef.h>
#include "led.h"
#include "pb.h"
#include "mcr_regs.h"
#include "mxc_delay.h"
#include "mxc_sys.h"
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"
#include "hid_kbd.h"
#include "msc.h"
#include "descriptors.h"
#include "mscmem.h"

/* **** Definitions **** */
#define EVENT_ENUM_COMP MAXUSB_NUM_EVENTS
#define EVENT_REMOTE_WAKE (EVENT_ENUM_COMP + 1)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

/* **** Global Data **** */
volatile int configured;
volatile int suspended;
volatile unsigned int event_flags;
int remote_wake_en;

/* This EP assignment must match the Configuration Descriptor */
static msc_cfg_t msc_cfg = {
    1, /* EP OUT */
    MXC_USBHS_MAX_PACKET, /* OUT max packet size */
    2, /* EP IN */
    MXC_USBHS_MAX_PACKET, /* IN max packet size */
};

static const msc_idstrings_t ids = {
    "MAXIM", /* Vendor string.  Maximum of 8 bytes */
    "MSC Example", /* Product string.  Maximum of 16 bytes */
    "1.0" /* Version string.  Maximum of 4 bytes */
};

/* Functions to control "disk" memory. See msc.h for definitions. */
static const msc_mem_t mem = {
    mscmem_Init, mscmem_Start, mscmem_Stop, mscmem_Ready, mscmem_Size, mscmem_Read, mscmem_Write,
};

/* **** Function Prototypes **** */
static int setconfigCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int setfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int clrfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata);
static int eventCallback(maxusb_event_t evt, void *data);
static void usbAppSleep(void);
static void usbAppWakeup(void);
static void buttonCallback(void *pb);
int usbStartupCallback(void);
int usbShutdownCallback(void);

/**
 * User-supplied function to delay usec micro-seconds
 *
 * @param[in]  usec  The usec time to delay.
 */
void delay_us(unsigned int usec)
{
    /* mxc_delay() takes unsigned long, so can't use it directly */
    MXC_Delay(usec);
}

/* ************************************************************************** */

void USB_IRQHandler(void)
{
    MXC_USB_EventHandler();
}

/* ************************************************************************** */
int main(void)
{
    maxusb_cfg_options_t usb_opts;

    printf("\n\n***** " TOSTRING(
        TARGET) " USB Composite Device (Keyboard and Mass Storage) Example *****\n");
    printf("Waiting for VBUS...\n");

    /* Initialize state */
    configured = 0;
    suspended = 0;
    event_flags = 0;
    remote_wake_en = 0;

    /* Start out in full speed */
    usb_opts.enable_hs = 0;
    usb_opts.delay_us = delay_us; /* Function which will be used for delays */
    usb_opts.init_callback = usbStartupCallback;
    usb_opts.shutdown_callback = usbShutdownCallback;

    /* Initialize the usb module */
    if (MXC_USB_Init(&usb_opts) != 0) {
        printf("usb_init() failed\n");

        while (1) {}
    }

    /* Initialize the enumeration module */
    if (enum_init() != 0) {
        printf("enum_init() failed\n");

        while (1) {}
    }

    /* Register enumeration data */
    enum_register_descriptor(ENUM_DESC_DEVICE, (uint8_t *)&composite_device_descriptor, 0);
    enum_register_descriptor(ENUM_DESC_CONFIG, (uint8_t *)&composite_config_descriptor, 0);
    enum_register_descriptor(ENUM_DESC_STRING, lang_id_desc, 0);
    enum_register_descriptor(ENUM_DESC_STRING, mfg_id_desc, 1);
    enum_register_descriptor(ENUM_DESC_STRING, prod_id_desc, 2);
    enum_register_descriptor(ENUM_DESC_STRING, serial_id_desc, 3);
    enum_register_descriptor(ENUM_DESC_STRING, hidkbd_func_desc, 4);
    enum_register_descriptor(ENUM_DESC_STRING, msc_func_desc, 5);

    /* Handle configuration */
    enum_register_callback(ENUM_SETCONFIG, setconfigCallback, NULL);

    /* Handle feature set/clear */
    enum_register_callback(ENUM_SETFEATURE, setfeatureCallback, NULL);
    enum_register_callback(ENUM_CLRFEATURE, clrfeatureCallback, NULL);

    /* Initialize the class driver */
    if (msc_init(&composite_config_descriptor.msc_interface_descriptor, &ids, &mem) != 0) {
        printf("msc_init() failed\n");

        while (1) {}
    }

    if (hidkbd_init(&composite_config_descriptor.hid_interface_descriptor,
                    &composite_config_descriptor.hid_descriptor, report_descriptor) != 0) {
        printf("hidkbd_init() failed\n");

        while (1) {}
    }

    /* Register callbacks */
    MXC_USB_EventEnable(MAXUSB_EVENT_NOVBUS, eventCallback, NULL);
    MXC_USB_EventEnable(MAXUSB_EVENT_VBUS, eventCallback, NULL);

    /* Register callback for keyboard events */
    if (PB_RegisterCallback(0, buttonCallback) != E_NO_ERROR) {
        printf("PB_RegisterCallback() failed\n");

        while (1) {}
    }

    /* Start with USB in low power mode */
    usbAppSleep();
    NVIC_EnableIRQ(USB_IRQn);

    /* Wait for events */
    while (1) {
        if (suspended || !configured) {
            LED_Off(0);
        } else {
            LED_On(0);
        }

        if (event_flags) {
            /* Display events */
            if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_NOVBUS)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_NOVBUS);
                printf("VBUS Disconnect\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_VBUS)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_VBUS);
                printf("VBUS Connect\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_BRST)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_BRST);
                printf("Bus Reset\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_SUSP)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_SUSP);
                printf("Suspended\n");
            } else if (MXC_GETBIT(&event_flags, MAXUSB_EVENT_DPACT)) {
                MXC_CLRBIT(&event_flags, MAXUSB_EVENT_DPACT);
                printf("Resume\n");
            } else if (MXC_GETBIT(&event_flags, EVENT_ENUM_COMP)) {
                MXC_CLRBIT(&event_flags, EVENT_ENUM_COMP);
                printf("Enumeration complete. Press SW2 to send character.\n");
            } else if (MXC_GETBIT(&event_flags, EVENT_REMOTE_WAKE)) {
                MXC_CLRBIT(&event_flags, EVENT_REMOTE_WAKE);
                printf("Remote Wakeup\n");
            }
        }
    }
}

/******************************************************************************/
int usbStartupCallback(void)
{
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_USB);
    MXC_MCR->ldoctrl |= MXC_F_MCR_LDOCTRL_0P9EN;
    return E_NO_ERROR;
}

/******************************************************************************/
int usbShutdownCallback(void)
{
    MXC_SYS_ClockDisable(MXC_SYS_PERIPH_CLOCK_USB);

    return E_NO_ERROR;
}

/* ************************************************************************** */

static int setfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    if (sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 1;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

/* ************************************************************************** */

static int clrfeatureCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    if (sud->wValue == FEAT_REMOTE_WAKE) {
        remote_wake_en = 0;
    } else {
        // Unknown callback
        return -1;
    }

    return 0;
}

/* ************************************************************************** */

static void usbAppSleep(void)
{
    /* Place low-power code here if application needed */
    suspended = 1;
}

/* ************************************************************************** */

static void usbAppWakeup(void)
{
    /* Place low-power wakeup code here if application needed */
    suspended = 0;
}

/* ************************************************************************** */

static int setconfigCallback(MXC_USB_SetupPkt *sud, void *cbdata)
{
    /* Confirm the configuration value */
    if (sud->wValue == composite_config_descriptor.config_descriptor.bConfigurationValue) {
        //      on++;
        configured = 1;
        MXC_SETBIT(&event_flags, EVENT_ENUM_COMP);
        msc_cfg.out_ep = composite_config_descriptor.endpoint_descriptor_1.bEndpointAddress & 0x7;
        msc_cfg.out_maxpacket = composite_config_descriptor.endpoint_descriptor_1.wMaxPacketSize;
        msc_cfg.in_ep = composite_config_descriptor.endpoint_descriptor_2.bEndpointAddress & 0x7;
        msc_cfg.in_maxpacket = composite_config_descriptor.endpoint_descriptor_2.wMaxPacketSize;

        msc_configure(&msc_cfg);
        return hidkbd_configure(composite_config_descriptor.endpoint_descriptor_3.bEndpointAddress &
                                USB_EP_NUM_MASK);
    } else if (sud->wValue == 0) {
        configured = 0;
        msc_deconfigure();
        return hidkbd_deconfigure();
    }

    return -1;
}

/* ************************************************************************** */

static int eventCallback(maxusb_event_t evt, void *data)
{
    /* Set event flag */
    MXC_SETBIT(&event_flags, evt);

    switch (evt) {
    case MAXUSB_EVENT_NOVBUS:
        MXC_USB_EventDisable(MAXUSB_EVENT_BRST);
        MXC_USB_EventDisable(MAXUSB_EVENT_SUSP);
        MXC_USB_EventDisable(MAXUSB_EVENT_DPACT);
        MXC_USB_Disconnect();
        configured = 0;
        enum_clearconfig();
        hidkbd_deconfigure();
        msc_deconfigure();
        usbAppSleep();
        break;

    case MAXUSB_EVENT_VBUS:
        MXC_USB_EventClear(MAXUSB_EVENT_BRST);
        MXC_USB_EventEnable(MAXUSB_EVENT_BRST, eventCallback, NULL);
        MXC_USB_EventClear(MAXUSB_EVENT_SUSP);
        MXC_USB_EventEnable(MAXUSB_EVENT_SUSP, eventCallback, NULL);
        MXC_USB_Connect();
        usbAppSleep();
        break;

    case MAXUSB_EVENT_BRST:
        usbAppWakeup();
        enum_clearconfig();
        hidkbd_deconfigure();
        msc_deconfigure();
        configured = 0;
        suspended = 0;
        break;

    case MAXUSB_EVENT_SUSP:
        usbAppSleep();
        break;

    case MAXUSB_EVENT_DPACT:
        usbAppWakeup();
        break;

    default:
        break;
    }

    return 0;
}

/* ************************************************************************** */

void buttonCallback(void *pb)
{
    static const uint8_t chars[] = "Maxim Integrated\n";
    static int i = 0;
    int count = 0;
    int button_pressed = 0;

    //determine if interrupt triggered by bounce or a true button press
    while (PB_Get(0) && !button_pressed) {
        count++;

        if (count > 1000) {
            button_pressed = 1;
        }
    }

    if (button_pressed) {
        LED_Toggle(0);

        if (configured) {
            if (suspended && remote_wake_en) {
                /* The bus is suspended. Wake up the host */
                suspended = 0;
                usbAppWakeup();
                MXC_USB_RemoteWakeup();
                MXC_SETBIT(&event_flags, EVENT_REMOTE_WAKE);
            } else {
                if (i >= (sizeof(chars) - 1)) {
                    i = 0;
                }

                hidkbd_keypress(chars[i++]);
            }
        }
    }
}
