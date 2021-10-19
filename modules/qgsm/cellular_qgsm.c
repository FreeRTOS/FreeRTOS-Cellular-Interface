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
#include "cellular_qgsm.h"

/*-----------------------------------------------------------*/

#define ENBABLE_MODULE_UE_RETRY_COUNT      ( 3U )
#define ENBABLE_MODULE_UE_RETRY_TIMEOUT    ( 5000U )

/*-----------------------------------------------------------*/

static CellularError_t sendAtCommandWithRetryTimeout( CellularContext_t * pContext,
                                                      const CellularAtReq_t * pAtReq );

/*-----------------------------------------------------------*/

static cellularModuleContext_t cellularqgsmContext = { 0 };

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
const char * CellularSrcTokenErrorTable[] =
{ "ERROR", "BUSY", "NO CARRIER", "NO ANSWER", "NO DIALTONE", "ABORTED", "+CMS ERROR", "+CME ERROR", "SEND FAIL" };
/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
uint32_t CellularSrcTokenErrorTableSize = sizeof( CellularSrcTokenErrorTable ) / sizeof( char * );

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
const char * CellularSrcTokenSuccessTable[] =
{ "OK", "CONNECT OK", "SEND OK", ">", "0, CLOSE OK" };
/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
uint32_t CellularSrcTokenSuccessTableSize = sizeof( CellularSrcTokenSuccessTable ) / sizeof( char * );

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
const char * CellularUrcTokenWoPrefixTable[] =
{ "NORMAL POWER DOWN", "RDY" };
/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
uint32_t CellularUrcTokenWoPrefixTableSize = sizeof( CellularUrcTokenWoPrefixTable ) / sizeof( char * );

/*-----------------------------------------------------------*/

static CellularError_t sendAtCommandWithRetryTimeout( CellularContext_t * pContext,
                                                      const CellularAtReq_t * pAtReq )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
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
            {
                break;
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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
        ( void ) memset( &cellularqgsmContext, 0, sizeof( cellularModuleContext_t ) );

        /* Create the mutex for DNS. */
        status = PlatformMutex_Create( &cellularqgsmContext.dnsQueryMutex, false );

        if( status == false )
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
        else
        {
            /* Create the queue for DNS. */
            cellularqgsmContext.pktDnsQueue = xQueueCreate( 1, sizeof( cellularDnsQueryResult_t ) );

            if( cellularqgsmContext.pktDnsQueue == NULL )
            {
                PlatformMutex_Destroy( &cellularqgsmContext.dnsQueryMutex );
                cellularStatus = CELLULAR_NO_MEMORY;
            }
            else
            {
                *ppModuleContext = ( void * ) &cellularqgsmContext;
            }
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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
        vQueueDelete( cellularqgsmContext.pktDnsQueue );

        /* Delete the mutex for DNS. */
        PlatformMutex_Destroy( &cellularqgsmContext.dnsQueryMutex );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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

    if( pContext != NULL )
    {
        /* Disable echo. */
        atReqGetWithResult.pAtCmd = "ATE0";
        cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetWithResult );

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Disable DTR function. */
            atReqGetNoResult.pAtCmd = "AT&D0";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Enable RTS/CTS hardware flow control. */
            atReqGetNoResult.pAtCmd = "AT+IFC=2,2";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Report verbose mobile termination error. */
            atReqGetNoResult.pAtCmd = "AT+CMEE=2";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Configure Band configuration to all bands. */
            /* Use ATV1 to set the response format */
            atReqGetNoResult.pAtCmd = "ATV1";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            atReqGetNoResult.pAtCmd = "AT+CFUN=1";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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

    /*Enable multiple TCP IP session at the same time */
    atReqGetNoResult.pAtCmd = "AT+QIMUX=1";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Set the method to Handle Receive TCP/IP Data */
    atReqGetNoResult.pAtCmd = "AT+QINDI=1";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Operator Selection */
    atReqGetNoResult.pAtCmd = "AT+COPS=3,2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Network Registration */
    atReqGetNoResult.pAtCmd = "AT+CREG=2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Network registration status */
    atReqGetNoResult.pAtCmd = "AT+CGREG=2";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Network Time Synchronization Report */
    atReqGetNoResult.pAtCmd = "AT+CTZR=1";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*TCP IP Transfer Mode */
    atReqGetNoResult.pAtCmd = "AT+QIMODE=0";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );
    /*Connect with Domain Name Server */
    atReqGetNoResult.pAtCmd = "AT+QIDNSIP=1";
    ( void ) _Cellular_AtcmdRequestWithCallback( pContext, atReqGetNoResult );

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_StartTCPIP( CellularContext_t * pContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    CellularAtReq_t atReqStartTCPIP =
    {
        NULL,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Form the AT command. */
        atReqStartTCPIP.pAtCmd = "AT+QIREGAPP";
        cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqStartTCPIP );
    }

    return cellularStatus;
}
/*-----------------------------------------------------------*/
