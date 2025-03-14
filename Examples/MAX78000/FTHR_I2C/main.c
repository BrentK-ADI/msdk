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
 * @file        main.c
 * @brief       I2C Loopback Example
 * @details     This example uses the I2C Master to read/write from/to the I2C Slave. For
 *              this example you must connect P0.10 to P0.16 (SCL) and P0.11 to P0.17 (SCL). The Master
 *              will use P0.10 and P0.11. The Slave will use P0.16 and P0.17. You must also
 *              connect the pull-up jumpers (JP21 and JP22) to the proper I/O voltage.
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "nvic_table.h"
#include "i2c.h"
#include "dma.h"

#ifdef BOARD_EVKIT_V1
#warning This example is not supported by the standard EvKit.
#endif

/***** Definitions *****/
#define PMIC_I2C MXC_I2C1

#define I2C_FREQ 100000
#define PMIC_SLAVE_ADDR (0x28)
#define INIT_LEN 2
#define LED_SET_LEN 4
#define LED_CFG_REG_ADDR 0x2C
#define LED_SET_REG_ADDR 0x2D
#define PMIC_LED_BLUE 0x1
#define PMIC_LED_RED 0x2
#define PMIC_LED_GREEN 0x4

/***** Globals *****/
static uint8_t tx_buf[LED_SET_LEN];

/***** Functions *****/

void SetLEDs(int state)
{
    int error;
    // Set the LED Color
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = LED_SET_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    tx_buf[0] = LED_SET_REG_ADDR;
    tx_buf[1] = (uint8_t)((state & PMIC_LED_BLUE) << 5); //Set Blue LED?
    tx_buf[2] = (uint8_t)((state & PMIC_LED_RED) << 4); //Set Red LED?
    tx_buf[3] = (uint8_t)((state & PMIC_LED_GREEN) << 3); //Set Green LED?

    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        printf("Error writing: %d\n", error);
        while (1) {}
    }
}

int main(void)
{
    MXC_Delay(MXC_DELAY_MSEC(500)); //Wait for PMIC to power-up
    printf("\n******** Featherboard I2C Demo *********\n");

    printf("\nThis demo uses I2C to change the state of the\n");
    printf("on-board Power Management IC's RGB LED every second.\n");

    printf("\nYou should observe the LED cycling through it's\n");
    printf("eight color states.\n");

    int error, i = 0;

    //Setup the I2CM
    error = MXC_I2C_Init(PMIC_I2C, 1, 0);

    if (error != E_NO_ERROR) {
        printf("-->Failed master\n");
        while (1) {}
    } else {
        printf("\n-->I2C Initialization Complete");
    }

    MXC_I2C_SetFrequency(PMIC_I2C, I2C_FREQ);

    // Set the LED current strength
    mxc_i2c_req_t reqMaster;
    reqMaster.i2c = PMIC_I2C;
    reqMaster.addr = PMIC_SLAVE_ADDR;
    reqMaster.tx_buf = tx_buf;
    reqMaster.tx_len = INIT_LEN;
    reqMaster.rx_buf = NULL;
    reqMaster.rx_len = 0;
    reqMaster.restart = 0;

    tx_buf[0] = LED_CFG_REG_ADDR;
    tx_buf[1] = 1; //Set LED current to 1mA

    if ((error = MXC_I2C_MasterTransaction(&reqMaster)) != 0) {
        printf("Error writing: %d\n", error);
        while (1) {}
    }

    while (1) {
        MXC_Delay(MXC_DELAY_SEC(1));
        SetLEDs(i);
        i = (i + 1) % 8; //Cycle through 8 LED Colors
    }
}
