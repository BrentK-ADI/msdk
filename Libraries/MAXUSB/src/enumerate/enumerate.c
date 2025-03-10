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

#include <string.h>
#include "usb.h"
#include "usb_event.h"
#include "enumerate.h"

/***** Definitions *****/
#define ENUM_DESC_SIZE     10   /* Size of the entire descriptor table */

/* Maximum # of descriptors supported other than device, configuration, other speed, and qualifier */
#define ENUM_NUM_STRINGS   (ENUM_DESC_SIZE - ENUM_DESC_STRING)

/***** File Scope Data *****/

/* Structure to hold callback pointers */
typedef struct {
  int (*fnaddr)(MXC_USB_SetupPkt *, void *);
  void *cbdata;
} callback_t;

/* Hold function pointers for all callbacks */
static callback_t callback[ENUM_NUM_CALLBACKS];

static void (*callback_getdescriptor)(MXC_USB_SetupPkt *, const uint8_t **, uint16_t *) = NULL;

#ifdef __IAR_SYSTEMS_ICC__
#pragma data_alignment = 4
#elif __GNUC__
__attribute__((aligned(4)))
#endif
static uint8_t config_value = 0;
#ifdef __IAR_SYSTEMS_ICC__
#pragma data_alignment = 4
#elif __GNUC__
__attribute__((aligned(4)))
#endif
static uint8_t enum_data[2];
static MXC_USB_Req_t enum_req;

/* Holds descriptor pointers */
const uint8_t *enum_desc_table[ENUM_DESC_SIZE];

/***** Function Prototypes *****/
static int setup(maxusb_event_t event, void *cbdata);
static int std(MXC_USB_SetupPkt *sud);
static int getstatus(MXC_USB_SetupPkt *sud);
static int clearfeature(MXC_USB_SetupPkt *sud);
static int setfeature(MXC_USB_SetupPkt *sud);
static int getdescriptor(MXC_USB_SetupPkt *sud);
static int getconfig(MXC_USB_SetupPkt *sud);
static int setconfig(MXC_USB_SetupPkt *sud);
static int getinterface(MXC_USB_SetupPkt *sud);
static int setinterface(MXC_USB_SetupPkt *sud);
static void status_stage_callback(void *cbdata);

/* Holds the device status for getstatus */
static uint8_t device_status;

/***
 * README: ACK/STALL of SETUP enumeration packets
 *
 * Most request do not require a DATAx stage, as all needed data is contained
 * within the 8-byte SETUP packet. ACK/STALL of these requests are handled
 * by logic at the end of setup(). This is a synchronous execution path.
 *
 * Requests which do have a DATAx stage will return a value > 0,
 * which prevents status() from issuing an ACK/STALL. Instead, the ACK/STALL
 * is generated by the USB request callback which is called when all data is
 * transferred asynchronously.
 *
 */

/* USB request object for enumeration packets requiring IN/OUT data phase */
static const MXC_USB_Req_t enum_req_init = {
  0,                       /* ep */
  NULL,                    /* data */
  0,                       /* reqlen */
  0,                       /* actlen */
  0,                       /* error_code */
  status_stage_callback,   /* callback */
  &enum_req                /* callback data */
};

/******************************************************************************/
static void status_stage_callback(void *cbdata)
{
  MXC_USB_Req_t *req = (MXC_USB_Req_t*)cbdata;

  if (req->error_code == 0) {
    /* Send ACK to Status stage */
    MXC_USB_Ackstat(0);
  } else {
    /* STALL the Status stage */
    MXC_USB_Stall(0);
  }

  /* Clear the request to indicate completion */
  memset(req, 0, sizeof(MXC_USB_Req_t));
}

/******************************************************************************/
int enum_init(void)
{
  memset(callback, 0, sizeof(callback));
  memset(enum_desc_table, 0, sizeof(enum_desc_table));
  device_status = 0;

  /* SETUP data will be handled by setup(), and status_stage_callback uses 
   * enum_req.error_code to ACK/STALL and end of transaction if DATA phase was needed
   */
  return MXC_USB_EventEnable(MAXUSB_EVENT_SUDAV, setup, NULL);
}

/******************************************************************************/
int enum_register_callback(enum_callback_t type, int (*func)(MXC_USB_SetupPkt *indata, void *cbdata), void *cbdata)
{
  if (type >= ENUM_NUM_CALLBACKS) {
    return -1;
  }

  /* Register the callback and data */
  callback[type].fnaddr = func;
  callback[type].cbdata = cbdata;

  return 0;
}

/******************************************************************************/
int enum_query_callback(enum_callback_t type, int (**func)(MXC_USB_SetupPkt *indata, void *cbdata), void **cbdata)
{
  if (type >= ENUM_NUM_CALLBACKS) {
    return -1;
  }

  /* Get the registered callback */
  *func = callback[type].fnaddr;
  *cbdata = callback[type].cbdata;

  return 0;
}

/******************************************************************************/
int enum_register_getdescriptor(void (*func)(MXC_USB_SetupPkt *, const uint8_t **desc, uint16_t *desclen))
{
  /* Register the callback */
  callback_getdescriptor = func;
  return 0;
}

/******************************************************************************/
int enum_query_getdescriptor(void (**func)(MXC_USB_SetupPkt *, const uint8_t **desc, uint16_t *desclen))
{
  /* Get the registered callback */
  *func = callback_getdescriptor;

  return 0;
}

/******************************************************************************/
int enum_register_descriptor(enum_descriptor_t type, const uint8_t *desc, uint8_t index)
{
  if ((type >= ENUM_NUM_DESCRIPTORS) || (index >= ENUM_NUM_STRINGS)) {
    return -1;
  }

  if (type < ENUM_DESC_STRING) {
    /* Ignore index. Force to zero. */
    index = 0;
  }

  enum_desc_table[type+index] = desc;

  if (type == ENUM_DESC_CONFIG) {
    if (((MXC_USB_configuration_descriptor_t*)(enum_desc_table[ENUM_DESC_CONFIG]))->bmAttributes & BMATT_SELF_POWERED) {
      device_status |= STATUS_DEV_SELF_POWERED;
    }
  }

  return 0;
}

/******************************************************************************/
static int setup(maxusb_event_t event, void *cbdata)
{
  MXC_USB_SetupPkt sud;
  int result;

  if (MXC_USB_GetSetup(&sud) < 0) {
    return -1;
  }

  if (enum_req.data != NULL) {
    /* ERROR: overlapping SETUP packets */
    result = -1;
  } else {
    /* Decode device requests into standard / class / vendor */
    switch (sud.bmRequestType & RT_TYPE_MASK) {
      case RT_TYPE_STD:
           /* Standard */
           result = std(&sud);
        break;

      case RT_TYPE_CLASS:
            /* Class */
            if (callback[ENUM_CLASS_REQ].fnaddr != NULL) {
              result = callback[ENUM_CLASS_REQ].fnaddr(&sud, callback[ENUM_CLASS_REQ].cbdata);
            } else {
              /* Default class request handler (stall) */
              result = -1;
            }
         break;

      case RT_TYPE_VENDOR:
            /* Vendor */
            if (callback[ENUM_VENDOR_REQ].fnaddr != NULL) {
              result = callback[ENUM_VENDOR_REQ].fnaddr(&sud, callback[ENUM_VENDOR_REQ].cbdata);
            } else {
              /* Default vendor request handler (stall) */
              result = -1;
            }
         break;

      default:
            /* Reserved */
            result = -1;
         break;
    }
  }

  /* If any handler returned > 0, then there is a DATA stage. Do not ACK/STALL here. */
  if (result < 1) {
    /* Result of the SETUP processing will cause either status-stage ACK or STALL */
    if (!result) {
      MXC_USB_Ackstat(0);
    } else {
      MXC_USB_Stall(0);
    }
  }

  return result;
}

/******************************************************************************/
static int std(MXC_USB_SetupPkt *sud)
{
  int result;

  switch (sud->bRequest) {
    case SDR_GET_STATUS:
      result = getstatus(sud);
      break;
    case SDR_CLEAR_FEATURE:
      result = clearfeature(sud);
      break;
    case SDR_SET_FEATURE:
      result = setfeature(sud);
      break;
    case SDR_SET_ADDRESS:
      result = MXC_USB_SetFuncAddr(sud->wValue);
      break;
    case SDR_GET_DESCRIPTOR:
      result = getdescriptor(sud);
      break;
    case SDR_SET_DESCRIPTOR:
      /* Descriptor creation and modification is not implemented */
      result = -1;
      break;
    case SDR_GET_CONFIG:
      result = getconfig(sud);
      break;
    case SDR_SET_CONFIG:
      result = setconfig(sud);
      break;
    case SDR_GET_INTERFACE:
      result = getinterface(sud);
      break;
    case SDR_SET_INTERFACE:
      result = setinterface(sud);
      break;
    case SDR_SYNCH_FRAME:
      /* Isochronous support is not implemented */
      result = -1;
      break;
    default:
      result = -1;
      break;
  }

  return result;
}

/******************************************************************************/
static int getstatus(MXC_USB_SetupPkt *sud)
{
  int result;

  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->wValue != 0) || (sud->wLength != 2) ||
      (((sud->bmRequestType & RT_RECIP_MASK) == RT_RECIP_DEVICE) && (sud->wIndex != 0))) {
    return -1;
  }

  /* Device and Interfaces always return a 16-bit zero value in the data phase */
  if ((sud->bmRequestType & RT_RECIP_MASK) == RT_RECIP_ENDP) {
    /* Interface halt status taken directly from USB SIE registers */
    result = MXC_USB_IsStalled(sud->wIndex & 0x0f);
    if (result < 0) {
      return result;
    }
    enum_data[0] = (result > 0) ? 0x01 : 0x00;
    enum_data[1] = 0;

  } else if ((sud->bmRequestType & RT_RECIP_MASK) == RT_RECIP_IFACE) {
    enum_data[0] = 0;
    enum_data[1] = 0;

  } else if ((sud->bmRequestType & RT_RECIP_MASK) == RT_RECIP_DEVICE) {
    enum_data[0] = device_status;
    enum_data[1] = 0;
  }

  /* Write the resulting data into the endpoint and arm it */
  memcpy(&enum_req, &enum_req_init, sizeof(MXC_USB_Req_t));
  enum_req.data = enum_data;
  enum_req.reqlen = 2;
  result = MXC_USB_WriteEndpoint(&enum_req);
  if (!result) {
    /* Cause the status stage to be delayed */
    result = 1;
  }

  return result;
}

/******************************************************************************/
static int clearfeature(MXC_USB_SetupPkt *sud)
{
  int result;

  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->bmRequestType & RT_DEV_TO_HOST) ||
      (sud->wLength != 0) ||
      ((sud->wValue == FEAT_ENDPOINT_HALT) && ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_ENDP)) ||
      ((sud->wValue == FEAT_REMOTE_WAKE) && ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE))) {
      return -1;
  }

  if ((sud->wValue == FEAT_ENDPOINT_HALT) && (sud->wIndex > 0)) {
    /* Unhalt endpoint */
    result = MXC_USB_Unstall(sud->wIndex & 0x0f);
    if (result < 0) {
      return result;
    }
  } else if ((sud->wValue == FEAT_REMOTE_WAKE)) {
    /* Clear the remote wakeup feature */
    if (callback[ENUM_CLRFEATURE].fnaddr != NULL) {
      result = callback[ENUM_CLRFEATURE].fnaddr(sud, NULL);
      if (result < 0) {
        return result;
      }
      device_status &= ~STATUS_DEV_REMOTE_WAKE;
    } else {
      return -1;
    }
  } else if ((sud->wValue == FEAT_TEST_MODE)) {
    /* Clear the test mode feature */
    if (callback[ENUM_CLRFEATURE].fnaddr != NULL) {
      result = callback[ENUM_CLRFEATURE].fnaddr(sud, NULL);
      if (result < 0) {
        return result;
      }
  } else {
      return -1;
    }
  } else {
    /* Unsupported */
    return -1;
  }

  return 0;
}

/******************************************************************************/
static int setfeature(MXC_USB_SetupPkt *sud)
{
  int result;

  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->wLength !=0) ||
      ((sud->wValue == FEAT_ENDPOINT_HALT) && ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_ENDP)) ||
      ((sud->wValue == FEAT_REMOTE_WAKE) && ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE)) ||
      ((sud->wValue == FEAT_TEST_MODE) && ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE))) {
    return -1;
  }

  if ((sud->wValue == FEAT_ENDPOINT_HALT) && (sud->wIndex > 0)){
    /* Halt endpoint */
    result = MXC_USB_Stall(sud->wIndex & 0x0f);
    if (result < 0) {
      return result;
    }
  } else if ((sud->wValue == FEAT_REMOTE_WAKE)) {
    /* Clear the remote wakeup feature */
    if (callback[ENUM_SETFEATURE].fnaddr != NULL) {
      result = callback[ENUM_SETFEATURE].fnaddr(sud, NULL);
      if (result < 0) {
        return result;
      }
      device_status |= STATUS_DEV_REMOTE_WAKE;
    } else {
      return -1;
    }
  } else if ((sud->wValue == FEAT_TEST_MODE)) {
    /* Clear the test mode feature */
    if (callback[ENUM_SETFEATURE].fnaddr != NULL) {
      result = callback[ENUM_SETFEATURE].fnaddr(sud, NULL);
      if (result < 0) {
        return result;
      }
    } else {
      return -1;
    }
  } else {
    /* Unsupported */
    return -1;
  }

  return 0;
}

/******************************************************************************/
static int getdescriptor(MXC_USB_SetupPkt *sud)
{
  int result;
  const uint8_t *dsc;
  uint16_t dsclen;

  /* Default to stall */
  dsc = NULL;
  dsclen = 0;

  switch (sud->wValue >> 8) {
    case DESC_DEVICE:
      if (((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE) ||
          (sud->wIndex != 0)) {
        /* Stall */
        dsclen = 0;
      } else {
        dsc = enum_desc_table[ENUM_DESC_DEVICE];
        if (dsc == NULL) {
          dsclen = 0;
        } else {
          dsclen = dsc[0];
        }
      }
      break;

    case DESC_CONFIG:
      if (((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE) ||
          (sud->wIndex != 0)) {
        /* Stall */
        dsclen = 0;
      } else {
        dsc = enum_desc_table[ENUM_DESC_CONFIG];
        if (dsc == NULL) {
          dsclen = 0;
        } else {
          dsclen = (dsc[3] << 8) + dsc[2];
        }
      }
      break;

    case DESC_STRING:
      if ((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE) {
        /* Let the class-specific callback handle it */
        goto default_handler;
      } else {
        if ((sud->wValue & 0xff) < ENUM_NUM_STRINGS) {
          dsc = enum_desc_table[ENUM_DESC_STRING + (sud->wValue & 0xff)];
          dsclen = dsc[0];
        } else {
          /* Host asked for an out-of-range string descriptor */
          dsclen = 0;
        }
      }
      break;

    case DESC_DEVICE_QUAL:
      if (((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE) ||
          (sud->wIndex != 0)) {
        /* Stall */
        dsclen = 0;
      } else {
        dsc = enum_desc_table[ENUM_DESC_QUAL];
        if (dsc == NULL) {
          dsclen = 0;
        } else {
          //dsclen = (dsc[3] << 8) + dsc[2];
          dsclen = dsc[0];
        }
      }
      break;

    case DESC_OTHER_SPEED:
      if (((sud->bmRequestType & RT_RECIP_MASK) != RT_RECIP_DEVICE) ||
          (sud->wIndex != 0)) {
        /* Stall */
        dsclen = 0;
      } else {
        dsc = enum_desc_table[ENUM_DESC_OTHER];
        if (dsc == NULL) {
          dsclen = 0;
        } else {
          dsclen = (dsc[3] << 8) + dsc[2];
        }
      }
      break;

    default:
    default_handler:
      /* If a class-specific handler is registered, pass this request off to it */
      if (callback_getdescriptor != NULL) {
        callback_getdescriptor(sud, &dsc, &dsclen);
        if (dsc == NULL) {
          dsclen = 0;
        }
      } else {
        /* Stall */
        dsclen = 0;
      }
      break;
  }

  /* Adjust dsclen if descriptor is longer than the requested number of bytes */
  if (dsclen > sud->wLength) {
    dsclen = sud->wLength;
  }

  if (!dsclen) {
    result = -1;
  } else {
    /* Write data into endpoint buffer */
    memcpy(&enum_req, &enum_req_init, sizeof(MXC_USB_Req_t));
    enum_req.data = (uint8_t*)dsc;
    enum_req.reqlen = dsclen;
    result = MXC_USB_WriteEndpoint(&enum_req);
  }

  if (!result) {
    /* Cause the status stage to be delayed */
    result = 1;
  }

  return result;
}

/******************************************************************************/
static int getconfig(MXC_USB_SetupPkt *sud)
{
  int result;

  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->wValue != 0) || (sud->wIndex != 0) || (sud->wLength != 1)) {
    return -1;
  }

  /* Write the resulting data into the endpoint and arm it */
  memcpy(&enum_req, &enum_req_init, sizeof(MXC_USB_Req_t));
  enum_req.data = &config_value;
  enum_req.reqlen = 1;
  result = MXC_USB_WriteEndpoint(&enum_req);

  if (!result) {
    /* Cause the status stage to be delayed */
    result = 1;
  }

  return result;
}

/******************************************************************************/
static int setconfig(MXC_USB_SetupPkt *sud)
{
  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->wIndex != 0) || (sud->wLength != 0)) {
    return -1;
  }

  /* If the callback is set for a configuration value validator, use it */
  /* Otherwise, assume any wValue is acceptable and ACK the Status stage */
  if (callback[ENUM_SETCONFIG].fnaddr != NULL) {
    if (callback[ENUM_SETCONFIG].fnaddr(sud, callback[ENUM_SETCONFIG].cbdata) < 0) {
      /* Validation routine did not accept value, stall */
      return -1;
    }
  }

  /* Save off configuration value */
  config_value = sud->wValue;

  return 0;
}

/******************************************************************************/
static int getinterface(MXC_USB_SetupPkt *sud)
{
  int result;

  /* If any values are out of range (per USB 2.0 spec), stall */
  if ((sud->wValue != 0) || (sud->wLength != 1)) {
    return -1;
  }

  /* Check for application-specific handler */
  if (callback[ENUM_GETINTERFACE].fnaddr != NULL) {
    result = callback[ENUM_GETINTERFACE].fnaddr(sud, callback[ENUM_GETINTERFACE].cbdata);
    if (result < 0) {
      return result;
    }
    /* Lower 8 bits from the callback are passed to host */
    enum_data[0] = (uint8_t)result;
  } else {
    enum_data[0] = 0;
  }

  /* If the default handler is used, wIndex is ignored, and 0x00 is returned */
  /*  during the Data phase */
  memcpy(&enum_req, &enum_req_init, sizeof(MXC_USB_Req_t));
  enum_req.data = enum_data;
  enum_req.reqlen = 1;
  result = MXC_USB_WriteEndpoint(&enum_req);

  if (!result) {
    /* Cause the status stage to be delayed */
    result = 1;
  }

  return result;
}

/******************************************************************************/
static int setinterface(MXC_USB_SetupPkt *sud)
{
  /* If any values are out of range (per USB 2.0 spec), stall */
  if (sud->wLength != 0) {
    return -1;
  }

  /* Check for application-specific handler */
  if (callback[ENUM_SETINTERFACE].fnaddr != NULL) {
    if (callback[ENUM_SETINTERFACE].fnaddr(sud, callback[ENUM_SETINTERFACE].cbdata) < 0) {
      return -1;
    }
  } else {
    /* If the default handler is used, wIndex is ignored, and wValue must be */
    /*  zero (no alternate setting) */
    if (sud->wValue != 0) {
      return -1;
    }
  }

  return 0;
}

/******************************************************************************/
uint8_t enum_getconfig(void)
{
  return config_value;
}

/******************************************************************************/
void enum_clearconfig(void)
{
  config_value = 0;
}
