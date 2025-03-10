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
 * @file    main.c
 * @brief   ADC Example
 * @details This example configures the AFE of the MAX32675 ADCs to sample input
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mxc_device.h"
#include "board.h"
#include "led.h"
#include "mxc_delay.h"
#include "afe.h"

/***** Definitions *****/
#define ADC_CONVERSIONS_PER_SECOND 50
#define ADC_SAMPLE_RATE_120SPS 7
#define ADC_CH0 0
#define ADC_CH1 1
#define ADC_CH2 2
#define ADC_CH3 3
#define GPIO_OUT 3
#define DATA_READY_INT 2

#define AFE_TIMER_INSTANCE MXC_TMR1

/***** Globals *****/

/***** Functions *****/

// *****************************************************************************
int main(void)
{
    int status = E_NO_ERROR;
    uint32_t read_val = 0;
    uint64_t avg_adc_0 = 0;
    uint64_t avg_adc_1 = 0;

    status = afe_load_trims(AFE_TIMER_INSTANCE);
    if (status != E_NO_ERROR) {
        printf("Error during afe load trims: %d\n", status);
        while (1) {}
    }

    printf("\n\n\n\n\nMAX32675 ADC Example\n\n");

    printf("ADC0 and ADC1 are set to sample differentially between AIN2 (pos)\n");
    printf("and AIN3 (neg). Both ADCs are configured to a sampling rate of 120\n");
    printf("samples per second. ADC0 has a PGA gain of 8X for its input, ADC1 uses 16X\n");
    printf("input gain. ADC sample data is stored in the DATA0 register of each ADC.\n");
    printf("This example gets 50 samples from each, and calculates and prints\n");
    printf("the average reading.\n");

    printf("\nSampling will begin after 5 seconds...\n");

    MXC_Delay(MXC_DELAY_SEC(5));

    //
    // Enable ADCs
    //

    // Set AFE power state to normal
    afe_write_register(MXC_R_AFE_ADC_ZERO_PD, MXC_S_AFE_ADC_ZERO_PD_PD_RESET);
    afe_write_register(MXC_R_AFE_ADC_ONE_PD, MXC_S_AFE_ADC_ONE_PD_PD_RESET);

    afe_write_register(MXC_R_AFE_ADC_ZERO_PD, MXC_S_AFE_ADC_ZERO_PD_PD_NORMAL_MODE);
    afe_write_register(MXC_R_AFE_ADC_ONE_PD, MXC_S_AFE_ADC_ONE_PD_PD_NORMAL_MODE);

    // Set Reference voltages to be the Analog Power supply and ground
    afe_write_register(MXC_R_AFE_ADC_ZERO_CTRL, MXC_S_AFE_ADC_ZERO_CTRL_REF_SEL_AVDD_AND_AGND &
                                                    MXC_F_AFE_ADC_ZERO_CTRL_REF_SEL);
    afe_write_register(MXC_R_AFE_ADC_ONE_CTRL, MXC_S_AFE_ADC_ONE_CTRL_REF_SEL_AVDD_AND_AGND &
                                                   MXC_F_AFE_ADC_ONE_CTRL_REF_SEL);

    // Select AINP and AINN for AIN0
    afe_write_register(MXC_R_AFE_ADC_ZERO_MUX_CTRL0,
                       (((ADC_CH2 << MXC_F_AFE_ADC_ZERO_MUX_CTRL0_AINP_SEL_POS) &
                         MXC_F_AFE_ADC_ZERO_MUX_CTRL0_AINP_SEL) |
                        ((ADC_CH3 << MXC_F_AFE_ADC_ZERO_MUX_CTRL0_AINN_SEL_POS) &
                         MXC_F_AFE_ADC_ZERO_MUX_CTRL0_AINN_SEL)));

    afe_write_register(MXC_R_AFE_ADC_ONE_MUX_CTRL0,
                       (((ADC_CH2 << MXC_F_AFE_ADC_ONE_MUX_CTRL0_AINP_SEL_POS) &
                         MXC_F_AFE_ADC_ONE_MUX_CTRL0_AINP_SEL) |
                        ((ADC_CH3 << MXC_F_AFE_ADC_ONE_MUX_CTRL0_AINN_SEL_POS) &
                         MXC_F_AFE_ADC_ONE_MUX_CTRL0_AINN_SEL)));

    // Filter Options, Select SINC4 @ 120 samples per second
    afe_write_register(MXC_R_AFE_ADC_ZERO_FILTER,
                       MXC_S_AFE_ADC_ZERO_FILTER_LINEF_SINC4 |
                           ((ADC_SAMPLE_RATE_120SPS << MXC_F_AFE_ADC_ZERO_FILTER_RATE_POS) &
                            MXC_F_AFE_ADC_ZERO_FILTER_RATE));

    afe_write_register(MXC_R_AFE_ADC_ONE_FILTER,
                       MXC_S_AFE_ADC_ONE_FILTER_LINEF_SINC4 |
                           ((ADC_SAMPLE_RATE_120SPS << MXC_F_AFE_ADC_ONE_FILTER_RATE_POS) &
                            MXC_F_AFE_ADC_ONE_FILTER_RATE));

    // Bypass mode for input
    afe_write_register(MXC_R_AFE_ADC_ZERO_PGA, MXC_S_AFE_ADC_ZERO_PGA_SIG_PATH_PGA_PATH |
                                                   MXC_S_AFE_ADC_ZERO_PGA_GAIN_GAIN_8X);
    afe_write_register(MXC_R_AFE_ADC_ONE_PGA, MXC_S_AFE_ADC_ONE_PGA_SIG_PATH_PGA_PATH |
                                                  MXC_S_AFE_ADC_ONE_PGA_GAIN_GAIN_16X);

    //Enable ADC_RDY GPIO outputs
    afe_write_register(MXC_R_AFE_ADC_ZERO_GP0_CTRL,
                       (((GPIO_OUT << MXC_F_AFE_ADC_ZERO_GP0_CTRL_GP0_DIR_POS) &
                         MXC_F_AFE_ADC_ZERO_GP0_CTRL_GP0_DIR) |
                        ((DATA_READY_INT << MXC_F_AFE_ADC_ZERO_GP0_CTRL_GP0_OSEL_POS) &
                         MXC_F_AFE_ADC_ZERO_GP0_CTRL_GP0_OSEL)));

    afe_write_register(MXC_R_AFE_ADC_ONE_GP0_CTRL,
                       (((GPIO_OUT << MXC_F_AFE_ADC_ONE_GP0_CTRL_GP0_DIR_POS) &
                         MXC_F_AFE_ADC_ONE_GP0_CTRL_GP0_DIR) |
                        ((DATA_READY_INT << MXC_F_AFE_ADC_ONE_GP0_CTRL_GP0_OSEL_POS) &
                         MXC_F_AFE_ADC_ONE_GP0_CTRL_GP0_OSEL)));

    //
    // Infinite ADC reading loop
    //
    while (1) {
        //
        // Get 50 samples from each ADC
        //
        for (int adc_loop_count = 0; adc_loop_count < ADC_CONVERSIONS_PER_SECOND;
             adc_loop_count++) {
            // Start a single Conversion
            afe_write_register(MXC_R_AFE_ADC_ZERO_CONV_START,
                               MXC_S_AFE_ADC_ZERO_CONV_START_CONV_TYPE_SINGLE);

            afe_write_register(MXC_R_AFE_ADC_ONE_CONV_START,
                               MXC_S_AFE_ADC_ONE_CONV_START_CONV_TYPE_SINGLE);

            // NOTE: Wait for DATA_RDY on ADC1, so we don't need to bank swap back to 0, and assume it will finish first.
            // Also, this is only safe to do if ADCs are sampling at the same RATE.

            while (1) {
                afe_read_register(MXC_R_AFE_ADC_ONE_STATUS, &read_val);

                if (read_val & MXC_F_AFE_ADC_ONE_STATUS_DATA_RDY) {
                    break;
                }
            }

            // Now read data
            afe_read_register(MXC_R_AFE_ADC_ONE_DATA0, &read_val);
            avg_adc_1 += read_val;

            // Now read data
            afe_read_register(MXC_R_AFE_ADC_ZERO_DATA0, &read_val);
            avg_adc_0 += read_val;

            // Reading this data last also save another bank swap at the top.
        }

        // Calculate and display average readings
        avg_adc_0 = avg_adc_0 / ADC_CONVERSIONS_PER_SECOND;
        printf("ADC0 AVG val: %08llX\n", avg_adc_0);

        avg_adc_1 = avg_adc_1 / ADC_CONVERSIONS_PER_SECOND;
        printf("ADC1 AVG val: %08llX\n\n", avg_adc_1);

        // Reset ADC value averages
        avg_adc_0 = 0;
        avg_adc_1 = 0;
    } // END of ADC read loop
}
