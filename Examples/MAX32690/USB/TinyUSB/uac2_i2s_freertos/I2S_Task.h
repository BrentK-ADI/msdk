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
#ifndef EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_I2S_TASK_H_
#define EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_I2S_TASK_H_

#include "FreeRTOS.h"
#include "stream_buffer.h"

/**
 * Initializes the I2S task and immediately starts the I2S DMA. Data will not be
 * pushed to the stream buffer until I2S_TaskStartStream is called
 * @param audioStreamBuf - Streambuffer to push audio data to
 */
void I2S_TaskInit(StreamBufferHandle_t audioStreamBuf);

/**
 * Enables the task to push data to the stream buffer
 */
void I2S_TaskStartStream(void);

/**
 * Stops the task from pushing data to the stream buffer
 */
void I2S_TaskStopStream(void);

#endif // EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_I2S_TASK_H_
