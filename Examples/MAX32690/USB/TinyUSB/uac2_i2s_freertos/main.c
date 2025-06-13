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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "TaskPriorities.h"
#include "USB_Task.h"
#include "I2S_Task.h"
#include "Logging.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#include "i2c.h"
#include "i2c_regs.h"
#include "dma.h"
#include "nvic_table.h"

#define CODEC_I2C MXC_I2C2
#define CODEC_I2C_FREQ 100000
#define MAX9867_ADDR 0x18

#define CODEC_MCLOCK 12288000
#define SAMPLE_RATE 48000

static StreamBufferHandle_t dataSB;
static TaskHandle_t backgroundTask;

static void BackgroundTaskBody(void *pvParameters);
static void ConfigureCodec(void);
static void CodecWriteReg(uint8_t reg, uint8_t val);
static uint8_t CodecReadReg(uint8_t reg);
static void CodecUpdateReg(uint8_t reg, uint8_t mask, uint8_t val);

/* Global DMA Handler */
void DMA_Handler(void)
{
    MXC_DMA_Handler();
}

int main(void)
{
    int i;
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO0);
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO1);
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO2);
    MXC_SYS_ClockEnable(MXC_SYS_PERIPH_CLOCK_GPIO3);

    /* Configure all DMA channels. */
    for (i = 0; i < MXC_DMA_CHANNELS; i++) {
        MXC_NVIC_SetVector(MXC_DMA_CH_GET_IRQ(i), DMA_Handler);
        NVIC_SetPriority(MXC_DMA_CH_GET_IRQ(i), 7); //Play nice with FreeRTOS
        NVIC_EnableIRQ(MXC_DMA_CH_GET_IRQ(i));
    }

    xTaskCreate(BackgroundTaskBody, (const char *)"Background", 512, NULL, TASK_PRIO_BACKGROUND,
                &backgroundTask);

    vTaskStartScheduler();

    return 0;
}

void BackgroundTaskBody(void *pvParameters)
{
    LoggingInit();
    dataSB = xStreamBufferCreate(0x4000, 1);
    ConfigureCodec();
    USB_TaskInit(dataSB);
    I2S_TaskInit(dataSB);

    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        LOG_MSG_INFO0(BKGND, "Tick");
    }
}

void CodecUpdateReg(uint8_t reg, uint8_t mask, uint8_t val)
{
    uint8_t tmp;

    tmp = CodecReadReg(reg);
    tmp &= ~mask;
    tmp |= val & mask;

    CodecWriteReg(reg, tmp);
}

void CodecWriteReg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    mxc_i2c_req_t i2c_req;

    i2c_req.i2c = CODEC_I2C;
    i2c_req.addr = MAX9867_ADDR;
    i2c_req.restart = 0;
    i2c_req.callback = (void *)0;
    i2c_req.tx_buf = buf;
    i2c_req.tx_len = sizeof(buf);
    i2c_req.rx_len = 0;

    //The I2C driver is buggy and sometimes returns before the bus is ready.
    //Wait for a ready bus to compensate for that.
    while (CODEC_I2C->status & MXC_F_I2C_STATUS_MST_BUSY) {}
    MXC_I2C_MasterTransaction(&i2c_req);
}

uint8_t CodecReadReg(uint8_t reg)
{
    uint8_t buf[1] = { reg };
    uint8_t dest;
    mxc_i2c_req_t i2c_req;

    i2c_req.i2c = CODEC_I2C;
    i2c_req.addr = MAX9867_ADDR;
    i2c_req.restart = 0;
    i2c_req.callback = (void *)0;
    i2c_req.tx_buf = buf;
    i2c_req.tx_len = sizeof(buf);
    i2c_req.rx_buf = &dest;
    i2c_req.rx_len = 1;

    //The I2C driver is buggy and sometimes returns before the bus is ready.
    //Wait for a ready bus to compensate for that.
    while (CODEC_I2C->status & MXC_F_I2C_STATUS_MST_BUSY) {}
    MXC_I2C_MasterTransaction(&i2c_req);
    return dest;
}

void ConfigureCodec()
{
    uint8_t r;
    if (MXC_I2C_Init(CODEC_I2C, 1, 0) != E_NO_ERROR) {
        LOG_MSG_ERR0(CODEC, "Error starting I2C");
    }
    MXC_I2C_SetFrequency(CODEC_I2C, CODEC_I2C_FREQ);
    CodecWriteReg(0x17, 0x00); //Shutdown for configuration

    for (r = 0x4; r < 0x17; r++) {
        //Clear all regs to POR
        CodecWriteReg(0x17, 0x00);
    }

    CodecWriteReg(0x05, 0x1 << 4); //Prescaler for 12.2MHz clock
    CodecWriteReg(0x06, 0x60); //NI=0x6000, giving LRCLK 48kHz
    CodecWriteReg(0x09, 0x02);
    CodecWriteReg(0x08, 0x98); //I2S format, data is delayed 1 bit clock, HI-Z mode disabled
    CodecUpdateReg(0x14, 0xF0, (2 << 6) | (2 << 4)); //Stereo Line In
    CodecUpdateReg(0xD, 0xFF, (0xF << 4) | (0xF & 0x0F)); //ADC Level -12Db
    CodecUpdateReg(0xE, 0x40, 1 << 6); //Disconnect Line in from Left Headphone
    CodecUpdateReg(0xF, 0x40, 1 << 6); //Disconnect Line in from Right Headphone
    CodecUpdateReg(0xE, 0xF, 0xF); //Line in -6dB
    CodecUpdateReg(0xF, 0xF, 0xF); //Line in -6dB

    //Assert SHDN as first step in toggling SHDN when changing enabled circuitry
    CodecUpdateReg(0x17, 0x80, 0x00);

    //Enable ADCs and Line In
    CodecUpdateReg(0x17, 0xE3, 0x80 | 0x1 | 0x2 | 0x20 | 0x40);
}
