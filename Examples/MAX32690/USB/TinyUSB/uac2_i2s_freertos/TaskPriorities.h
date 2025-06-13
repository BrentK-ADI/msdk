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
#ifndef EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_TASKPRIORITIES_H_
#define EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_TASKPRIORITIES_H_

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#define TASK_PRIO_BACKGROUND (tskIDLE_PRIORITY + 1)
#define TASK_PRIO_LOGGING (TASK_PRIO_BACKGROUND + 1)
#define TASK_PRIO_I2S (TASK_PRIO_LOGGING + 1)
#define TASK_PRIO_USBD (TASK_PRIO_I2S + 1)

#endif // EXAMPLES_MAX32690_USB_TINYUSB_UAC2_I2S_FREERTOS_TASKPRIORITIES_H_
