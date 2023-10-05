/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief  Maxim Custom service server.
 *
 *  Copyright (c) 2012-2018 Arm Ltd. All Rights Reserved.
 *  ARM Ltd. confidential and proprietary.
 *
 *  IMPORTANT.  Your use of this file is governed by a Software License Agreement
 *  ("Agreement") that must be accepted in order to download or otherwise receive a
 *  copy of this file.  You may not use or copy this file for any purpose other than
 *  as described in the Agreement.  If you do not agree to all of the terms of the
 *  Agreement do not use this file and delete all copies in your possession or control;
 *  if you do not have a copy of the Agreement, you must contact ARM Ltd. prior
 *  to any use, copying or further distribution of this software.
 */
/*************************************************************************************************/

#ifndef EXAMPLES_MAX32665_BLE_MCS_SERVICES_SVC_MCS_H_
#define EXAMPLES_MAX32665_BLE_MCS_SERVICES_SVC_MCS_H_

#include "wsf_types.h"
#include "att_api.h"
#include "util/bstream.h"
#include "svc_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! \addtogroup Mcs
 *  \{ */
/**************************************************************************************************
  Macros
**************************************************************************************************/
/*Custom Time Service UUID*/
#define ATT_UUID_CUSTOM_TIME_SERVICE 0x18, 0x05
/* MCS service GATT characteristic UUIDs*/
#define ATT_UUID_CURRENT_TIME_CHAR 0x2A, 0x2B
#define ATT_UUID_LOCAL_TIME_INFO_CHAR 0x2A0F
/**************************************************************************************************
 Handle Ranges
**************************************************************************************************/

/** \name Maxim custom Service Handles
 *
 */
/**@{*/
#define MCS_START_HDL 0x1500 /*!< \brief Start handle. */
#define MCS_END_HDL (MCS_MAX_HDL - 1) /*!< \brief End handle. */

/**************************************************************************************************
 Handles
**************************************************************************************************/

/*! \brief Maxim custom Service Handles */
enum {
    CTS_SVC_HDL = MCS_START_HDL, /*!< \brief Maxim custom service declaration */
    CTS_CURRENT_TIME_CH_HDL, /*!< \brief R characteristic */
    CTS_CURRENT_TIME_HDL, /*!< \brief R*/
    MCS_MAX_HDL /*!< \brief Maximum handle. */
};
/**@}*/

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCtsAddGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCtsRemoveGroup(void);

/*************************************************************************************************/
/*!
 *  \brief  Register callbacks for the service.
 *
 *  \param  readCback   Read callback function.
 *  \param  writeCback  Write callback function.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCtsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback);

/*! \} */ /* TEST_SERVICE */

#ifdef __cplusplus
};
#endif

#endif // EXAMPLES_MAX32665_BLE_MCS_SERVICES_SVC_MCS_H_
