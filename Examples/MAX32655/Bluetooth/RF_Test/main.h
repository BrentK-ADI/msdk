/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Main module.
 *
 *  Copyright (c) 2013-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Portions Copyright (c) 2022-2023 Analog Devices, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef EXAMPLES_MAX32655_BLUETOOTH_RF_TEST_MAIN_H_
#define EXAMPLES_MAX32655_BLUETOOTH_RF_TEST_MAIN_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ll_init_api.h"
#include "ll_api.h"
#include "chci_tr.h"
#include "lhci_api.h"
#include "hci_defs.h"
#include "wsf_assert.h"
#include "wsf_buf.h"
#include "wsf_heap.h"
#include "wsf_timer.h"
#include "wsf_trace.h"
#include "wsf_bufio.h"
#include "wsf_cs.h"
#include "bb_ble_sniffer_api.h"
#include "pal_bb.h"
#include "pal_cfg.h"
#include "tmr.h"
#include "wut_regs.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "task.h"
#include "semphr.h"
#include "FreeRTOS_CLI.h"
#include "uart.h"
#include "mxc_delay.h"
#include "mxc_errors.h"

typedef enum uint8_t {
    NO_TEST,
    BLE_RX_TEST,
    BLE_TX_TEST,
    BLE_CONST_TX,
    BLE_SWEEP_TEST,
    BLE_FHOP_TEST,
} test_t;

typedef union {
    struct {
        uint16_t duration_ms;
        uint8_t channel;
        test_t testType;
    };
    uint32_t allData;
} tx_config_t;

typedef union {
    struct {
        uint8_t start_channel;
        uint8_t end_channel;
        uint16_t duration_per_ch_ms;
    };
    uint32_t allData;
} sweep_config_t;

typedef struct {
    uint8_t cmd[100];
    uint8_t length;
} cmd_history_t;

typedef struct {
    cmd_history_t command[10];
    uint8_t head;
    uint8_t tail;
    int queuePointer;
} queue_t;

void setPhy(uint8_t newPhy);
void startFreqHopping(void);
void setPacketLen(uint8_t len);
void setPacketType(uint8_t type);
void setTxPower(int8_t power);
void printConfigs(void);
uint16_t getFreqFromRfChannel(uint8_t ch);

#endif // EXAMPLES_MAX32655_BLUETOOTH_RF_TEST_MAIN_H_
