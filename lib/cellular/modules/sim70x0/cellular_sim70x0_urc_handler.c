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

#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_common_internal.h"

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "cellular_platform.h"
#include "cellular_types.h"
#include "cellular_common.h"
#include "cellular_common_api.h"
#include "cellular_common_portable.h"
#include "cellular_sim70x0.h"

/*-----------------------------------------------------------*/

static void _Cellular_ProcessPowerDown( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessPsmPowerDown( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessModemRdy( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessSocketOpen( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessSocketDataInd( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessSocketState(CellularContext_t* pContext, char* pInputLine);
static void _Cellular_ProcessPdnStatus(CellularContext_t* pContext, char* pInputLine);
static void _Cellular_ProcessSimstat( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessIndication( CellularContext_t * pContext, char * pInputLine );
static void _Cellular_ProcessCereg(CellularContext_t* pContext, char* pInputLine);

/*-----------------------------------------------------------*/

/* Try to Keep this map in Alphabetical order. */
/* FreeRTOS Cellular Common Library porting interface. */
CellularAtParseTokenMap_t CellularUrcHandlerTable[] =
{
    { "APP PDP",                _Cellular_ProcessPdnStatus     },
    { "CADATAIND",              _Cellular_ProcessSocketDataInd },
    { "CAOPEN",                 _Cellular_ProcessSocketOpen    },
    { "CASTATE",                _Cellular_ProcessSocketState   },
    { "CEREG",                  _Cellular_ProcessCereg },
    { "CGREG",                  Cellular_CommonUrcProcessCgreg },
    { "CPIN",                   _Cellular_ProcessSimstat       },
    { "CPSMSTATUS: ENTER PSM",  _Cellular_ProcessPsmPowerDown  },
    { "CREG",                   Cellular_CommonUrcProcessCreg  },
    { "CSQ",                    _Cellular_ProcessIndication    },
    { "NORMAL POWER DOWN",      _Cellular_ProcessPowerDown     },
    { "RDY",                    _Cellular_ProcessModemRdy      },
};

/* FreeRTOS Cellular Common Library porting interface. */
uint32_t CellularUrcHandlerTableSize = sizeof( CellularUrcHandlerTable ) / sizeof( CellularAtParseTokenMap_t );

/*-----------------------------------------------------------*/

/* internal function of _parseSocketOpen to reduce complexity. */
static CellularPktStatus_t _parseSocketOpenNextTok( const char * pToken,
                                                    uint32_t sockIndex,
                                                    CellularSocketContext_t * pSocketData )
{
    /* Handling: CAOPEN: <link-num>,<err> */
    int32_t             sockStatus      = 0;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;

    atCoreStatus = Cellular_ATStrtoi( pToken, 10, &sockStatus );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( sockStatus != 0 )
        {
            const struct
            {
                uint8_t     nErr;
                const char* sErr;
            }   err_tb[] =
            {
                { 0,  "Success"                     },
                { 1,  "Socket error"                },
                { 2,  "No memory"                   },
                { 3,  "Connection limit"            },
                { 4,  "Parameter invalid"           },
                { 6,  "Invalid IP address"          },
                { 7,  "Not support the function"    },
                { 12, "Canft bind the port"        },
                { 13, "Canft listen the port"      },
                { 20, "Canft resolv the host"      },
                { 21, "Network not active"          },
                { 23, "Remote refuse"               },
                { 24, "Certificatefs time expired" },
                { 25, "Certificatefs common name does not match"                   },
                { 26, "Certificatefs common name does not match and time expired"  },
                { 27, "Connect failed"                                              },
            };

            const char* sErr = "unknown error";
            int         i;

            pSocketData->socketState = SOCKETSTATE_DISCONNECTED;
            for (i = 0; i < sizeof(err_tb) / sizeof(err_tb[0]); i++)
            {
                if ((int32_t)err_tb[i].nErr == sockStatus)
                {
                    sErr = err_tb[i].sErr;
                    break;
                }
            }
            CellularLogError( "_parseSocketOpen: id=%d, status: [%d] %s", sockIndex, sockStatus, sErr );
        }
        else
        {
            pSocketData->socketState = SOCKETSTATE_CONNECTED;
            CellularLogDebug( "_parseSocketOpen: Socket open success, conn %d", sockIndex );
        }

        /* Indicate the upper layer about the socket open status. */
        if( pSocketData->openCallback != NULL )
        {
            if( sockStatus != 0 )
            {
                pSocketData->openCallback( CELLULAR_URC_SOCKET_OPEN_FAILED,
                                           pSocketData, pSocketData->pOpenCallbackContext );
            }
            else
            {
                pSocketData->openCallback( CELLULAR_URC_SOCKET_OPENED,
                                           pSocketData, pSocketData->pOpenCallbackContext );
            }
        }
        else
        {
            CellularLogError( "_parseSocketOpen: Socket open callback for conn %d is not set!!", sockIndex );
        }
    }

    pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    return pktStatus;
}

/*-----------------------------------------------------------*/

static void _Cellular_ProcessCereg(CellularContext_t* pContext, char* pInputLine)
{
    /*
    Handling:
        +CEREG: <stat>[,[<tac>],[<rac>],[<ci>],[<AcT>]]
        +CEREG: 1,"122D","A","2845C10",7
    */

    if (pContext == NULL || pInputLine == NULL)
    {
        CellularLogError("Error processing CEREG (nul pointer)" );
        return;
    }

    char* pUrcStr;
    char* pToken = NULL;

    pUrcStr = pInputLine;
    if (CELLULAR_AT_SUCCESS != Cellular_ATRemoveAllWhiteSpaces(pUrcStr))
        return;

    if (CELLULAR_AT_SUCCESS == Cellular_ATGetNextTok(&pUrcStr, &pToken))
    {
        Cellular_CommonUrcProcessCereg(pContext, pToken);    /* only handle <stat>*/
    }
    else
    {
        CellularLogError("Error processing CEREG: get <stat> failed.");
    }

    /* ignore <tac>, <rac>, <ci>, <Act> field*/
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessSocketOpen( CellularContext_t * pContext,
                                         char * pInputLine )
{
    /*handling +CAOPEN: <link-num>,<error>  */
    char    *           pUrcStr         = NULL;
    char    *           pToken          = NULL;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    uint32_t            sockIndex       = 0;
    int32_t             tempValue       = 0;

    if( pContext == NULL || pInputLine == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pUrcStr = pInputLine;
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces(pUrcStr);
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
        atCoreStatus = Cellular_ATGetNextTok( &pUrcStr, &pToken );
        
    if( atCoreStatus == CELLULAR_AT_SUCCESS )
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );
        
    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( IsValidPDP( tempValue ) )
        {
            sockIndex = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error processing in Socket index. token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    if (atCoreStatus == CELLULAR_AT_SUCCESS)
        atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pToken);

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
        atCoreStatus = Cellular_ATStrtoi(pToken, 10, &tempValue);

    if (atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        CellularSocketContext_t* pSocketData;
                    
        pSocketData = _Cellular_GetSocketData(pContext, sockIndex);
        pktStatus = _parseSocketOpenNextTok(pToken, sockIndex, pSocketData);
    }
    else
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );

    if( pktStatus != CELLULAR_PKT_STATUS_OK )
    {
        CellularLogDebug( "Socket Open URC Parse failure" );
    }
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _parseUrcIndicationCsq( const CellularContext_t * pContext,
                                                   char * pUrcStr )
{
    /*Handling +CSQ: <rssi>,<ber>   */
    char *              pToken          = NULL;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularError_t     cellularStatus  = CELLULAR_SUCCESS;
    int32_t             retStrtoi       = 0;
    int16_t             csqRssi         = CELLULAR_INVALID_SIGNAL_VALUE;
    int16_t             csqBer          = CELLULAR_INVALID_SIGNAL_VALUE;
    CellularSignalInfo_t signalInfo     = { 0 };
    char *              pLocalUrcStr    = pUrcStr;

    if( ( pContext == NULL ) || ( pUrcStr == NULL ) )
    {
        atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
    }
    else
    {
        /*<rssi>    */
        atCoreStatus = Cellular_ATGetNextTok( &pLocalUrcStr, &pToken );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &retStrtoi );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( retStrtoi >= INT16_MIN ) && ( retStrtoi <= ( int32_t ) INT16_MAX ) )
        {
            cellularStatus = _Cellular_ConvertCsqSignalRssi( (int16_t) retStrtoi, &csqRssi );

            if( cellularStatus != CELLULAR_SUCCESS )
            {
                atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
            }
        }
        else
        {
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    /* Parse the BER index from string and convert it. */
    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        /*<ber> */
        atCoreStatus = Cellular_ATGetNextTok( &pLocalUrcStr, &pToken );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &retStrtoi );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( retStrtoi >= INT16_MIN ) &&
            ( retStrtoi <= ( int32_t ) INT16_MAX ) )
        {
            cellularStatus = _Cellular_ConvertCsqSignalBer( (int16_t) retStrtoi, &csqBer );

            if( cellularStatus != CELLULAR_SUCCESS )
            {
                atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
            }
        }
        else
        {
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    /* Handle the callback function. */
    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        signalInfo.rssi = csqRssi;
        signalInfo.rsrp = CELLULAR_INVALID_SIGNAL_VALUE;
        signalInfo.rsrq = CELLULAR_INVALID_SIGNAL_VALUE;
        signalInfo.ber  = csqBer;
        signalInfo.bars = CELLULAR_INVALID_SIGNAL_BAR_VALUE;
        _Cellular_SignalStrengthChangedCallback( pContext, CELLULAR_URC_EVENT_SIGNAL_CHANGED, &signalInfo );
    }

    if( atCoreStatus != CELLULAR_AT_SUCCESS )
    {
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessIndication( CellularContext_t * pContext,
                                         char * pInputLine )
{
    /*Handling: +CSQ: <rssi>,<ber>  */
    char *              pUrcStr      = NULL;
    CellularPktStatus_t pktStatus    = CELLULAR_PKT_STATUS_OK;
    CellularATError_t   atCoreStatus = CELLULAR_AT_SUCCESS;

    /* Check context status. */
    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( pInputLine == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pUrcStr = pInputLine;
        atCoreStatus = Cellular_ATRemoveAllDoubleQuote( pUrcStr );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATRemoveLeadingWhiteSpaces( &pUrcStr );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            if( strstr( pUrcStr, "+CSQ:" ) != NULL )
            {
                pktStatus = _parseUrcIndicationCsq( ( const CellularContext_t * ) pContext, pUrcStr );
            }
        }

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    if( pktStatus != CELLULAR_PKT_STATUS_OK )
    {
        CellularLogDebug( "UrcIndication Parse failure" );
    }
}

/*-----------------------------------------------------------*/

static void _informDataReadyToUpperLayer( CellularSocketContext_t * pSocketData )
{
    /* Indicate the upper layer about the data reception. */
    if( ( pSocketData != NULL ) && ( pSocketData->dataReadyCallback != NULL ) )
    {
        pSocketData->dataReadyCallback( pSocketData, pSocketData->pDataReadyCallbackContext );
    }
    else
    {
        CellularLogError( "_parseSocketUrc: Data ready callback not set!!" );
    }
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _parseSocketUrcRecv( const CellularContext_t * pContext,
                                                char * pUrcStr )
{
    /*Handling +CADATAIND: <cid>    */
    char *      pToken          = NULL;
    char *      pLocalUrcStr    = pUrcStr;
    int32_t     tempValue       = 0;
    uint32_t    sockIndex       = 0;
    CellularSocketContext_t *   pSocketData     = NULL;
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;

    atCoreStatus = Cellular_ATGetNextTok( &pLocalUrcStr, &pToken );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            if( ( tempValue >= 0 ) && ( tempValue < ( int32_t ) CELLULAR_NUM_SOCKET_MAX ) )
            {
                sockIndex = ( uint32_t ) tempValue;
            }
            else
            {
                CellularLogError( "Error in processing SockIndex. Token %s", pToken );
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        pSocketData = _Cellular_GetSocketData( pContext, sockIndex );

        if( pSocketData != NULL )
        {
            if( pSocketData->dataMode == CELLULAR_ACCESSMODE_BUFFER )
            {
                /* Data received indication in buffer mode, need to fetch the data. */
                CellularLogDebug( "Data Received on socket Conn Id %d", sockIndex );
                _informDataReadyToUpperLayer( pSocketData );
            }
        }
        else
        {
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    if( atCoreStatus != CELLULAR_AT_SUCCESS )
    {
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _parseSocketUrcClosed( const CellularContext_t * pContext,
                                                  char * pUrcStr )
{
    char *      pToken = NULL;
    char *      pLocalUrcStr = pUrcStr;
    int32_t     tempValue = 0;
    uint32_t    sockIndex = 0;
    CellularSocketContext_t *   pSocketData = NULL;
    CellularATError_t           atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t         pktStatus = CELLULAR_PKT_STATUS_OK;

    atCoreStatus = Cellular_ATGetNextTok( &pLocalUrcStr, &pToken );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue < ( int32_t ) CELLULAR_NUM_SOCKET_MAX )
        {
            sockIndex = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing Socket Index. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        pSocketData = _Cellular_GetSocketData( pContext, sockIndex );

        if( pSocketData != NULL )
        {
            pSocketData->socketState = SOCKETSTATE_DISCONNECTED;
            CellularLogDebug( "Socket closed. Conn Id %d", sockIndex );

            /* Indicate the upper layer about the socket close. */
            if( pSocketData->closedCallback != NULL )
            {
                pSocketData->closedCallback( pSocketData, pSocketData->pClosedCallbackContext );
            }
            else
            {
                CellularLogInfo( "_parseSocketUrc: Socket close callback not set!!" );
            }
        }
        else
        {
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    if( atCoreStatus != CELLULAR_AT_SUCCESS )
    {
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

static  int nCurrent_cid = -1;

/*-----------------------------------------------------------*/
static CellularPktStatus_t _parseSocketUrcDeAct(const CellularContext_t* pContext,
    char* pUrcStr)
{
    /*Handling +APP PDP: DEACTIVE   */

    pUrcStr = NULL;
    _Cellular_PdnEventCallback(pContext, CELLULAR_URC_EVENT_PDN_DEACTIVATED, nCurrent_cid );

    return CELLULAR_PKT_STATUS_OK;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _parseSocketUrcAct( const CellularContext_t * pContext,
                                               char * pUrcStr )
{
    /*+APP PDP: <cid>, ACTIVE   */
    /*+APP PDP: <cid>, DEACTIVE */
    int32_t tempValue       = 0;
    char *  pToken          = NULL;
    char *  pLocalUrcStr    = pUrcStr;
    uint8_t contextId       = 0;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;

    atCoreStatus = Cellular_ATGetNextTok( &pLocalUrcStr, &pToken );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );
    }

    if (atCoreStatus == CELLULAR_AT_SUCCESS)
    {
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( ( tempValue >= ( int32_t ) CELLULAR_PDN_CONTEXT_ID_MIN ) &&
              ( tempValue <= ( int32_t ) CELLULAR_PDN_CONTEXT_ID_MAX ) ) )
        {
            contextId = ( uint8_t ) tempValue;

            if(IsValidPDP(contextId))
            {
                atCoreStatus = Cellular_ATGetNextTok(&pLocalUrcStr, &pToken);

                CellularLogDebug("PDN Context: %d = %s", contextId, pToken);

                /* Indicate the upper layer about the PDN active/deactivate. */
                if (strcmp(pToken, "ACTIVE") == 0)
                    _Cellular_PdnEventCallback(pContext, CELLULAR_URC_EVENT_PDN_ACTIVATED, contextId);
                else if (strcmp(pToken, "DEACTIVE") == 0)
                    _Cellular_PdnEventCallback(pContext, CELLULAR_URC_EVENT_PDN_DEACTIVATED, contextId);
            }
            else
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }
        else
        {
            atCoreStatus = CELLULAR_AT_ERROR;
            CellularLogError( "Error in processing Context Id. Token %s", pToken );
        }
    }

    pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t _parseSocketUrcDns( const CellularContext_t * pContext,
                                               char * pUrcStr )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    cellularModuleContext_t * pSimContext = NULL;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( pUrcStr == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        cellularStatus = _Cellular_GetModuleContext( pContext, ( void ** ) &pSimContext);

        if( cellularStatus != CELLULAR_SUCCESS )
        {
            pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
        }
    }

    if( pktStatus == CELLULAR_PKT_STATUS_OK )
    {
        if(pSimContext->dnsEventCallback != NULL )
        {
            pSimContext->dnsEventCallback(pSimContext, pUrcStr, pSimContext->pDnsUsrData );
        }
        else
        {
            CellularLogDebug( "_parseSocketUrcDns: spurious DNS response!!" );
            pktStatus = CELLULAR_PKT_STATUS_INVALID_DATA;
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessSocketDataInd(CellularContext_t* pContext, char* pInputLine)
{
    /*Handling: +CADATAIND: <cid>   */
    char*       pUrcStr = NULL;
    char*       pToken  = NULL;
    int32_t     socketId = -1;

    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularSocketContext_t*    pSocketData     = NULL;

    if (pContext == NULL || pInputLine == NULL)
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
        goto err;
    }

    pUrcStr = pInputLine;
    atCoreStatus = Cellular_ATRemoveLeadingWhiteSpaces(&pUrcStr);

    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pToken);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    atCoreStatus = Cellular_ATStrtoi(pToken, 10, &socketId);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    if (!IsValidPDP(socketId))
        goto err;

    pSocketData = _Cellular_GetSocketData(pContext, socketId);

    if (pSocketData != NULL)
    {
        if (pSocketData->dataMode == CELLULAR_ACCESSMODE_BUFFER)
        {
            /* Data received indication in buffer mode, need to fetch the data. */
            CellularLogDebug("Data Received on socket Conn Id %d", socketId);
            cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
            xEventGroupSetBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_IND);

            _informDataReadyToUpperLayer(pSocketData);
        }
    }
    else
    {
        atCoreStatus = CELLULAR_AT_ERROR;
    }

    if (atCoreStatus == CELLULAR_SUCCESS)
        return;

 err:
    _Cellular_TranslateAtCoreStatus(atCoreStatus);
    CellularLogDebug("SocketDataInd process failure");
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessSocketState(CellularContext_t* pContext, char* pInputLine)
{
    /*Handling: +CASTATE: <cid>,<state> */
    char*       pUrcStr = NULL;
    char*       pToken  = NULL;
    int32_t     socketId, socketState;

    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularSocketContext_t*    pSocketData     = NULL;

    if (pContext == NULL || pInputLine == NULL)
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
        goto err;
    }

    pUrcStr = pInputLine;
    atCoreStatus = Cellular_ATRemoveLeadingWhiteSpaces(&pUrcStr);

    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pToken);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    atCoreStatus = Cellular_ATStrtoi(pToken, 10, &socketId);
    if (atCoreStatus != CELLULAR_AT_SUCCESS )
        goto err;

    if( !IsValidCID((uint8_t)socketId ) )
    {
        CellularLogError("Error in processing Socket Index. Token %s", pToken);
        atCoreStatus = CELLULAR_AT_ERROR;
        goto err;
    }

    pSocketData = _Cellular_GetSocketData(pContext, socketId);

    if (pSocketData == NULL)
    {
        CellularLogError("Error can't get Socket data. socketId=%d", socketId);
        atCoreStatus = CELLULAR_AT_ERROR;
        goto err;
    }

    atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pToken);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    atCoreStatus = Cellular_ATStrtoi(pToken, 10, &socketState);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err;

    /*
        0 Closed by remote server or internal error
        1 Connected to remote server
        2 Listening (server mode)
    */
    pSocketData->socketState = socketState==0 ? SOCKETSTATE_DISCONNECTED : SOCKETSTATE_CONNECTED;
    CellularLogDebug("Socket %d. change state: %d", socketId, socketState);

    /* Indicate the upper layer about the socket close. */
    if (pSocketData->closedCallback != NULL)
    {
        pSocketData->closedCallback(pSocketData, pSocketData->pClosedCallbackContext);
    }
    else
    {
        CellularLogInfo("_parseSocketUrc: Socket close callback not set!!");
    }
    return;

err:
    _Cellular_TranslateAtCoreStatus(atCoreStatus);
    CellularLogDebug("SocketStateInd process failure");
}

/*-----------------------------------------------------------*/
/* Cellular common prototype. */
static void _Cellular_ProcessPdnStatus(CellularContext_t* pContext, char* pInputLine)
{
    //Handling; +APP PDP: <pdpidx>,<statusx>

    char*                       pUrcStr         = NULL;
    char*                       pIndex          = NULL;
    char*                       pStatus         = NULL;
    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;
    cellularModuleContext_t*    pSimContext     = NULL;
    int32_t                     socketId        = 0;

    if (pContext == NULL)
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
        goto err;
    }

    if (pInputLine == NULL)
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
        goto err;
    }

    pUrcStr = pInputLine;
    atCoreStatus = Cellular_ATRemoveLeadingWhiteSpaces(&pUrcStr);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err2;

    atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pIndex);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err2;

    atCoreStatus = Cellular_ATStrtoi(pIndex, 10, &socketId);
    if (atCoreStatus != CELLULAR_AT_SUCCESS || !IsValidCID(socketId))
        goto err2;

    pSimContext = (cellularModuleContext_t *)pContext->pModueContext;
    if (pSimContext == NULL)
    {
        CellularLogInfo("_Cellular_ProcessPdnStatus: get socket(%d) data(nul) failed", socketId);
        goto err2;
    }

    atCoreStatus = Cellular_ATGetNextTok(&pUrcStr, &pStatus);
    if (atCoreStatus != CELLULAR_AT_SUCCESS)
        goto err2;
    
    CellularLogInfo("Pdp Info: cid=%s, status=%s", pIndex, pStatus);

    xEventGroupSetBits( pSimContext->pdnEvent, CNACT_EVENT_BIT_ACT);

    return;
    
err2:
    pktStatus = _Cellular_TranslateAtCoreStatus(atCoreStatus);

err:
    if (pktStatus != CELLULAR_PKT_STATUS_OK)
        CellularLogDebug("+APP PDP: Parse failure");
}


/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessSimstat( CellularContext_t * pContext,
                                      char * pInputLine )
{
    CellularSimCardStatus_t simCardStuts = { CELLULAR_SIM_CARD_UNKNOWN, CELLULAR_SIM_CARD_LOCK_UNKNOWN };

    if( pContext != NULL )
    {
        ( void ) _Cellular_ParseSimstat( pInputLine, &simCardStuts );
    }
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessPowerDown( CellularContext_t * pContext,
                                        char * pInputLine )
{
    /* The token is the pInputLine. No need to process the pInputLine. */
    ( void ) pInputLine;

    if( pContext == NULL )
    {
        CellularLogError( "_Cellular_ProcessPowerDown: Context not set" );
    }
    else
    {
        CellularLogDebug( "_Cellular_ProcessPowerDown: Modem Power down event received" );
        _Cellular_ModemEventCallback( pContext, CELLULAR_MODEM_EVENT_POWERED_DOWN );
    }
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessPsmPowerDown( CellularContext_t * pContext,
                                           char * pInputLine )
{
    /* The token is the pInputLine. No need to process the pInputLine. */
    ( void ) pInputLine;

    if( pContext == NULL )
    {
        CellularLogError( "_Cellular_ProcessPowerDown: Context not set" );
    }
    else
    {
        CellularLogDebug( "_Cellular_ProcessPsmPowerDown: Modem PSM power down event received" );
        _Cellular_ModemEventCallback( pContext, CELLULAR_MODEM_EVENT_PSM_ENTER );
    }
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
static void _Cellular_ProcessModemRdy( CellularContext_t * pContext,
                                       char * pInputLine )
{
    /* The token is the pInputLine. No need to process the pInputLine. */
    ( void ) pInputLine;

    if( pContext == NULL )
    {
        CellularLogWarn( "_Cellular_ProcessModemRdy: Context not set" );
    }
    else
    {
        CellularLogDebug( "_Cellular_ProcessModemRdy: Modem Ready event received" );
        _Cellular_ModemEventCallback( pContext, CELLULAR_MODEM_EVENT_BOOTUP_OR_REBOOT );
    }
}

/*-----------------------------------------------------------*/

/* Cellular common prototype. */
CellularPktStatus_t _Cellular_ParseSimstat( char * pInputStr, CellularSimCardStatus_t* pSimStuts )
{
    //Handling +CPIN: <state>
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    char *              pToken          = NULL;
    char *              pLocalInputStr  = pInputStr;

    if( ( pInputStr == NULL ) || ( strlen( pInputStr ) == 0U ) ||
        ( strlen( pInputStr ) < 2U ) || (pSimStuts == NULL ) )
    {
        CellularLogError( "_Cellular_ProcessQsimstat Input data is invalid %s", pInputStr );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
#if 0
        atCoreStatus = Cellular_ATGetNextTok(&pLocalInputStr, &pToken);

        if (atCoreStatus == CELLULAR_AT_SUCCESS)
        {
            CellularLogDebug("URC Enable: %s", pToken);
            atCoreStatus = Cellular_ATGetNextTok(&pLocalInputStr, &pToken);
        }
#endif
        pToken = pLocalInputStr;

        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces(pToken);

        if (atCoreStatus == CELLULAR_AT_SUCCESS)
        {
            CellularLogDebug(" Sim status: %s", pToken);

            if (strcmp(pToken, "READY") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_READY;
            }
            else if (strcmp(pToken, "NOT READY") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_LOCK_UNKNOWN;
            }
            else if (strcmp(pToken, "SIM PIN") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PIN;
            }
            else if (strcmp(pToken, "SIM PUK") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PUK;
            }
            else if (strcmp(pToken, "PH_SIM PIN") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PH_NET_PIN;
            }
            else if (strcmp(pToken, "PH_SIM PUK") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PH_NET_PUK;
            }
            else if (strcmp(pToken, "PH_NET PIN") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PH_NETSUB_PIN;
            }
            else if (strcmp(pToken, "SIM PIN2") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PIN2;
            }
            else if (strcmp(pToken, "SIM PUK2") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_INSERTED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_PUK2;
            }
            else if (strcmp(pToken, "NOT INSERT") == 0)
            {
                pSimStuts->simCardState = CELLULAR_SIM_CARD_REMOVED;
                pSimStuts->simCardLockState = CELLULAR_SIM_CARD_LOCK_UNKNOWN;
            }
            else
            {
                CellularLogError("Error in processing SIM state. token %s", pToken);
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/
