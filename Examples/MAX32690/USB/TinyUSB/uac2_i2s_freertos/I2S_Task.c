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
#include "I2S_Task.h"
#include "Logging.h"
#include "TaskPriorities.h"

#include "mxc_device.h"
#include "gcr_regs.h"
#include "mcr_regs.h"
#include "mxc_sys.h"
#include "i2s.h"
#include "dma.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "queue.h"

/* These are somewhat arbitrary, but work. Adjust if necessary */
#define NUM_QUEUE_ITEMS 5
#define I2S_BUFF_SIZE 1024

/* Represents a DMA I2S Data buffer transaction */
typedef struct {
    uint16_t data[I2S_BUFF_SIZE];
    //Optional room to do other stuff in this struct
} i2s_buffer_t;

static StreamBufferHandle_t dataStreamBuff;
static TaskHandle_t taskHandle;
static QueueHandle_t emptyQueue;
static QueueHandle_t fullQueue;

static mxc_i2s_req_t i2s_req; /**< I2S Request instance */
static int rxChannelID = -1; /**< DMA Channel for Rx */
static uint32_t dummybuffer; /**< Needed for I2S init */

static i2s_buffer_t bufferPool[NUM_QUEUE_ITEMS];
static i2s_buffer_t *volatile activeBuffer;
static i2s_buffer_t *volatile reloadBuffer;

static bool streamRunning = false;

static void I2S_TaskBody(void *param);
static void I2S_Init(void);
static void I2S_DMA_Callback(int ch, int error);
static void I2S_Reload(uint16_t *reloadBuffer, uint32_t bufferSizeSamples);

void I2S_TaskInit(StreamBufferHandle_t audioStreamBuff)
{
    int i;
    i2s_buffer_t *bufferPtr;

    emptyQueue = xQueueCreate(NUM_QUEUE_ITEMS, sizeof(i2s_buffer_t *));
    fullQueue = xQueueCreate(NUM_QUEUE_ITEMS, sizeof(i2s_buffer_t *));

    //Prime the empty queue
    for (i = 0; i < NUM_QUEUE_ITEMS; i++) {
        bufferPtr = &bufferPool[i];
        xQueueSend(emptyQueue, &bufferPtr, 0);
    }
    dataStreamBuff = audioStreamBuff;

    I2S_Init();
    xTaskCreate(I2S_TaskBody, "I2S", 512, NULL, TASK_PRIO_I2S, &taskHandle);
}

void I2S_TaskBody(void *param)
{
    i2s_buffer_t *qData;
    bool lastState = false;
    while (1) {
        if (xQueueReceive(fullQueue, &qData, portMAX_DELAY) == pdTRUE) {
            //Simple on/off logic. If on, load into the stream buffer. if
            //transitioning to on, flush to give a clean slate
            if (streamRunning) {
                if (lastState == false) {
                    xStreamBufferReset(dataStreamBuff);
                    lastState = streamRunning;
                }
                xStreamBufferSend(dataStreamBuff, qData->data, I2S_BUFF_SIZE * sizeof(uint16_t),
                                  portMAX_DELAY);
            }
            xQueueSend(emptyQueue, &qData, portMAX_DELAY);
        }
    }
}

/**
 * Initializes the I2S peripheral and starts the DMA Transactions. The DMA
 * makes use of the reload feature to constantly have buffers being filled and
 * data being continuous.
 */
void I2S_Init()
{
    i2s_req.wordSize = MXC_I2S_DATASIZE_HALFWORD;
    i2s_req.sampleSize = MXC_I2S_SAMPLESIZE_SIXTEEN;
    i2s_req.bitsWord = 16;
    i2s_req.justify = MXC_I2S_MSB_JUSTIFY;
    i2s_req.wsPolarity = MXC_I2S_POL_NORMAL;
    i2s_req.channelMode = MXC_I2S_EXTERNAL_SCK_EXTERNAL_WS;
    i2s_req.stereoMode = MXC_I2S_MONO_LEFT_CH;
    i2s_req.bitOrder = MXC_I2S_MSB_FIRST;

    i2s_req.rawData = NULL;
    i2s_req.txData = (void *)&dummybuffer;

    //Init requires _something_ here to not fail, but wont get used since we are using DMA
    i2s_req.rxData = (void *)&dummybuffer;
    i2s_req.length = 1;
    MXC_I2S_Init(&i2s_req);

    MXC_I2S_RegisterDMACallback(I2S_DMA_Callback);

    //Grab the first 2 buffers
    if ((xQueueReceive(emptyQueue, (void *)&activeBuffer, 0) != pdTRUE) ||
        (xQueueReceive(emptyQueue, (void *)&reloadBuffer, 0) != pdTRUE)) {
        LOG_MSG_ERR0(I2S, "Fatal Error. No initial I2S buffers");
        return;
    }

    //Start transferring
    rxChannelID = MXC_I2S_RXDMAConfig((void *)activeBuffer->data, I2S_BUFF_SIZE * sizeof(uint16_t));

    //And do the first reload
    I2S_Reload(reloadBuffer->data, I2S_BUFF_SIZE);
}

/**
 * Sets the DMA up for the next (reload) transfer
 * @param reloadBuffer - Sample buffer to set
 * @param bufferSizeSample - Number of _samples_ to configure
 */
void I2S_Reload(uint16_t *reloadBuffer, uint32_t bufferSizeSamples)
{
    mxc_dma_srcdst_t srcdst;
    srcdst.ch = rxChannelID;
    srcdst.dest = (void *)reloadBuffer;
    srcdst.len = bufferSizeSamples * sizeof(uint16_t);
    MXC_DMA_SetSrcReload(srcdst);
}

/**
 * Callback from DMA notifying the I2S data is loaded up. The strategy here is
 * to minimize how much work is done in the ISR. So push the buffer to the
 * full queue for processing by the task, and setup the reload. Thats it.
 */
void I2S_DMA_Callback(int ch, int error)
{
    BaseType_t higherTaskWoken;
    i2s_buffer_t *nextBuff;
    i2s_buffer_t *tempBuff;
    if (ch == rxChannelID) {
        if (xQueueReceiveFromISR(emptyQueue, &nextBuff, &higherTaskWoken) == pdTRUE) {
            //Play musical buffer pointers
            tempBuff = activeBuffer;
            activeBuffer = reloadBuffer;
            reloadBuffer = nextBuff;
            I2S_Reload(reloadBuffer->data, I2S_BUFF_SIZE);

            //TODO(BrentK-ADI): check for failures
            xQueueSendFromISR(fullQueue, &tempBuff, &higherTaskWoken);
        } else {
            //Buffer underflow. No empty buffers available. Reuse the current
            if (activeBuffer != reloadBuffer) {
                //If active and reload aren't the same, can push on the queue.
                //TODO(BrentK-ADI): Check for failures.
                xQueueSendFromISR(fullQueue, (void *)&activeBuffer, &higherTaskWoken);
            }
            //Active and reload buffers are the same
            activeBuffer = reloadBuffer;

            //Keep pushing the reload until we're no longer underflowing
            I2S_Reload(reloadBuffer->data, I2S_BUFF_SIZE);
        }
    } else {
        //Error, unexpected
    }
}

void I2S_TaskStartStream()
{
    streamRunning = true;
}

void I2S_TaskStopStream()
{
    streamRunning = false;
}
