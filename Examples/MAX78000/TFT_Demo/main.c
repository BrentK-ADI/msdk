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
 * @brief   TFT Demo Example!
 *
 * @details
 */

/***** Includes *****/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "mxc.h"
#include "icc.h"
#include "mxc_device.h"
#include "mxc_delay.h"
#include "utils.h"
#include "state.h"
#include "keypad.h"
#include "led.h"
#include "pb.h"
#include "example_config.h"

#ifdef BOARD_FTHR_REVA

#define TFT_BUFF_SIZE 32 // TFT buffer size

void TFT_Print(char *str, int x, int y, int font)
{
    // fonts id
    text_t text;
    text.data = str;
    text.len = 20;
    MXC_TFT_PrintFont(x, y, font, &text, NULL);
}

void TFT_Feather_test(void)
{
    area_t _area;
    area_t *area;
    char buff[TFT_BUFF_SIZE];

    MXC_TFT_SetRotation(ROTATE_90);
    MXC_TFT_ShowImage(0, 0, (int)&img_1_rgb565[0]);

    MXC_Delay(2000000);

    MXC_TFT_SetBackGroundColor(RED);

    area = &_area;
    area->x = 10;
    area->y = 10;
    area->w = 200;
    area->h = 100;

    MXC_TFT_FillRect(area, GREEN);

    MXC_Delay(2000000);

    MXC_TFT_ClearScreen();
    MXC_TFT_Line(10, 10, 200, 200, NAVY);
    MXC_Delay(1000000);
    MXC_TFT_Rectangle(10, 10, 200, 200, NAVY);
    MXC_Delay(1000000);
    MXC_TFT_Circle(100, 100, 50, PURPLE);
    MXC_Delay(1000000);
    MXC_TFT_FillCircle(100, 100, 50, PURPLE);
    MXC_Delay(1000000);

    MXC_TFT_SetBackGroundColor(BLACK);
    MXC_TFT_SetForeGroundColor(WHITE); // set chars to white

    MXC_TFT_ClearScreen();
    memset(buff, 32, TFT_BUFF_SIZE);
    snprintf(buff, sizeof(buff), "ANALOG DEVICES");
    TFT_Print(buff, 0, 10, (int)&Liberation_Sans12x12[0]);

    snprintf(buff, sizeof(buff), "Analog Devices");
    TFT_Print(buff, 0, 50, (int)&Liberation_Sans24x24[0]);

    snprintf(buff, sizeof(buff), "Analog Devices");
    TFT_Print(buff, 0, 100, (int)&Liberation_Sans28x28[0]);

    snprintf(buff, sizeof(buff), "Analog Devices");
    TFT_Print(buff, 0, 150, (int)&Liberation_Sans16x16[0]);

    snprintf(buff, sizeof(buff), "Analog Devices");
    TFT_Print(buff, 0, 200, (int)&Liberation_Sans19x19[0]);

    while (1) {}
    // stop here
}
#endif // #ifdef BOARD_FTHR_REVA

int main(void)
{
#if defined(BOARD_FTHR_REVA)
    // Wait for PMIC 1.8V to become available, about 180ms after power up.
    MXC_Delay(200000);
#endif

    /* Enable cache */
    MXC_ICC_Enable(MXC_ICC0);

    /* Set system clock to 100 MHz */
    MXC_SYS_Clock_Select(MXC_SYS_CLOCK_IPO);
    SystemCoreClockUpdate();

#ifdef BOARD_FTHR_REVA
#ifdef ENABLE_TFT
    /* Initialize TFT display */
    MXC_TFT_Init(MXC_SPI0, 1, NULL, NULL);
    TFT_Feather_test();
#endif
#endif // #ifdef BOARD_FTHR_REVA

#ifdef BOARD_EVKIT_V1
    int key;
    unsigned int start_time;
    State *state;

    /* Initialize RTC */
    MXC_RTC_Init(0, 0);
    MXC_RTC_Start();

#ifdef ENABLE_TFT
    /* Initialize TFT display */
    MXC_TFT_Init();
#endif

#ifdef ENABLE_TS
    /* Initialize Touch Screen controller */
    MXC_TS_Init();
    MXC_TS_Start();
#endif

    /* Display Home page */
    state_init();

    /* Get current time */
    start_time = utils_get_time_ms();

    while (1) {
        /* Get current screen state */
        state = state_get_current();

#ifdef ENABLE_TS
        /* Check pressed touch screen key */
        key = MXC_TS_GetKey();
#else
        key = KEY_1;
#endif

        if (key > 0) {
            state->prcss_key(key);
            start_time = utils_get_time_ms();
        }

        /* Check timer tick */
        if (utils_get_time_ms() >= (start_time + state->timeout)) {
            if (state->tick) {
                state->tick();
                start_time = utils_get_time_ms();
            }
        }
    }
#endif // #ifdef BOARD_EVKIT_V1
}
