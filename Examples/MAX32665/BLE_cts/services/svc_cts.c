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

#include "svc_cts.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/

/*! Characteristic read permissions */
#ifndef MCS_SEC_PERMIT_READ
#define MCS_SEC_PERMIT_READ SVC_SEC_PERMIT_READ
#endif

/*! Characteristic write permissions */
#ifndef MCS_SEC_PERMIT_WRITE
#define MCS_SEC_PERMIT_WRITE SVC_SEC_PERMIT_WRITE
#endif

/**************************************************************************************************
 Service variables
**************************************************************************************************/

/*Service variables declaration*/
const uint8_t attCtsSvcUuid[ATT_16_UUID_LEN] = { ATT_UUID_CUSTOM_TIME_SERVICE };

/*Characteristic variables declaration*/
const uint8_t svcCtsButtonUuid[ATT_16_UUID_LEN] = { ATT_UUID_CURRENT_TIME_CHAR };

static const uint8_t ctsValSvc[] = { ATT_UUID_CUSTOM_TIME_SERVICE };
static const uint16_t ctsLenSvc = sizeof(ctsValSvc);

static const uint8_t ctsButtonValCh[] = { ATT_PROP_READ | ATT_PROP_NOTIFY,
                                          UINT16_TO_BYTES(CTS_CURRENT_TIME_HDL), ATT_UUID_MCS_BUTTON };
static const uint16_t ctsButtonLenCh = sizeof(ctsButtonValCh);


/*Characteristic values declaration*/
static uint8_t ctsButtonVal[] = { 0 };
static const uint16_t ctsButtonValLen = sizeof(ctsButtonVal);

static uint8_t ctsButtonValChCcc[] = { UINT16_TO_BYTES(0x0000) };
static const uint16_t ctsButtonLenValChCcc = sizeof(ctsButtonValChCcc);

static uint8_t ctsRVal[] = { 0 };
static const uint16_t ctsRValLen = sizeof(ctsRVal);

static uint8_t ctsGVal[] = { 0 };
static const uint16_t ctsGValLen = sizeof(ctsGVal);

static uint8_t ctsBVal[] = { 0 };
static const uint16_t ctsBValLen = sizeof(ctsBVal);

/**************************************************************************************************
 Maxim Custom Service group
**************************************************************************************************/

/* Attribute list for cts group */
static const attsAttr_t ctsList[] = {
    /*-----------------------------*/
    /* Service declaration */
    { attPrimSvcUuid, (uint8_t *)ctsValSvc, (uint16_t *)&ctsLenSvc, sizeof(ctsValSvc), 0,
      MCS_SEC_PERMIT_READ },

    /*----------------------------*/
    /* Button characteristic declaration */
    { attChUuid, (uint8_t *)ctsButtonValCh, (uint16_t *)&ctsButtonLenCh, sizeof(ctsButtonValCh), 0,
      MCS_SEC_PERMIT_READ },
    /* Button characteristic value */
    { svcCtsButtonUuid, (uint8_t *)ctsButtonVal, (uint16_t *)&ctsButtonValLen, sizeof(ctsButtonVal),
      0, MCS_SEC_PERMIT_READ },
    /*Button characteristic value descriptor*/
    { attCliChCfgUuid, (uint8_t *)ctsButtonValChCcc, (uint16_t *)&ctsButtonLenValChCcc,
      sizeof(ctsButtonValChCcc), ATTS_SET_CCC, (ATTS_PERMIT_READ | SVC_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /* R characteristic declaration */
    { attChUuid, (uint8_t *)ctsRValCh, (uint16_t *)&ctsRLenCh, sizeof(ctsRValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* R characteristic characteristic value */
    { svcCtsRUuid, (uint8_t *)ctsRVal, (uint16_t *)&ctsRValLen, sizeof(ctsRVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /* G characteristic declaration */
    { attChUuid, (uint8_t *)ctsGValCh, (uint16_t *)&ctsGLenCh, sizeof(ctsGValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* G characteristic characteristic value */
    { svcCtsGUuid, (uint8_t *)ctsGVal, (uint16_t *)&ctsGValLen, sizeof(ctsGVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },

    /*-----------------------------*/
    /*  B characteristic declaration */
    { attChUuid, (uint8_t *)ctsBValCh, (uint16_t *)&ctsBLenCh, sizeof(ctsBValCh),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) },
    /* B characteristic value */
    { svcCtsBUuid, (uint8_t *)ctsBVal, (uint16_t *)&ctsBValLen, sizeof(ctsBVal),
      ATTS_SET_WRITE_CBACK, (MCS_SEC_PERMIT_READ | MCS_SEC_PERMIT_WRITE) }
};

/* Test group structure */
static attsGroup_t svcCtsGroup = { NULL, (attsAttr_t *)ctsList, NULL,
                                   NULL, MCS_START_HDL,         MCS_END_HDL };

/*************************************************************************************************/
/*!
 *  \brief  Add the services to the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCtsAddGroup(void)
{
    AttsAddGroup(&svcCtsGroup);
}

/*************************************************************************************************/
/*!
 *  \brief  Remove the services from the attribute server.
 *
 *  \return None.
 */
/*************************************************************************************************/
void SvcCtsRemoveGroup(void)
{
    AttsRemoveGroup(MCS_START_HDL);
}

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
void SvcCtsCbackRegister(attsReadCback_t readCback, attsWriteCback_t writeCback)
{
    svcCtsGroup.readCback = readCback;
    svcCtsGroup.writeCback = writeCback;
}
