/*
 * FreeRTOS-Cellular-Interface v1.2.0
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
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 */

/* The config header is always included first. */


#include <stdint.h>
#include "cellular_platform.h"
#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_common.h"
#include "cellular_common_portable.h"
#include "cellular_bg96.h"

/*-----------------------------------------------------------*/

#define ENBABLE_MODULE_UE_RETRY_COUNT      ( 3U )
#define ENBABLE_MODULE_UE_RETRY_TIMEOUT    ( 5000U )
#define BG96_NWSCANSEQ_CMD_MAX_SIZE        ( 29U ) /* The length of AT+QCFG="nwscanseq",020301,1\0. */

/*-----------------------------------------------------------*/

static CellularError_t sendAtCommandWithRetryTimeout( CellularContext_t * pContext,
                                                      const CellularAtReq_t * pAtReq );

/*-----------------------------------------------------------*/

static cellularModuleContext_t cellularBg96Context = { 0 };

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
{ "OK", "CONNECT", "SEND OK", ">" };
/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
uint32_t CellularSrcTokenSuccessTableSize = sizeof( CellularSrcTokenSuccessTable ) / sizeof( char * );

/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
const char * CellularUrcTokenWoPrefixTable[] =
{ "NORMAL POWER DOWN", "PSM POWER DOWN", "RDY" };
/* FreeRTOS Cellular Common Library porting interface. */
/* coverity[misra_c_2012_rule_8_7_violation] */
uint32_t CellularUrcTokenWoPrefixTableSize = sizeof( CellularUrcTokenWoPrefixTable ) / sizeof( char * );

cellularModuleSocketContext_t cellularBg96SocketContext[ CELLULAR_NUM_SOCKET_MAX ] = { 0 };

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

static bool appendRatList( char * pRatList,
                           CellularRat_t cellularRat )
{
    bool retValue = true;

    /* Configure RAT Searching Sequence to default radio access technology. */
    switch( cellularRat )
    {
        case CELLULAR_RAT_CATM1:
            strcat( pRatList, "02" );
            break;

        case CELLULAR_RAT_NBIOT:
            strcat( pRatList, "03" );
            break;

        case CELLULAR_RAT_GSM:
            strcat( pRatList, "01" );
            break;

        default:
            /* Configure RAT Searching Sequence to automatic. */
            retValue = false;
            break;
    }

    return retValue;
}

/*-----------------------------------------------------------*/

static bool _Cellular_CreateUdpSocketConnectMutex( cellularModuleSocketContext_t * pBg96SocketContext )
{
    bool status = false;

    status = PlatformMutex_Create( &pBg96SocketContext->udpSocketConnectMutex, false );

    return status;
}

/*-----------------------------------------------------------*/

static void _Cellular_DestroyUdpSocketConnectMutex( cellularModuleSocketContext_t * pBg96SocketContext )
{
    PlatformMutex_Destroy( &pBg96SocketContext->udpSocketConnectMutex );
}

/*-----------------------------------------------------------*/

static CellularError_t bg96SocketOpenCallback( CellularSocketHandle_t socketHandle )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    uint32_t socketId = socketHandle->socketId;
    cellularModuleSocketContext_t * pBg96SocketContext = &cellularBg96SocketContext[ socketId ];
    bool needUdpResources = false;

    if( socketHandle->socketProtocol == CELLULAR_SOCKET_PROTOCOL_UDP )
    {
        needUdpResources = true;
    }

    if( needUdpResources )
    {
        /* Allocate resources for UDP sockets. */
        if( _Cellular_CreateUdpSocketConnectMutex( pBg96SocketContext ) == false )
        {
            LogError( ( "bg96SocketOpenCallback: Create UDP socket mutex failed." ) );
            cellularStatus = CELLULAR_RESOURCE_CREATION_FAIL;
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Create the queue for UDP socket connect. */
            pBg96SocketContext->udpSocketOpenQueue = xQueueCreate( 1, sizeof( CellularUrcEvent_t ) );

            if( pBg96SocketContext->udpSocketOpenQueue == NULL )
            {
                LogError( ( "bg96SocketOpenCallback: Create UDP socket queue failed." ) );
                cellularStatus = CELLULAR_NO_MEMORY;

                /* Free the allocated resources. */
                _Cellular_DestroyUdpSocketConnectMutex( pBg96SocketContext );
            }
        }
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        socketHandle->pModemData = pBg96SocketContext;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

static CellularError_t bg96SocketCloseCallback( CellularSocketHandle_t socketHandle )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    cellularModuleSocketContext_t * pBg96SocketContext = ( cellularModuleSocketContext_t * ) socketHandle->pModemData;
    bool hasUdpResources = false;

    if( socketHandle->socketProtocol == CELLULAR_SOCKET_PROTOCOL_UDP )
    {
        hasUdpResources = true;
    }

    if( hasUdpResources )
    {
        /* Release UDP resources. */
        _Cellular_DestroyUdpSocketConnectMutex( pBg96SocketContext );
        xQueueDelete( pBg96SocketContext->udpSocketOpenQueue );
        socketHandle->pModemData = NULL;
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
        ( void ) memset( &cellularBg96Context, 0, sizeof( cellularModuleContext_t ) );

        /* Create the mutex for DNS. */
        status = PlatformMutex_Create( &cellularBg96Context.dnsQueryMutex, false );

        if( status == false )
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
        else
        {
            /* Create the queue for DNS. */
            cellularBg96Context.pktDnsQueue = xQueueCreate( 1, sizeof( cellularDnsQueryResult_t ) );

            if( cellularBg96Context.pktDnsQueue == NULL )
            {
                PlatformMutex_Destroy( &cellularBg96Context.dnsQueryMutex );
                cellularStatus = CELLULAR_NO_MEMORY;
            }
            else
            {
                *ppModuleContext = ( void * ) &cellularBg96Context;
            }
        }

        /* Set module callback function for socket open/close. */
        _Cellular_RegisterModuleSocketOpenCallback( pContext, bg96SocketOpenCallback );
        _Cellular_RegisterModuleSocketCloseCallback( pContext, bg96SocketCloseCallback );
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
        vQueueDelete( cellularBg96Context.pktDnsQueue );

        /* Delete the mutex for DNS. */
        PlatformMutex_Destroy( &cellularBg96Context.dnsQueryMutex );
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
    char ratSelectCmd[ BG96_NWSCANSEQ_CMD_MAX_SIZE ] = "AT+QCFG=\"nwscanseq\",";
    bool retAppendRat = true;

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
            /* Setting URC output port. */
            #if defined( CELLULAR_BG96_URC_PORT_USBAT ) || defined( BG96_URC_PORT_USBAT )
                atReqGetNoResult.pAtCmd = "AT+QURCCFG=\"urcport\",\"usbat\"";
            #else
                atReqGetNoResult.pAtCmd = "AT+QURCCFG=\"urcport\",\"uart1\"";
            #endif
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Configure Band configuration to all bands. */
            atReqGetNoResult.pAtCmd = "AT+QCFG=\"band\",f,400a0e189f,a0e189f";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Configure RAT(s) to be Searched to Automatic. */
            atReqGetNoResult.pAtCmd = "AT+QCFG=\"nwscanmode\",0,1";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            /* Configure Network Category to be Searched under LTE RAT to LTE Cat M1 and Cat NB1. */
            atReqGetNoResult.pAtCmd = "AT+QCFG=\"iotopmode\",2,1";
            cellularStatus = sendAtCommandWithRetryTimeout( pContext, &atReqGetNoResult );
        }

        if( cellularStatus == CELLULAR_SUCCESS )
        {
            retAppendRat = appendRatList( ratSelectCmd, CELLULAR_CONFIG_DEFAULT_RAT );
            configASSERT( retAppendRat == true );

            #ifdef CELLULAR_CONFIG_DEFAULT_RAT_2
                retAppendRat = appendRatList( ratSelectCmd, CELLULAR_CONFIG_DEFAULT_RAT_2 );
                configASSERT( retAppendRat == true );
            #endif

            #ifdef CELLULAR_CONFIG_DEFAULT_RAT_3
                retAppendRat = appendRatList( ratSelectCmd, CELLULAR_CONFIG_DEFAULT_RAT_3 );
                configASSERT( retAppendRat == true );
            #endif

            strcat( ratSelectCmd, ",1" ); /* Take effect immediately. */
            atReqGetNoResult.pAtCmd = ratSelectCmd;
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
