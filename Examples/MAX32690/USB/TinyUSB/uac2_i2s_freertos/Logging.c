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
#include <stdio.h>
#include <stdarg.h>

#include "Logging.h"

#include "TaskPriorities.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "mxc_device.h"
#include "mxc_errors.h"
#include "uart.h"

#ifndef LOGGING_BAUD
#define LOGGING_BAUD 115200
#endif

/** Define the number of buffers for strings and their size.  Tune these based
 * on need and memory requirements */
#ifndef LOGGING_CONSOLE_NUM_BUFFS
#define LOGGING_CONSOLE_NUM_BUFFS 50
#endif

#ifndef LOGGING_CONSOLE_BUF_SIZE
#define LOGGING_CONSOLE_BUF_SIZE 256
#endif

/* List of the current levels */
static log_level_t sourceLevels[LOG_SOURCE_COUNT];

/** Define the memory buffer for console data */
static char logBuffers[LOGGING_CONSOLE_NUM_BUFFS * LOGGING_CONSOLE_BUF_SIZE];

static mxc_uart_regs_t *loggingUart = MXC_UART_GET_UART(LOGGING_UART);

static TaskHandle_t taskHandle; /**< Console task handle              */
static QueueHandle_t buffEmptyQ; /**< Queue for empty buffers          */
static QueueHandle_t buffFullQ; /**< Queue for waiting to be printed  */

/** Prototypes **/
static void LoggingTaskBody(void *pvParameters);

int LoggingInit()
{
    BaseType_t xReturned;
    int i;
    char *bufPtr;

    //Set all the sources to the configured level to start
    for (i = 0; i < LOG_SOURCE_COUNT; i++) {
        sourceLevels[i] = GLOBAL_LOG_LEVEL;
    }

    //Initialize the hardware layer
    MXC_UART_Init(loggingUart, LOGGING_BAUD, MXC_UART_APB_CLK);

    //Create the queues
    buffEmptyQ = xQueueCreate(LOGGING_CONSOLE_NUM_BUFFS, sizeof(char *));
    buffFullQ = xQueueCreate(LOGGING_CONSOLE_NUM_BUFFS, sizeof(char *));

    //Fill the empty queue with all the buffer pointers
    for (i = 0; i < LOGGING_CONSOLE_NUM_BUFFS; i++) {
        bufPtr = &(logBuffers[i * LOGGING_CONSOLE_BUF_SIZE]);
        xQueueSend(buffEmptyQ, &bufPtr, 0);
    }

    //Create the task
    xReturned = xTaskCreate(LoggingTaskBody, (const char *)"Logging", configMINIMAL_STACK_SIZE,
                            NULL, TASK_PRIO_LOGGING, &taskHandle);

    if (xReturned == pdPASS) {
        return 0;
    } else {
        return -1;
    }
}

/**
 *  Actual console task body. Just loop, blocking waiting for messages to display
 */
void LoggingTaskBody(void *pvParameters)
{
    int keepRunning = 1;
    char *bufPtr;
    char *wrPtr;
    while (keepRunning) {
        xQueueReceive(buffFullQ, &bufPtr, portMAX_DELAY);
        wrPtr = bufPtr;
        /* In theory the console task is a low priority so sitting and blocking
        on this is fine. TODO in the future, use DMA or interrupt
        */
        while (*wrPtr != '\0') {
            MXC_UART_WriteCharacter(loggingUart, *wrPtr++);
        }
        xQueueSend(buffEmptyQ, &bufPtr, portMAX_DELAY);
    }
}

int LoggingPrint(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    LoggingvPrint(fmt, args);
    va_end(args);
    return 0;
}

void LoggingvPrint(const char *fmt, va_list args)
{
    char *bufPtr;
    BaseType_t xReturned;

    xReturned = xQueueReceive(buffEmptyQ, &bufPtr, 0);
    if (xReturned == pdPASS) {
        vsnprintf(bufPtr, LOGGING_CONSOLE_BUF_SIZE, fmt, args);
        xQueueSend(buffFullQ, &bufPtr, 0);
    }
}

log_level_t LoggingGetSourceLevel(log_source_t src)
{
    if (src < LOG_SOURCE_COUNT) {
        return sourceLevels[src];
    } else {
        return LOG_LEVEL_NONE;
    }
}

void LoggingSetSourceLevel(log_source_t src, log_level_t level)
{
    if ((src < LOG_SOURCE_COUNT) && (level <= GLOBAL_LOG_LEVEL)) {
        sourceLevels[src] = level;
    }
}
