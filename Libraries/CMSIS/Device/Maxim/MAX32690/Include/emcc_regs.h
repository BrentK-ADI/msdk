/**
 * @file    emcc_regs.h
 * @brief   Registers, Bit Masks and Bit Positions for the EMCC Peripheral Module.
 * @note    This file is @generated.
 * @ingroup emcc_registers
 */

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

#ifndef LIBRARIES_CMSIS_DEVICE_MAXIM_MAX32690_INCLUDE_EMCC_REGS_H_
#define LIBRARIES_CMSIS_DEVICE_MAXIM_MAX32690_INCLUDE_EMCC_REGS_H_

/* **** Includes **** */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__ICCARM__)
  #pragma system_include
#endif

#if defined (__CC_ARM)
  #pragma anon_unions
#endif
/// @cond
/*
    If types are not defined elsewhere (CMSIS) define them here
*/
#ifndef __IO
#define __IO volatile
#endif
#ifndef __I
#ifdef __cplusplus
#define __I volatile
#else
#define __I volatile const
#endif
#endif
#ifndef __O
#define __O  volatile
#endif
#ifndef __R
#define __R  volatile const
#endif
/// @endcond

/* **** Definitions **** */

/**
 * @ingroup     emcc
 * @defgroup    emcc_registers EMCC_Registers
 * @brief       Registers, Bit Masks and Bit Positions for the EMCC Peripheral Module.
 * @details     External Memory Cache Controller Registers.
 */

/**
 * @ingroup emcc_registers
 * Structure type to access the EMCC Registers.
 */
typedef struct {
    __I  uint32_t info;                 /**< <tt>\b 0x0000:</tt> EMCC INFO Register */
    __I  uint32_t sz;                   /**< <tt>\b 0x0004:</tt> EMCC SZ Register */
    __R  uint32_t rsv_0x8_0xff[62];
    __IO uint32_t ctrl;                 /**< <tt>\b 0x0100:</tt> EMCC CTRL Register */
    __R  uint32_t rsv_0x104_0x6ff[383];
    __IO uint32_t invalidate;           /**< <tt>\b 0x0700:</tt> EMCC INVALIDATE Register */
} mxc_emcc_regs_t;

/* Register offsets for module EMCC */
/**
 * @ingroup    emcc_registers
 * @defgroup   EMCC_Register_Offsets Register Offsets
 * @brief      EMCC Peripheral Register Offsets from the EMCC Base Peripheral Address.
 * @{
 */
#define MXC_R_EMCC_INFO                    ((uint32_t)0x00000000UL) /**< Offset from EMCC Base Address: <tt> 0x0000</tt> */
#define MXC_R_EMCC_SZ                      ((uint32_t)0x00000004UL) /**< Offset from EMCC Base Address: <tt> 0x0004</tt> */
#define MXC_R_EMCC_CTRL                    ((uint32_t)0x00000100UL) /**< Offset from EMCC Base Address: <tt> 0x0100</tt> */
#define MXC_R_EMCC_INVALIDATE              ((uint32_t)0x00000700UL) /**< Offset from EMCC Base Address: <tt> 0x0700</tt> */
/**@} end of group emcc_registers */

/**
 * @ingroup  emcc_registers
 * @defgroup EMCC_INFO EMCC_INFO
 * @brief    Cache ID Register.
 * @{
 */
#define MXC_F_EMCC_INFO_RELNUM_POS                     0 /**< INFO_RELNUM Position */
#define MXC_F_EMCC_INFO_RELNUM                         ((uint32_t)(0x3FUL << MXC_F_EMCC_INFO_RELNUM_POS)) /**< INFO_RELNUM Mask */

#define MXC_F_EMCC_INFO_PARTNUM_POS                    6 /**< INFO_PARTNUM Position */
#define MXC_F_EMCC_INFO_PARTNUM                        ((uint32_t)(0xFUL << MXC_F_EMCC_INFO_PARTNUM_POS)) /**< INFO_PARTNUM Mask */

#define MXC_F_EMCC_INFO_ID_POS                         10 /**< INFO_ID Position */
#define MXC_F_EMCC_INFO_ID                             ((uint32_t)(0x3FUL << MXC_F_EMCC_INFO_ID_POS)) /**< INFO_ID Mask */

/**@} end of group EMCC_INFO_Register */

/**
 * @ingroup  emcc_registers
 * @defgroup EMCC_SZ EMCC_SZ
 * @brief    Memory Configuration Register.
 * @{
 */
#define MXC_F_EMCC_SZ_CCH_POS                          0 /**< SZ_CCH Position */
#define MXC_F_EMCC_SZ_CCH                              ((uint32_t)(0xFFFFUL << MXC_F_EMCC_SZ_CCH_POS)) /**< SZ_CCH Mask */

#define MXC_F_EMCC_SZ_MEM_POS                          16 /**< SZ_MEM Position */
#define MXC_F_EMCC_SZ_MEM                              ((uint32_t)(0xFFFFUL << MXC_F_EMCC_SZ_MEM_POS)) /**< SZ_MEM Mask */

/**@} end of group EMCC_SZ_Register */

/**
 * @ingroup  emcc_registers
 * @defgroup EMCC_CTRL EMCC_CTRL
 * @brief    Cache Control and Status Register.
 * @{
 */
#define MXC_F_EMCC_CTRL_EN_POS                         0 /**< CTRL_EN Position */
#define MXC_F_EMCC_CTRL_EN                             ((uint32_t)(0x1UL << MXC_F_EMCC_CTRL_EN_POS)) /**< CTRL_EN Mask */

#define MXC_F_EMCC_CTRL_WRITE_ALLOC_POS                1 /**< CTRL_WRITE_ALLOC Position */
#define MXC_F_EMCC_CTRL_WRITE_ALLOC                    ((uint32_t)(0x1UL << MXC_F_EMCC_CTRL_WRITE_ALLOC_POS)) /**< CTRL_WRITE_ALLOC Mask */

#define MXC_F_EMCC_CTRL_CWFST_DIS_POS                  2 /**< CTRL_CWFST_DIS Position */
#define MXC_F_EMCC_CTRL_CWFST_DIS                      ((uint32_t)(0x1UL << MXC_F_EMCC_CTRL_CWFST_DIS_POS)) /**< CTRL_CWFST_DIS Mask */

#define MXC_F_EMCC_CTRL_RDY_POS                        16 /**< CTRL_RDY Position */
#define MXC_F_EMCC_CTRL_RDY                            ((uint32_t)(0x1UL << MXC_F_EMCC_CTRL_RDY_POS)) /**< CTRL_RDY Mask */

/**@} end of group EMCC_CTRL_Register */

/**
 * @ingroup  emcc_registers
 * @defgroup EMCC_INVALIDATE EMCC_INVALIDATE
 * @brief    Invalidate All Cache Contents. Any time this register location is written
 *           (regardless of the data value), the cache controller immediately begins
 *           invalidating the entire contents of the cache memory. The cache will be in
 *           bypass mode until the invalidate operation is complete. System software can
 *           examine the Cache Ready bit (CACHE_CTRL.CACHE_RDY) to determine when the
 *           invalidate operation is complete. Note that it is not necessary to disable the
 *           cache controller prior to beginning this operation. Reads from this register
 *           always return 0.
 * @{
 */
#define MXC_F_EMCC_INVALIDATE_IA_POS                   0 /**< INVALIDATE_IA Position */
#define MXC_F_EMCC_INVALIDATE_IA                       ((uint32_t)(0xFFFFFFFFUL << MXC_F_EMCC_INVALIDATE_IA_POS)) /**< INVALIDATE_IA Mask */

/**@} end of group EMCC_INVALIDATE_Register */

#ifdef __cplusplus
}
#endif

#endif // LIBRARIES_CMSIS_DEVICE_MAXIM_MAX32690_INCLUDE_EMCC_REGS_H_
