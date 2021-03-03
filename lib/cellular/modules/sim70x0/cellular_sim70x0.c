/*
 * FreeRTOS Cellular Preview Release
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/* The config header is always included first. */


#include <stdint.h>
#include "cellular_platform.h"
#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_common.h"
#include "cellular_common_portable.h"
#include "cellular_sim70x0.h"

/*-----------------------------------------------------------*/

#define ENBABLE_MODULE_UE_RETRY_COUNT      ( 3U )
#define ENBABLE_MODULE_UE_RETRY_TIMEOUT    ( 5000U )

/*-----------------------------------------------------------*/

static CellularError_t sendAtCommandWithRetryTimeout( CellularContext_t * pContext,
                                                      const CellularAtReq_t * pAtReq );

/*-----------------------------------------------------------*/

static cellularModuleContext_t cellularSIM70XXContext = { 0 };

const char * CellularSrcTokenErrorTable[] =
{ "ERROR", "BUSY", "NO CARRIER", "NO ANSWER", "NO DIALTONE", "ABORTED", "+CMS ERROR", "+CME ERROR", "SEND FAIL" };
uint32_t CellularSrcTokenErrorTableSize = sizeof( CellularSrcTokenErrorTable ) / sizeof( char * );

const char * CellularSrcTokenSuccessTable[] =
{ "OK", "CONNECT", "SEND OK", ">" };
uint32_t CellularSrcTokenSuccessTableSize = sizeof( CellularSrcTokenSuccessTable ) / sizeof( char * );

const char * CellularUrcTokenWoPrefixTable[] =
{ "+APP PDP: 0,ACTIVE", "+APP PDP: 0,DEACTIVE", "NORMAL POWER DOWN", "PSM POWER DOWN", "RDY" };
uint32_t CellularUrcTokenWoPrefixTableSize = sizeof( CellularUrcTokenWoPrefixTable ) / sizeof( char * );


static  BYTE    nCID_Min = 0;
static  BYTE    nCID_Max = CELLULAR_NUM_SOCKET_MAX;     /* 0-12 */

static  BYTE    nPDP_Min = 0;
static  BYTE    nPDP_Max = CELLULAR_PDP_INDEX_MAX;      /* 0-4  */

BOOL    IsValidCID(int cid)
{
    return cid >= (int)nCID_Min && cid <= (int)nCID_Max;
}

BOOL    IsValidPDP(int cid)
{
    return cid >= (int)nPDP_Min && cid <= (int)nPDP_Max;
}


/*-----------------------------------------------------------*/

static CellularError_t sendAtCommandWithRetryTimeout( CellularContext_t * pContext,
                                                      const CellularAtReq_t * pAtReq )
{
    CellularError_t cellularStatus  = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus   = CELLULAR_PKT_STATUS_OK;
    uint8_t tryCount = 0;

    if( pAtReq == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        for( ; tryCount < ENBABLE_MODULE_UE_RETRY_COUNT; tryCount++ )
        {
            pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, *pAtReq, ENBABLE_MODULE_UE_RETRY_TIMEOUT );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );

            if( cellularStatus == CELLULAR_SUCCESS )
                break;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
CellularError_t Cellular_ModuleInit( const CellularContext_t * pContext,
                                     void ** ppModuleContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    bool status = false;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ppModuleContext == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Initialize the module context. */
        (void)memset( &cellularSIM70XXContext, 0, sizeof( cellularModuleContext_t ) );

        /* Create the mutex for DNS. */
        status = PlatformMutex_Create( &cellularSIM70XXContext.dnsQueryMutex, false );

        if( status == false )
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
        else
        {
            /* Create the queue for DNS. */
            cellularSIM70XXContext.pktDnsQueue = xQueueCreate( 1, sizeof( cellularDnsQueryResult_t ) );

            if( cellularSIM70XXContext.pktDnsQueue == NULL )
            {
                PlatformMutex_Destroy( &cellularSIM70XXContext.dnsQueryMutex );
                cellularStatus = CELLULAR_NO_MEMORY;
            }
            else
            {
                *ppModuleContext = ( void * ) &cellularSIM70XXContext;
            }

            cellularSIM70XXContext.pdnEvent = xEventGroupCreate();
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
CellularError_t Cellular_ModuleCleanUp( const CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( pContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        /* Delete DNS queue. */
        vQueueDelete( cellularSIM70XXContext.pktDnsQueue );

        /* Delete the mutex for DNS. */
        PlatformMutex_Destroy( &cellularSIM70XXContext.dnsQueryMutex );
    }

    return cellularStatus;
}

static CellularPktStatus_t set_CID_range_cb(CellularContext_t* pContext,
    const CellularATCommandResponse_t* pAtResp,
    void* pData,
    uint16_t dataLen)
{
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dataLen);

    if (pAtResp != NULL && pAtResp->pItm != NULL && pAtResp->pItm->pLine != NULL)
    {
        /* Handling: +CACID:(0-12)   */
        char    ns[8];
        char*   pLine = pAtResp->pItm->pLine;
        char*   pB1, * pE1, * pB2, * pE2;

        if ((pB1 = strchr(pLine, '(')) != NULL
         && (pE1 = strchr(pLine, '-')) != NULL
         && (pE2 = strchr(pLine, ')')) != NULL)
        {
            memset(ns, 0, sizeof(ns));
            strncpy(ns, pB1, pE1 - pB1);
            nCID_Min = (uint8_t)atoi(ns);

            pB2 = pE1 + 1;
            memset(ns, 0, sizeof(ns));
            strncpy(ns, pB2, pE2 - pB2);
            nCID_Max = (uint8_t)atoi(ns);

            CellularLogDebug("CID range: %d - %d", (int)nCID_Min, (int)nCID_Max);
            return CELLULAR_AT_SUCCESS;
        }
    }

    return CELLULAR_AT_ERROR;
}

static CellularPktStatus_t set_PDP_range_cb(CellularContext_t* pContext,
    const CellularATCommandResponse_t* pAtResp,
    void* pData,
    uint16_t dataLen)
{
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pData);
    UNREFERENCED_PARAMETER(dataLen);

    if (pAtResp != NULL && pAtResp->pItm != NULL && pAtResp->pItm->pLine != NULL)
    {
        /*Handling: +CNACT:(0-3),(0-2)  */
        char    ns[8];
        char*   pLine = pAtResp->pItm->pLine;
        char*   pB1, * pE1, * pB2, * pE2;

        if ((pB1 = strchr(pLine, '(')) != NULL
            && (pE1 = strchr(pLine, '-')) != NULL
            && (pE2 = strchr(pLine, ')')) != NULL)
        {
            memset(ns, 0, sizeof(ns));
            strncpy(ns, pB1, pE1 - pB1);
            nPDP_Min = (uint8_t)atoi(ns);

            pB2 = pE1 + 1;
            memset(ns, 0, sizeof(ns));
            strncpy(ns, pB2, pE2 - pB2);
            nPDP_Max = (uint8_t)atoi(ns);

            CellularLogDebug("PDP range: %d - %d", nPDP_Min, nPDP_Max);
            return CELLULAR_AT_SUCCESS;
        }
    }

    return CELLULAR_AT_ERROR;
}


/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
CellularError_t Cellular_ModuleEnableUE( CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularAtReq_t atReqGetNoResult =
    {
        NULL,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0
    };
    CellularAtReq_t atReqGetWithResult =
    {
        NULL,
        CELLULAR_AT_MULTI_WO_PREFIX,
        NULL,
        NULL,
        NULL,
        0
    };

    if (pContext == NULL)
        return CELLULAR_INVALID_HANDLE;

    /* Disable echo. */
    atReqGetWithResult.pAtCmd = "ATE0";
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetWithResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Disable DTR function. */
    atReqGetNoResult.pAtCmd = "AT&D0";
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Enable RTS/CTS hardware flow control. */
    atReqGetNoResult.pAtCmd = "AT+IFC=2,2";
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Configure Band configuration to all Cat-M1 bands. */
    atReqGetNoResult.pAtCmd = "AT+CBANDCFG=\"CAT-M\",1,3,8,18,19,26";   /*for Japan     */
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Configure Band configuration to all NB-IOT bands. */
    atReqGetNoResult.pAtCmd = "AT+CBANDCFG=\"NB-IOT\",1,3,8,18,19,26";   /*for Japan    */
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Configure RAT(s) to be Searched to Automatic. */
    atReqGetNoResult.pAtCmd = "AT+CNMP=2";
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Configure Network Category to be Searched under LTE  */
    atReqGetNoResult.pAtCmd = "AT+CNMP=38";     /*Only LTE, no GSM support  */
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    /* Configure RAT Searching Sequence to default radio access technology. */
    switch( CELLULAR_CONFIG_DEFAULT_RAT )
    {
    case CELLULAR_RAT_CATM1:
        atReqGetNoResult.pAtCmd = "AT+CMNB=1";
        break;
    case CELLULAR_RAT_NBIOT:
        atReqGetNoResult.pAtCmd = "AT+CMNB=2";
        break;
    case CELLULAR_RAT_GSM:
        atReqGetNoResult.pAtCmd = "AT+CNMP=13";
        break;
    default:
        /* Configure RAT Searching Sequence to automatic. */
        atReqGetNoResult.pAtCmd = "AT+CMNB=3";
        break;
    }
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
    if (cellularStatus != CELLULAR_SUCCESS)
        return cellularStatus;

    atReqGetNoResult.pAtCmd = "AT+CFUN=1";
    cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );

    atReqGetWithResult.pAtCmd = "AT+CACID=?";
    atReqGetWithResult.pAtRspPrefix = "+CACID";
    atReqGetWithResult.respCallback = set_CID_range_cb;
    cellularStatus = _Cellular_AtcmdRequestWithCallback(pContext, atReqGetWithResult);

    atReqGetWithResult.pAtCmd = "AT+CNACT=?";
    atReqGetWithResult.pAtRspPrefix = "+CNACT";
    atReqGetWithResult.respCallback = set_PDP_range_cb;
    cellularStatus = _Cellular_AtcmdRequestWithCallback(pContext, atReqGetWithResult);

#if 0
    atReqGetNoResult.pAtCmd = "AT+CACLOSE=0,0";
    _Cellular_AtcmdRequestWithCallback(pContext, atReqGetWithResult);

    atReqGetNoResult.pAtCmd = "AT+CNACT=0,0";
    _Cellular_AtcmdRequestWithCallback(pContext, atReqGetWithResult);
#endif // 0

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
CellularError_t Cellular_ModuleEnableUrc( CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularAtReq_t atReqGetNoResult =
    {
        NULL,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0
    };

    atReqGetNoResult.pAtCmd = "AT+COPS=3,2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    atReqGetNoResult.pAtCmd = "AT+CREG=2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    atReqGetNoResult.pAtCmd = "AT+CGREG=2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    atReqGetNoResult.pAtCmd = "AT+CEREG=2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    atReqGetNoResult.pAtCmd = "AT+CTZR=1";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    return cellularStatus;
}

/*-----------------------------------------------------------*/
