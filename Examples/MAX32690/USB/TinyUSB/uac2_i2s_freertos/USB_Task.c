/******************************************************************************
 *
 * Copyright (C) 2025 Analog Devices, Inc.
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
#include "USB_Task.h"
#include "I2S_Task.h"
#include "TaskPriorities.h"
#include "Logging.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "mxc_device.h"
#include "gcr_regs.h"
#include "mcr_regs.h"
#include "mxc_sys.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#define USBD_STACK_SIZE (4 * configMINIMAL_STACK_SIZE / 2) * (CFG_TUSB_DEBUG ? 2 : 1)
#define TX_BLOCK_SIZE (CFG_TUD_AUDIO_EP_SZ_IN - 2)

static StreamBufferHandle_t dataStreamBuff;
static TaskHandle_t taskHandle;

// Used to transition from streambuff to USB. Kept off the stack
static uint8_t txBuffer[TX_BLOCK_SIZE];

// Range states
static audio_control_range_4_n_t(1) sampleFreqRng; // Sample freq

// Audio controls
// Current states. Not used yet, information only
static bool mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
static uint16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1]; // +1 for master channel 0
static uint32_t sampFreq;
static uint8_t clkValid;

static void USB_TaskBody(void *param);

void USB_TaskInit(StreamBufferHandle_t audioStreamBuff)
{
    dataStreamBuff = audioStreamBuff;

    board_init();

    //Setup the control structures.
    sampFreq = CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE;
    clkValid = 1;
    sampleFreqRng.wNumSubRanges = 1;
    sampleFreqRng.subrange[0].bMin = CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bMax = CFG_TUD_AUDIO_FUNC_1_SAMPLE_RATE;
    sampleFreqRng.subrange[0].bRes = 0;

    xTaskCreate(USB_TaskBody, "USBD", USBD_STACK_SIZE, NULL, TASK_PRIO_USBD, &taskHandle);
}

void USB_TaskBody(void *param)
{
    tud_init(BOARD_TUD_RHPORT);
    if (board_init_after_tusb) {
        board_init_after_tusb();
    }

    while (1) {
        // tinyusb device task
        tud_task();
    }
}

/**
 * IMPORTANT: This is the callback from the stack that is used to push more
 * data into the USB stack.  This implementation leverages a streambuffer from
 * the I2S Task to shuttle data across. Do a check at the start to see if
 * enough is available, and if not, just send 0s. This strategy gives the I2S
 * time to fill the stream buffer when the USB EP is first opened.
 */
bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in,
                                   uint8_t cur_alt_setting)
{
    BaseType_t higherPriorityTask;
    if (xStreamBufferBytesAvailable(dataStreamBuff) < TX_BLOCK_SIZE) {
        //Data underflow. Just 0 out.
        memset(txBuffer, 0, TX_BLOCK_SIZE);
    } else {
        xStreamBufferReceiveFromISR(dataStreamBuff, txBuffer, TX_BLOCK_SIZE, &higherPriorityTask);
    }
    tud_audio_write(txBuffer, TX_BLOCK_SIZE);
    return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
    uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));
    if ((itf == 1) && (alt != 0)) {
        // Audio streaming start
        I2S_TaskStartStream();
    }

    return true;
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    //Stop the stream
    I2S_TaskStopStream();
    return true;
}

// Invoked when audio class specific set request received for an EP
bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request,
                             uint8_t *pBuff)
{
    return false; // Yet not implemented
}

// Invoked when audio class specific set request received for an interface
bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request,
                              uint8_t *pBuff)
{
    return false; // Yet not implemented
}

// Invoked when audio class specific set request received for an entity
// TODO(BrentK-ADI): Have the volume and mute control the CODEC and or I2S processing
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request,
                                 uint8_t *pBuff)
{
    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    //uint8_t itf = TU_U16_LOW(p_request->wIndex);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);

    // We do not support any set range requests here, only current value requests
    TU_VERIFY(p_request->bRequest == AUDIO_CS_REQ_CUR);

    // If request is for our feature unit
    if (entityID == 2) {
        switch (ctrlSel) {
        case AUDIO_FU_CTRL_MUTE:
            mute[channelNum] = ((audio_control_cur_1_t *)pBuff)->bCur;
            LOG_MSG_INFO(USBD, "Set Mute: %d of channel: %u", mute[channelNum], channelNum);
            return true;
        case AUDIO_FU_CTRL_VOLUME:
            volume[channelNum] = (uint16_t)((audio_control_cur_2_t *)pBuff)->bCur;
            LOG_MSG_INFO(USBD, "Set Volume: %d dB of channel: %u", volume[channelNum], channelNum);
            return true;
        default: // Unknown/Unsupported control
            return false;
        }
    }
    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an EP
bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an interface
bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    return false; // Yet not implemented
}

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    // Page 91 in UAC2 specification
    uint8_t channelNum = TU_U16_LOW(p_request->wValue);
    uint8_t ctrlSel = TU_U16_HIGH(p_request->wValue);
    uint8_t entityID = TU_U16_HIGH(p_request->wIndex);
    audio_desc_channel_cluster_t ret;

    // Input terminal (Microphone input)
    if (entityID == 1) {
        switch (ctrlSel) {
        case AUDIO_TE_CTRL_CONNECTOR:
            // Those are dummy values for now
            ret.bNrChannels = 1;
            ret.bmChannelConfig = (audio_channel_config_t)0;
            ret.iChannelNames = 0;

            LOG_MSG_INFO0(USBD, "Get terminal connector");

            return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret,
                                                              sizeof(ret));
        default: // Unknown/Unsupported control selector
            return false;
        }
    }

    // Feature unit
    if (entityID == 2) {
        switch (ctrlSel) {
        case AUDIO_FU_CTRL_MUTE:
            // Audio control mute cur parameter block consists of only one byte - we thus can send it right away
            // There does not exist a range parameter block for mute
            LOG_MSG_INFO(USBD, "Get Mute of channel: %u", channelNum);
            return tud_control_xfer(rhport, p_request, &mute[channelNum], 1);

        case AUDIO_FU_CTRL_VOLUME:
            switch (p_request->bRequest) {
            case AUDIO_CS_REQ_CUR:
                LOG_MSG_INFO(USBD, "Get Volume of channel: %u", channelNum);
                return tud_control_xfer(rhport, p_request, &volume[channelNum],
                                        sizeof(volume[channelNum]));

            case AUDIO_CS_REQ_RANGE:
                LOG_MSG_INFO(USBD, "Get Volume range of channel: %u", channelNum);

                // Copy values - only for testing - better is version below
                audio_control_range_2_n_t(1) ret;

                ret.wNumSubRanges = 1;
                ret.subrange[0].bMin = -90; // -90 dB
                ret.subrange[0].bMax = 90; // +90 dB
                ret.subrange[0].bRes = 1; // 1 dB steps

                return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, (void *)&ret,
                                                                  sizeof(ret));
            default: // Unknown/Unsupported control
                return false;
            }
            break;
        default: // Unknown/Unsupported control
            return false;
        }
    }

    // Clock Source unit
    if (entityID == 4) {
        switch (ctrlSel) {
        case AUDIO_CS_CTRL_SAM_FREQ:
            // channelNum is always zero in this case
            switch (p_request->bRequest) {
            case AUDIO_CS_REQ_CUR:
                LOG_MSG_INFO0(USBD, "Get Sample Freq.");
                return tud_control_xfer(rhport, p_request, &sampFreq, sizeof(sampFreq));

            case AUDIO_CS_REQ_RANGE:
                LOG_MSG_INFO0(USBD, "Get Sample Freq. range");
                return tud_control_xfer(rhport, p_request, &sampleFreqRng, sizeof(sampleFreqRng));
            default: // Unknown/Unsupported control
                return false;
            }
            break;

        case AUDIO_CS_CTRL_CLK_VALID:
            // Only cur attribute exists for this request
            LOG_MSG_INFO0(USBD, "Get Sample Freq. valid");
            return tud_control_xfer(rhport, p_request, &clkValid, sizeof(clkValid));
        default: // Unknown/Unsupported control
            return false;
        }
    }

    LOG_MSG_INFO(USBD, "Unsupported entity: %d", entityID);
    return false; // Yet not implemented
}

bool tud_audio_tx_done_post_load_cb(uint8_t rhport, uint16_t n_bytes_copied, uint8_t itf,
                                    uint8_t ep_in, uint8_t cur_alt_setting)
{
    //Nothing to do here yet
    return true;
}

//------------------------------------------------------------------------------
// Wrapper functions to bridge TinyUSB BSP with MSDK BSP
//------------------------------------------------------------------------------
void board_init(void)
{
    NVIC_SetPriority(USB_IRQn, 7);

    MXC_SYS_ClockSourceEnable(MXC_SYS_CLOCK_IPO);
    MXC_MCR->ldoctrl |= MXC_F_MCR_LDOCTRL_0P9EN;
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_USB);
    MXC_SYS_Reset_Periph(MXC_SYS_RESET0_USB);
}

//--------------------------------------------------------------------+
// Forward USB interrupt events to TinyUSB IRQ Handler
//--------------------------------------------------------------------+
void USB_IRQHandler(void)
{
    tud_int_handler(0);
}
