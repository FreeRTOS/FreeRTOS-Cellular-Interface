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

/* Standard includes. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "cellular_platform.h"
#include "cellular_config.h"
#include "cellular_config_defaults.h"
#include "cellular_common_internal.h"

#include "cellular_types.h"
#include "cellular_api.h"
#include "cellular_common_api.h"
#include "cellular_common.h"
#include "cellular_at_core.h"
#include "cellular_sim70x0.h"

/*-----------------------------------------------------------*/

#define CELLULAR_AT_CMD_TYPICAL_MAX_SIZE           ( 32U )
#define CELLULAR_AT_CMD_QUERY_DNS_MAX_SIZE         ( 280U )

#define SIGNAL_QUALITY_POS_SYSMODE                 ( 1U )
#define SIGNAL_QUALITY_POS_GSM_LTE_RSSI            ( 2U )
#define SIGNAL_QUALITY_POS_LTE_RSRP                ( 3U )
#define SIGNAL_QUALITY_POS_LTE_SINR                ( 4U )
#define SIGNAL_QUALITY_POS_LTE_RSRQ                ( 5U )
#define SIGNAL_QUALITY_SINR_MIN_VALUE              ( -20 )
#define SIGNAL_QUALITY_SINR_DIVISIBILITY_FACTOR    ( 5 )

#define COPS_POS_MODE                              ( 1U )
#define COPS_POS_FORMAT                            ( 2U )
#define COPS_POS_MCC_MNC_OPER_NAME                 ( 3U )
#define COPS_POS_RAT                               ( 4U )

/* AT command timeout for Get IP Address by Domain Name. */
#define DNS_QUERY_TIMEOUT_MS                       ( 60000UL )

/* Length of HPLMN including RAT. */
#define CRSM_HPLMN_RAT_LENGTH                      ( 9U )

/* Windows simulator implementation. */
#if defined( _WIN32 ) || defined( _WIN64 )
    #define strtok_r                  strtok_s
#endif

#define PRINTF_BINARY_PATTERN_INT4    "%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT4( i )            \
    ( ( ( ( i ) & 0x08UL ) != 0UL ) ? '1' : '0' ), \
    ( ( ( ( i ) & 0x04UL ) != 0UL ) ? '1' : '0' ), \
    ( ( ( ( i ) & 0x02UL ) != 0UL ) ? '1' : '0' ), \
    ( ( ( ( i ) & 0x01UL ) != 0UL ) ? '1' : '0' )

#define PRINTF_BINARY_PATTERN_INT8 \
    PRINTF_BINARY_PATTERN_INT4 PRINTF_BINARY_PATTERN_INT4
#define PRINTF_BYTE_TO_BINARY_INT8( i ) \
    PRINTF_BYTE_TO_BINARY_INT4( ( i ) >> 4 ), PRINTF_BYTE_TO_BINARY_INT4( i )

#define QPSMS_POS_MODE                           ( 0U )
#define QPSMS_POS_RAU                            ( 1U )
#define QPSMS_POS_RDY_TIMER                      ( 2U )
#define QPSMS_POS_TAU                            ( 3U )
#define QPSMS_POS_ACTIVE_TIME                    ( 4U )

#define CELLULAR_PDN_STATUS_POS_CONTEXT_ID       ( 0U )
#define CELLULAR_PDN_STATUS_POS_CONTEXT_STATE    ( 1U )
#define CELLULAR_PDN_STATUS_POS_IP_ADDRESS       ( 2U )

#define RAT_PRIOIRTY_STRING_LENGTH               ( 2U )
#define RAT_PRIOIRTY_LIST_LENGTH                 ( 3U )

#define INVALID_PDN_INDEX                        ( 0xFFU )

/*-----------------------------------------------------------*/

/**
 * @brief Parameters involved in receiving data through sockets
 */
typedef struct _socketDataRecv
{
    uint32_t * pDataLen;
    uint8_t * pData;
    CellularSocketAddress_t * pRemoteSocketAddress;
} _socketDataRecv_t;

/*-----------------------------------------------------------*/

static bool _parseSignalQuality( char * pQcsqPayload,
                                 CellularSignalInfo_t * pSignalInfo );
static CellularPktStatus_t _Cellular_RecvFuncGetSignalInfo( CellularContext_t * pContext,
                                                            const CellularATCommandResponse_t * pAtResp,
                                                            void * pData,
                                                            uint16_t dataLen );
static CellularError_t controlSignalStrengthIndication( CellularContext_t * pContext,
                                                        bool enable );
static CellularPktStatus_t _Cellular_RecvFuncGetIccid( CellularContext_t * pContext,
                                                       const CellularATCommandResponse_t * pAtResp,
                                                       void * pData,
                                                       uint16_t dataLen );
static CellularPktStatus_t _Cellular_RecvFuncGetImsi( CellularContext_t * pContext,
                                                      const CellularATCommandResponse_t * pAtResp,
                                                      void * pData,
                                                      uint16_t dataLen );
static bool _checkCrsmMemoryStatus( const char * pToken );
static bool _checkCrsmReadStatus( const char * pToken );
static bool _parseHplmn( char * pToken,
                         void * pData );
static CellularPktStatus_t _Cellular_RecvFuncGetHplmn( CellularContext_t * pContext,
                                                       const CellularATCommandResponse_t * pAtResp,
                                                       void * pData,
                                                       uint16_t dataLen );
static CellularPktStatus_t _Cellular_RecvFuncGetSimCardStatus( CellularContext_t * pContext,
                                                               const CellularATCommandResponse_t * pAtResp,
                                                               void * pData,
                                                               uint16_t dataLen );
static CellularSimCardLockState_t _getSimLockState( char * pToken );
static CellularATError_t parsePdnStatusContextId( char * pToken,
                                                  CellularPdnStatus_t * pPdnStatusBuffers);
static CellularATError_t parsePdnStatusContextState( char * pToken,
                                                     CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t getPdnStatusParseToken( char * pToken,
                                                 uint8_t tokenIndex,
                                                 CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t getPdnStatusParseLine( char * pRespLine,
                                                CellularPdnStatus_t * pPdnStatusBuffers, uint8_t nBuf );
static CellularPktStatus_t _Cellular_RecvFuncGetPdnStatus( CellularContext_t * pContext,
                                                           const CellularATCommandResponse_t * pAtResp,
                                                           void * pData,
                                                           uint16_t dataLen );
static CellularError_t buildSocketConnect( CellularSocketHandle_t socketHandle,
                                           char * pCmdBuf );
static CellularATError_t getDataFromResp( const CellularATCommandResponse_t * pAtResp,
                                          const _socketDataRecv_t * pDataRecv,
                                          uint32_t outBufSize );
static CellularPktStatus_t _Cellular_RecvFuncData( CellularContext_t * pContext,
                                                   const CellularATCommandResponse_t * pAtResp,
                                                   void * pData,
                                                   uint16_t dataLen );
static CellularATError_t parseQpsmsMode( char * pToken,
                                         CellularPsmSettings_t * pPsmSettings );
static CellularATError_t parseQpsmsRau( char * pToken,
                                        CellularPsmSettings_t * pPsmSettings );
static CellularATError_t parseQpsmsRdyTimer( char * pToken,
                                             CellularPsmSettings_t * pPsmSettings );
static CellularATError_t parseQpsmsTau( char * pToken,
                                        CellularPsmSettings_t * pPsmSettings );
static CellularATError_t parseQpsmsActiveTime( char * pToken,
                                               CellularPsmSettings_t * pPsmSettings );
static CellularATError_t parseGetPsmToken( char * pToken,
                                           uint8_t tokenIndex,
                                           CellularPsmSettings_t * pPsmSettings );
static CellularRat_t convertRatPriority( char * pRatString );
static CellularPktStatus_t _Cellular_RecvFuncGetRatPriority( CellularContext_t * pContext,
                                                             const CellularATCommandResponse_t * pAtResp,
                                                             void * pData,
                                                             uint16_t dataLen );
static CellularPktStatus_t _Cellular_RecvFuncGetPsmSettings( CellularContext_t * pContext,
                                                             const CellularATCommandResponse_t * pAtResp,
                                                             void * pData,
                                                             uint16_t dataLen );
static CellularPktStatus_t socketRecvDataPrefix( void * pCallbackContext,
                                                 char * pLine,
                                                 uint32_t lineLength,
                                                 char ** ppDataStart,
                                                 uint32_t * pDataLength );
static CellularError_t storeAccessModeAndAddress( CellularContext_t * pContext,
                                                  CellularSocketHandle_t socketHandle,
                                                  CellularSocketAccessMode_t dataAccessMode,
                                                  const CellularSocketAddress_t * pRemoteSocketAddress );
static CellularError_t registerDnsEventCallback( cellularModuleContext_t * pModuleContext,
                                                 CellularDnsResultEventCallback_t dnsEventCallback,
                                                 char * pDnsUsrData );
static void _dnsResultCallback( cellularModuleContext_t * pModuleContext,
                                char * pDnsResult,
                                char * pDnsUsrData );
static uint32_t appendBinaryPattern( char * cmdBuf,
                                     uint32_t cmdLen,
                                     uint32_t value,
                                     bool endOfString );
static CellularPktStatus_t socketSendDataPrefix( void * pCallbackContext,
                                                 char * pLine,
                                                 uint32_t * pBytesRead );

/*-----------------------------------------------------------*/
/*
//+CPSI: <System Mode>,<Operation Mode>,<MCC>-<MNC>,<TAC>,<SCellID>,<PCellID>,<Frequency Band>,<earfcn>,<dlbw>,<ulbw>,<RSRQ>,<RSRP>,<RSSI>,<RSSNR>
//+CPSI: LTE CAT-M1,Online,440-52,0x6061,33815299,94,EUTRAN-BAND18,5900,3,3,-8,-84,-60,18
//+CPSI: LTE NB-IOT,Online,440-20,0x1182,10171378,293,EUTRAN-BAND8,3740,0,0,-12,-75,-63,13
*/
static bool _parseSignalQuality( char * pQcsqPayload,
                                 CellularSignalInfo_t * pSignalInfo )
{
    char *              pToken = NULL, * pTmpQcsqPayload = pQcsqPayload;
    int32_t             tempValue = 0;

    if( ( pSignalInfo == NULL ) || ( pQcsqPayload == NULL ) )
    {
        CellularLogError( "_parseSignalQuality: Invalid Input Parameters" );
        return false;
    }

    if (Cellular_ATGetNextTok(&pTmpQcsqPayload, &pToken) != CELLULAR_AT_SUCCESS)
    {   /*<System Mode> */
        CellularLogDebug("_parseSignalQuality: get <System Mode> failed");
        return false;
    }

    if( ( strcmp( pToken, "LTE CAT-M1" ) != 0 ) &&
        ( strcmp( pToken, "LTE NB-IOT" ) != 0 ) )
    {
        CellularLogDebug( "_parseSignalQuality: Unsupport <System Mode>" );
        return false;
    }

    if (Cellular_ATGetNextTok(&pTmpQcsqPayload, &pToken) != CELLULAR_AT_SUCCESS)
    {   /*<Operation Mode>  */
        CellularLogDebug("_parseSignalQuality: get <Operation Mode> failed");
        return false;
    }

    if( ( strcmp( pToken, "Online" ) != 0 )  )
    {
        CellularLogDebug("_parseSignalQuality: <Operation Mode>=%s", pToken);
        return false;
    }

    if( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<MCC>-<MNC>:      440-20          */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<TAC>:            0x1182          */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<SCellID>:        10171378        */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<PCellID>:        293             */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<Frequency Band>: EUTRAN-BAND8    */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<earfcn>:         3740            */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS   /*<dlbw>:           0               */
     || Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS)  /*<ulbw>:           0               */
    {
        CellularLogDebug( "_parseSignalQuality: CPSI Response not expected format." );
        return false;
    }
        
    if (Cellular_ATGetNextTok(&pTmpQcsqPayload, &pToken) != CELLULAR_AT_SUCCESS)
    {   /*<RSRQ>    */
        CellularLogDebug("_parseSignalQuality: get RSRQ failed");
        return false;
    }

    if (Cellular_ATStrtoi(pToken, 10, &tempValue) != CELLULAR_AT_SUCCESS)
    {
        CellularLogError("_parseSignalQuality: Error in processing RSRQ. Token %s", pToken);
        return false;
    }
    pSignalInfo->rsrq = ( int16_t ) tempValue;

    if (Cellular_ATGetNextTok(&pTmpQcsqPayload, &pToken) != CELLULAR_AT_SUCCESS)
    {   /*<RSRP>    */
        CellularLogDebug("_parseSignalQuality: get RSRP failed");
        return false;
    }
    if( Cellular_ATStrtoi( pToken, 10, &tempValue ) != CELLULAR_AT_SUCCESS )
    {
        CellularLogError("_parseSignalQuality: Error in processing RSRP. Token %s", pToken);
        return false;
    }
    pSignalInfo->rsrp = ( int16_t ) tempValue;

    if( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS )
    {   /*<RSSI>    */
        CellularLogDebug("_parseSignalQuality: get RSSI failed");
        return false;
    }
    if( Cellular_ATStrtoi( pToken, 10, &tempValue ) != CELLULAR_AT_SUCCESS )
    {
        CellularLogError("_parseSignalQuality: Error in processing RSSI. Token %s", pToken);
        return false;
    }
    pSignalInfo->rssi = ( int16_t ) tempValue;

    if( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) != CELLULAR_AT_SUCCESS )
    {   /*<RSSNR>   */
        CellularLogDebug("_parseSignalQuality: get RSSNR failed");
        return false;
    }
    if( Cellular_ATStrtoi( pToken, 10, &tempValue ) != CELLULAR_AT_SUCCESS )
    {
        CellularLogError("_parseSignalQuality: Error in processing SINR. pToken %s", pToken);
        return false;
    }
    /* SINR -20 dBm to +30 dBm. */
    pSignalInfo->sinr = (int16_t) (SIGNAL_QUALITY_SINR_MIN_VALUE + 10 * tempValue/SIGNAL_QUALITY_SINR_DIVISIBILITY_FACTOR);

    return true;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetSignalInfo( CellularContext_t * pContext,
                                                            const CellularATCommandResponse_t * pAtResp,
                                                            void * pData,
                                                            uint16_t dataLen )
{
    char * pInputLine = NULL;
    CellularSignalInfo_t * pSignalInfo = ( CellularSignalInfo_t * ) pData;
    bool parseStatus = true;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pSignalInfo == NULL ) || ( dataLen != sizeof( CellularSignalInfo_t ) ) )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        CellularLogError( "GetSignalInfo: Input Line passed is NULL" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemovePrefix( &pInputLine );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATRemoveAllDoubleQuote( pInputLine );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pInputLine );
        }

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    if( pktStatus == CELLULAR_PKT_STATUS_OK )
    {
        parseStatus = _parseSignalQuality( pInputLine, pSignalInfo );

        if( parseStatus != true )
        {
            pSignalInfo->rssi = CELLULAR_INVALID_SIGNAL_VALUE;
            pSignalInfo->rsrp = CELLULAR_INVALID_SIGNAL_VALUE;
            pSignalInfo->rsrq = CELLULAR_INVALID_SIGNAL_VALUE;
            pSignalInfo->ber = CELLULAR_INVALID_SIGNAL_VALUE;
            pSignalInfo->bars = CELLULAR_INVALID_SIGNAL_BAR_VALUE;
            pktStatus = CELLULAR_PKT_STATUS_FAILURE;
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularError_t controlSignalStrengthIndication( CellularContext_t * pContext,
                                                        bool enable )
{
    CellularError_t     cellularStatus  = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;
    uint8_t             enable_value    = 0;

    CellularAtReq_t atReqControlSignalStrengthIndication =
    {
        "AT+CPSI?",
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    enable_value = enable ? 1 : 0;

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqControlSignalStrengthIndication );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetIccid( CellularContext_t * pContext,
                                                       const CellularATCommandResponse_t * pAtResp,
                                                       void * pData,
                                                       uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pRespLine = NULL;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) ||
             ( pAtResp->pItm->pLine == NULL ) || ( pData == NULL ) )
    {
        CellularLogError( "getIccid: Response in invalid " );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pRespLine = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pRespLine );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* Storing the ICCID value in the AT Response. */
            if( strlen( pRespLine ) < ( ( size_t ) CELLULAR_ICCID_MAX_SIZE + 1U ) )
            {
                ( void ) strncpy( pData, pRespLine, dataLen );
            }
            else
            {
                atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
            }
        }

        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetImsi( CellularContext_t * pContext,
                                                      const CellularATCommandResponse_t * pAtResp,
                                                      void * pData,
                                                      uint16_t dataLen )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pRespLine = NULL;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) ||
             ( pAtResp->pItm->pLine == NULL ) || ( pData == NULL ) )
    {
        CellularLogError( "getImsi: Response in invalid" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pRespLine = pAtResp->pItm->pLine;

        /* Removing all the Spaces in the AT Response. */
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pRespLine );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            if( strlen( pRespLine ) < ( CELLULAR_IMSI_MAX_SIZE + 1U ) )
            {
                ( void ) strncpy( ( char * ) pData, pRespLine, dataLen );
            }
            else
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static bool _checkCrsmMemoryStatus( const char * pToken )
{
    bool memoryStatus = true;

    if( pToken == NULL )
    {
        CellularLogError( "Input Parameter NULL" );
        memoryStatus = false;
    }

    if( memoryStatus )
    {
        /* checking the value sw2 in AT command response for memory problem during CRSM read.
         * Refer 3GPP Spec TS 51.011 Section 9.4. */
        if( strcmp( pToken, "64" ) == 0 )
        {
            CellularLogError( "_checkCrsmMemoryStatus: Error in Processing HPLMN: CRSM Memory Error" );
            memoryStatus = false;
        }
    }

    return memoryStatus;
}

/*-----------------------------------------------------------*/

static bool _checkCrsmReadStatus( const char * pToken )
{
    bool readStatus = true;

    if( pToken == NULL )
    {
        CellularLogError( "Input Parameter NULL" );
        readStatus = false;
    }

    if( readStatus )
    {
        /* checking the parameter sw1 in AT command response for successful CRSM read.
         * Refer 3GPP Spec TS 51.011 Section 9.4. */
        if( ( strcmp( pToken, "144" ) != 0 ) &&
            ( strcmp( pToken, "145" ) != 0 ) &&
            ( strcmp( pToken, "146" ) != 0 ) )
        {
            CellularLogError( "_checkCrsmReadStatus: Error in Processing HPLMN: CRSM Read Error" );
            readStatus = false;
        }
    }

    return readStatus;
}

/*-----------------------------------------------------------*/

static bool _parseHplmn( char * pToken,
                         void * pData )
{
    bool parseStatus = true;
    CellularPlmnInfo_t * plmn = ( CellularPlmnInfo_t * ) pData;

    if( ( pToken == NULL ) || ( pData == NULL ) )
    {
        CellularLogError( "Input Parameter NULL" );
        parseStatus = false;
    }

    if( parseStatus == true )
    {
        /* Checking if the very first HPLMN entry in AT command Response is valid*/
        if( ( strlen( pToken ) < ( CRSM_HPLMN_RAT_LENGTH ) ) || ( strncmp( pToken, "FFFFFF", 6 ) == 0 ) )
        {
            CellularLogError( "_parseHplmn: Error in Processing HPLMN: Invalid Token %s", pToken );
            parseStatus = false;
        }
    }

    if( parseStatus == true )
    {
        /* Returning only the very first HPLMN present in EFHPLMNwACT in SIM.
         * EF-HPLMNwACT can contain a maximum of 10 HPLMN entries in decreasing order of priority.
         * In this implementation, returning the very first HPLMN is the PLMN priority list. */
        /* Refer TS 51.011 Section 10.3.37 for encoding. */
        plmn->mcc[ 0 ] = pToken[ 1 ];
        plmn->mcc[ 1 ] = pToken[ 0 ];
        plmn->mcc[ 2 ] = pToken[ 3 ];
        plmn->mnc[ 0 ] = pToken[ 5 ];
        plmn->mnc[ 1 ] = pToken[ 4 ];

        if( pToken[ 2 ] != 'F' )
        {
            plmn->mnc[ 2 ] = pToken[ 2 ];
            plmn->mnc[ 3 ] = '\0';
        }
        else
        {
            plmn->mnc[ 2 ] = '\0';
        }
    }

    return parseStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetHplmn( CellularContext_t * pContext,
                                                       const CellularATCommandResponse_t * pAtResp,
                                                       void * pData,
                                                       uint16_t dataLen )
{
    bool parseStatus = true;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pCrsmResponse = NULL, * pToken = NULL;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) ||
             ( pData == NULL ) || ( dataLen != sizeof( CellularPlmnInfo_t ) ) )
    {
        CellularLogError( "GetHplmn: Response is invalid " );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pCrsmResponse = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pCrsmResponse );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* Removing the CRSM prefix in AT Response. */
            atCoreStatus = Cellular_ATRemovePrefix( &pCrsmResponse );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* Removing All quotes in the AT Response. */
            atCoreStatus = Cellular_ATRemoveAllDoubleQuote( pCrsmResponse );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* Getting the next token separated by comma in At Response*/
            atCoreStatus = Cellular_ATGetNextTok( &pCrsmResponse, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            parseStatus = _checkCrsmReadStatus( pToken );

            if( !parseStatus )
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pCrsmResponse, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            parseStatus = _checkCrsmMemoryStatus( pToken );

            if( !parseStatus )
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pCrsmResponse, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            parseStatus = _parseHplmn( pToken, pData );

            if( !parseStatus )
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetSimCardStatus( CellularContext_t * pContext,
                                                               const CellularATCommandResponse_t * pAtResp,
                                                               void * pData,
                                                               uint16_t dataLen )
{
    char *                      pInputLine      = NULL;
    const char *                pTokenPtr       = NULL;
    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        CellularLogError( "GetSimStatus: response is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pData == NULL ) || ( dataLen != sizeof( CellularSimCardState_t ) ) )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemoveLeadingWhiteSpaces( &pInputLine );
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            /* remove the token prefix. */
            pTokenPtr = strtok_r( pInputLine, ":", &pInputLine );

            /* check the token prefix. */
            if( pTokenPtr == NULL )
            {
                pktStatus = CELLULAR_PKT_STATUS_BAD_RESPONSE;
            }
            else
            {
                pktStatus = _Cellular_ParseSimstat( pInputLine, (CellularSimCardStatus_t*)pData);
            }
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularSimCardLockState_t _getSimLockState( char * pToken )
{
    CellularSimCardLockState_t tempState = CELLULAR_SIM_CARD_LOCK_UNKNOWN;

    if( pToken != NULL )
    {
        if( strcmp( pToken, "READY" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_READY;
        }
        else if( strcmp( pToken, "SIM PIN" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PIN;
        }
        else if( strcmp( pToken, "SIM PUK" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PUK;
        }
        else if( strcmp( pToken, "SIM PIN2" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PIN2;
        }
        else if( strcmp( pToken, "SIM PUK2" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PUK2;
        }
        else if( strcmp( pToken, "PH-NET PIN" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PH_NET_PIN;
        }
        else if( strcmp( pToken, "PH-NET PUK" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PH_NET_PUK;
        }
        else if( strcmp( pToken, "PH-NETSUB PIN" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PH_NETSUB_PIN;
        }
        else if( strcmp( pToken, "PH-NETSUB PUK" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_PH_NETSUB_PUK;
        }
        else if( strcmp( pToken, "PH-SP PIN" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_SP_PIN;
        }
        else if( strcmp( pToken, "PH-SP PUK" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_SP_PUK;
        }
        else if( strcmp( pToken, "PH-CORP PIN" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_CORP_PIN;
        }
        else if( strcmp( pToken, "PH-CORP PUK" ) == 0 )
        {
            tempState = CELLULAR_SIM_CARD_CORP_PUK;
        }
        else
        {
            CellularLogError( "Unknown SIM Lock State %s", pToken );
        }
    }

    return tempState;
}


/*-----------------------------------------------------------*/

static CellularATError_t parsePdnStatusContextId( char * pToken,
                                                  CellularPdnStatus_t * pPdnStatusBuffers)
{
    int32_t             tempValue = 0;
    CellularATError_t   atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( IsValidPDP(tempValue)  )
        {
            pPdnStatusBuffers->contextId = (uint8_t) tempValue;
        }
        else
        {
            CellularLogError( "Error in Processing Context Id. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parsePdnStatusContextState( char * pToken,
                                                     CellularPdnStatus_t * pPdnStatusBuffers )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue >= 0 && tempValue <= (int32_t)UINT8_MAX )
        {
            pPdnStatusBuffers->state = (uint8_t) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing PDN Status Buffer state. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getPdnStatusParseToken( char * pToken,
                                                 uint8_t tokenIndex,
                                                 CellularPdnStatus_t *pPdnStatusBuffers )
{
    /*Handling: +CNACT: <pdpidx>,<statusx>,<addressx>   */
    CellularATError_t   atCoreStatus = CELLULAR_AT_SUCCESS;

    switch( tokenIndex )
    {
    case ( CELLULAR_PDN_STATUS_POS_CONTEXT_ID ):
        CellularLogDebug( "Context Id: %s", pToken );
        atCoreStatus = parsePdnStatusContextId(pToken, pPdnStatusBuffers);
        break;

    case ( CELLULAR_PDN_STATUS_POS_CONTEXT_STATE ):
        CellularLogDebug( "Context State: %s", pToken );
        atCoreStatus = parsePdnStatusContextState( pToken, pPdnStatusBuffers );
        break;

    case ( CELLULAR_PDN_STATUS_POS_IP_ADDRESS ):
        CellularLogDebug( "IP address: %s", pToken );
        memset(pPdnStatusBuffers->ipAddress.ipAddress, 0, sizeof(pPdnStatusBuffers->ipAddress.ipAddress));
        strncpy( pPdnStatusBuffers->ipAddress.ipAddress, pToken,
            min(sizeof(pPdnStatusBuffers->ipAddress.ipAddress), strlen(pToken)) );

        pPdnStatusBuffers->pdnContextType = CELLULAR_PDN_CONTEXT_TYPE;
        
        pPdnStatusBuffers->ipAddress.ipAddressType  = pPdnStatusBuffers->pdnContextType == CELLULAR_PDN_CONTEXT_IPV4
                                                    ? CELLULAR_IP_ADDRESS_V4
                                                    : CELLULAR_IP_ADDRESS_V6;
        break;

    default:
        CellularLogError( "Unknown token in getPdnStatusParseToken %s %d",
                        pToken, tokenIndex );
        atCoreStatus = CELLULAR_AT_ERROR;
        break;
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getPdnStatusParseLine( char * pRespLine,
                                                CellularPdnStatus_t * pPdnStatusBuffers, uint8_t nBuf )
{
    /*Handling: +CNACT: <pdpidx>,<statusx>,<addressx>   */
    char *              pToken          = NULL;
    char *              pLocalRespLine  = pRespLine;
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    uint8_t             tokenIndex      = 0;

    atCoreStatus = Cellular_ATRemovePrefix( &pLocalRespLine );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATRemoveAllDoubleQuote( pLocalRespLine );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        atCoreStatus = Cellular_ATGetNextTok( &pLocalRespLine, &pToken );
    }

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        CellularPdnStatus_t pdnStatus;

        memset(&pdnStatus, 0, sizeof(pdnStatus));
        for (tokenIndex = 0; pToken != NULL; tokenIndex++ )
        {
            atCoreStatus = getPdnStatusParseToken( pToken, tokenIndex, &pdnStatus );

            if( atCoreStatus != CELLULAR_AT_SUCCESS )
            {
                CellularLogDebug( "getPdnStatusParseToken %s index %u failed", pToken, tokenIndex );
                break;
            }

            if (Cellular_ATGetNextTok(&pLocalRespLine, &pToken) != CELLULAR_AT_SUCCESS)
                pToken = NULL;
        }

        if (pdnStatus.contextId >= 0 && pdnStatus.contextId < nBuf)
        {
            if (IsValidPDP(pdnStatus.contextId))
                pPdnStatusBuffers[pdnStatus.contextId] = pdnStatus;
        }
        else
        {
            CellularLogDebug("getPdnStatusParseToken buffer(%u) is too small, ContextId=%u.",
                nBuf, pdnStatus.contextId );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetPdnStatus( CellularContext_t * pContext,
                                                           const CellularATCommandResponse_t * pAtResp,
                                                           void * pData,
                                                           uint16_t dataLen )
{
    CellularPktStatus_t             pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularATError_t               atCoreStatus    = CELLULAR_AT_SUCCESS;
    const CellularATCommandLine_t * pItem           = NULL;

    if( pContext == NULL )
    {
        CellularLogError( "GetPdnStatus: invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) )
    {
        CellularLogError( "GetPdnStatus: Response is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pData == NULL ) || ( dataLen < 1U ) )
    {
        CellularLogError( "GetPdnStatus: PDN Status bad parameters" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        CellularLogError( "GetPdnStatus: no activated PDN" );
        pktStatus = CELLULAR_PKT_STATUS_OK;
    }
    else
    {
        for (pItem = pAtResp->pItm; pItem != NULL; pItem = pItem->pNext )
        {
            atCoreStatus = getPdnStatusParseLine(pItem->pLine, (CellularPdnStatus_t *)pData, dataLen);
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                CellularLogError( "getPdnStatusParseLine parse %s failed", pItem->pLine);
                break;
            }
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularError_t buildSocketConnect( CellularSocketHandle_t socketHandle,
                                           char * pCmdBuf )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    const char *protocol = "TCP";

    if( pCmdBuf == NULL )
    {
        CellularLogError( "buildSocketConnect: Invalid command buffer" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        if( socketHandle->socketProtocol == CELLULAR_SOCKET_PROTOCOL_UDP )
            protocol = "UDP";

        (void)snprintf( pCmdBuf, CELLULAR_AT_CMD_MAX_SIZE, "AT+CAOPEN=%d,%ld,\"%s\",\"%s\",%d",
                socketHandle->socketId,     /* 0-12*/
                socketHandle->contextId,    /* 0-4*/
                protocol, 
                socketHandle->remoteSocketAddress.ipAddress.ipAddress,
                socketHandle->remoteSocketAddress.port );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getDataFromResp( const CellularATCommandResponse_t * pAtResp,
                                          const _socketDataRecv_t * pDataRecv,
                                          uint32_t outBufSize )
{
    CellularATError_t   atCoreStatus    = CELLULAR_AT_SUCCESS;
    const char *        pInputLine      = NULL;
    uint32_t            dataLenToCopy   = 0;

    /* Check if the received data size is greater than the output buffer size. */
    if( *pDataRecv->pDataLen > outBufSize )
    {
        CellularLogError( "Data is turncated, received data length %d, out buffer size %d",
                     *pDataRecv->pDataLen, outBufSize );
        dataLenToCopy = outBufSize;
        *pDataRecv->pDataLen = outBufSize;
    }
    else
    {
        dataLenToCopy = *pDataRecv->pDataLen;
    }

    /*handling: +CARECV: <len>\0<data>   */

    /* Data is stored in the next intermediate response. */
    if( pAtResp->pItm != NULL && pAtResp->pItm->pNext != NULL )
    {
        pInputLine = pAtResp->pItm->pNext->pLine;

        if ((pInputLine != NULL) && (dataLenToCopy > 0U))
        {
            /* Copy the data to the out buffer. */
            (void)memcpy(pDataRecv->pData, pInputLine, dataLenToCopy);
        }
        else
        {
            CellularLogError("Receive Data: Data pointer NULL");
            atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
        }
    }
    else if( *pDataRecv->pDataLen == 0U )
    {
        /* Receive command success but no data. */
        CellularLogDebug( "Receive Data: no data" );
    }
    else
    {
        CellularLogError( "Receive Data: Intermediate response empty" );
        atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncData( CellularContext_t * pContext,
                                                   const CellularATCommandResponse_t * pAtResp,
                                                   void * pData,
                                                   uint16_t dataLen )
{
    /*copy +CARECV: <len> / data to recv buffer */
    CellularATError_t           atCoreStatus    = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t         pktStatus       = CELLULAR_PKT_STATUS_OK;
    char*                       pInputLine      = NULL;
    char *                      pToken          = NULL;
    const _socketDataRecv_t *   pDataRecv       = ( _socketDataRecv_t * ) pData;
    int32_t                     nRecvCnt        = 0;

    if( pContext == NULL )
    {
        CellularLogError( "Receive Data: invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        CellularLogError( "Receive Data: response is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pDataRecv == NULL ) || ( pDataRecv->pData == NULL ) || ( pDataRecv->pDataLen == NULL ) )
    {
        CellularLogError( "Receive Data: Bad param" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemovePrefix( &pInputLine );

        /* parse the datalen. */
        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATStrtoi( pToken, 10, &nRecvCnt);

            if (atCoreStatus == CELLULAR_AT_SUCCESS
                && nRecvCnt >= 0 && nRecvCnt <= CELLULAR_MAX_RECV_DATA_LEN)
            {

                *pDataRecv->pDataLen = nRecvCnt;

                if (nRecvCnt == 0)
                {
                    /* all data received, next AT+CARECV need +CADATAIND: */
                    cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
                    xEventGroupClearBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_IND);
                }
            }
            else
            {
                CellularLogError("Error in Data Length Processing: No valid digit found. Token %s", pToken);
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        /* Process the data buffer. */
        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = getDataFromResp( pAtResp, pDataRecv, dataLen );
        }

        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseQpsmsMode( char * pToken,
                                         CellularPsmSettings_t * pPsmSettings )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( tempValue >= 0 ) && ( tempValue <= ( int32_t ) UINT8_MAX ) )
        {
            pPsmSettings->mode = ( uint8_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing mode. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseQpsmsRau( char * pToken,
                                        CellularPsmSettings_t * pPsmSettings )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue >= 0 )
        {
            pPsmSettings->periodicRauValue = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing Periodic Processing RAU value. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseQpsmsRdyTimer( char * pToken,
                                             CellularPsmSettings_t * pPsmSettings )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue >= 0 )
        {
            pPsmSettings->gprsReadyTimer = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing Periodic Processing GPRS Ready Timer value. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseQpsmsTau( char * pToken,
                                        CellularPsmSettings_t * pPsmSettings )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue >= 0 )
        {
            pPsmSettings->periodicTauValue = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing Periodic TAU value value. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseQpsmsActiveTime( char * pToken,
                                               CellularPsmSettings_t * pPsmSettings )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( tempValue >= 0 )
        {
            pPsmSettings->activeTimeValue = ( uint32_t ) tempValue;
        }
        else
        {
            CellularLogError( "Error in processing Periodic Processing Active time value. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parseGetPsmToken( char * pToken,
                                           uint8_t tokenIndex,
                                           CellularPsmSettings_t * pPsmSettings )
{
    /*Handling: +CPSMS: <mode>,[<Requested_Periodic-RAU>],[<Requested_GPRS-READY-timer>],[<Requested_Periodic-TAU>],[<Requested_Active-Time>]   */
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    switch( tokenIndex )
    {
        case QPSMS_POS_MODE:
            atCoreStatus = parseQpsmsMode( pToken, pPsmSettings );
            break;

        case QPSMS_POS_RAU:
            atCoreStatus = parseQpsmsRau( pToken, pPsmSettings );
            break;

        case QPSMS_POS_RDY_TIMER:
            atCoreStatus = parseQpsmsRdyTimer( pToken, pPsmSettings );
            break;

        case QPSMS_POS_TAU:
            atCoreStatus = parseQpsmsTau( pToken, pPsmSettings );
            break;

        case QPSMS_POS_ACTIVE_TIME:
            atCoreStatus = parseQpsmsActiveTime( pToken, pPsmSettings );
            break;

        default:
            CellularLogDebug( "Unknown Parameter Position in AT+QPSMS Response" );
            atCoreStatus = CELLULAR_AT_ERROR;
            break;
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularRat_t convertRatPriority( char * pRatString )
{
    CellularRat_t retRat = CELLULAR_RAT_INVALID;

    if( strncmp( pRatString, "01", RAT_PRIOIRTY_STRING_LENGTH ) == 0 )
    {
        retRat = CELLULAR_RAT_GSM;
    }
    else if( strncmp( pRatString, "02", RAT_PRIOIRTY_STRING_LENGTH ) == 0 )
    {
        retRat = CELLULAR_RAT_CATM1;
    }
    else if( strncmp( pRatString, "03", RAT_PRIOIRTY_STRING_LENGTH ) == 0 )
    {
        retRat = CELLULAR_RAT_NBIOT;
    }
    else
    {
        CellularLogDebug( "Invalid RAT string %s", pRatString );
    }

    return retRat;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetRatPriority( CellularContext_t * pContext,
                                                             const CellularATCommandResponse_t * pAtResp,
                                                             void * pData,
                                                             uint16_t dataLen )
{
    char * pInputLine = NULL, * pToken = NULL;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularRat_t * pRatPriorities = NULL;
    char pTempString[ RAT_PRIOIRTY_STRING_LENGTH + 1 ] = { "\0" }; /* The return RAT has two chars plus NULL char. */
    uint32_t ratIndex = 0;
    uint32_t maxRatPriorityLength = ( dataLen > RAT_PRIOIRTY_LIST_LENGTH ? RAT_PRIOIRTY_LIST_LENGTH : dataLen );

    if( pContext == NULL )
    {
        CellularLogError( "GetRatPriority: Invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) ||
             ( pAtResp->pItm->pLine == NULL ) || ( pData == NULL ) || ( dataLen == 0U ) )
    {
        CellularLogError( "GetRatPriority: Invalid param" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        pRatPriorities = ( CellularRat_t * ) pData;

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
        }

        /* Response string 020301 => pToken : 020301, pInputLine : NULL. */
        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            if( strlen( pToken ) != ( RAT_PRIOIRTY_STRING_LENGTH * RAT_PRIOIRTY_LIST_LENGTH ) )
            {
                atCoreStatus = CELLULAR_AT_ERROR;
            }
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            memset( pRatPriorities, CELLULAR_RAT_INVALID, dataLen );

            for( ratIndex = 0; ratIndex < maxRatPriorityLength; ratIndex++ )
            {
                memcpy( pTempString, &pToken[ ratIndex * RAT_PRIOIRTY_STRING_LENGTH ], RAT_PRIOIRTY_STRING_LENGTH );
                pRatPriorities[ ratIndex ] = convertRatPriority( pTempString );
            }
        }

        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
static CellularPktStatus_t _Cellular_RecvFuncGetPsmSettings( CellularContext_t * pContext,
                                                             const CellularATCommandResponse_t * pAtResp,
                                                             void * pData,
                                                             uint16_t dataLen )
{
    /*Handling: +CPSMS: <mode>,[<Requested_Periodic-RAU>],[<Requested_GPRS-READY-timer>],[<Requested_Periodic-TAU>],[<Requested_Active-Time>]   */
    char*                   pInputLine   = NULL;
    char*                   pToken       = NULL;
    uint8_t                 tokenIndex   = 0;
    CellularPktStatus_t     pktStatus    = CELLULAR_PKT_STATUS_OK;
    CellularATError_t       atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularPsmSettings_t * pPsmSettings = NULL;

    if( pContext == NULL )
    {
        CellularLogError( "GetPsmSettings: Invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) ||
             ( pAtResp->pItm->pLine == NULL ) || ( pData == NULL ) || ( dataLen != sizeof( CellularPsmSettings_t ) ) )
    {
        CellularLogError( "GetPsmSettings: Invalid param" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        pPsmSettings = ( CellularPsmSettings_t * ) pData;
        atCoreStatus = Cellular_ATRemovePrefix( &pInputLine );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATRemoveAllDoubleQuote( pInputLine );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            tokenIndex = 0;

            while( pToken != NULL )
            {
                atCoreStatus = parseGetPsmToken( pToken, tokenIndex, pPsmSettings );

                if( atCoreStatus != CELLULAR_AT_SUCCESS )
                {
                    CellularLogInfo( "parseGetPsmToken %s index %d failed", pToken, tokenIndex );
                }

                tokenIndex++;

                if( *pInputLine == ',' )
                {
                    *pInputLine = '\0';
                    pToken      = pInputLine;
                    *pToken     = '\0';
                    pInputLine  = &pInputLine[ 1 ];
                }
                else if( Cellular_ATGetNextTok( &pInputLine, &pToken ) != CELLULAR_AT_SUCCESS )
                {
                    break;
                }
                else
                {
                    /* Empty Else MISRA 15.7 */
                }
            }
        }

        CellularLogDebug( "PSM setting: mode: %d, RAU: %d, RDY_Timer: %d, TAU: %d, Active_time: %d",
                     pPsmSettings->mode,
                     pPsmSettings->periodicRauValue,
                     pPsmSettings->gprsReadyTimer,
                     pPsmSettings->periodicTauValue,
                     pPsmSettings->activeTimeValue );
        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

                                                /* ..............0  */
#define     MAX_CARECV_STRING_PREFIX_STRING 15  /* +CARECV: 1459,   */   

static CellularPktStatus_t socketRecvDataPrefix( void * pCallbackContext,
                                                 char * pLine,
                                                 uint32_t lineLength,
                                                 char ** ppDataStart,
                                                 uint32_t * pDataLength )
{
    CellularContext_t*  pContext = (CellularContext_t *)pCallbackContext;
    char                sPrefix[MAX_CARECV_STRING_PREFIX_STRING];

    /* Handling: +CARECV: 1459,<data>    */
    if ((pLine == NULL) || (ppDataStart == NULL) || (pDataLength == NULL))
    {
        CellularLogError("Data prefix Bad Param(nul point)");
        return CELLULAR_PKT_STATUS_BAD_PARAM;
    }

    char    *pData = strchr( pLine, ',' );
    char    *pEos;
    char    *pPrefix;
    char    *pToken;

    *pDataLength = 0;
    *ppDataStart = NULL;

    if (pData != NULL
     && pData - pLine < MAX_CARECV_STRING_PREFIX_STRING
     && strncmp(pLine, "+CARECV:", 8) == 0)
        goto ok;        /* matched +CARECV: 1459,<data>    */

    /*
    * OK
    * +CADATAIND: 0
    * +CARECV: 0
    */
    CellularPktStatus_t     pktStatus = CELLULAR_PKT_STATUS_PREFIX_MISMATCH;

    pData = pLine;
    while ( pData - pLine < (int)lineLength
        && (pEos = strchr(pData, '\r')) != NULL
        && pEos - pLine < (int)lineLength)
    {
        int     nLen = min(sizeof(sPrefix) - 1, pEos - pData);
        strncpy(sPrefix, pData, nLen);
        sPrefix[nLen] = 0;

        pData = ++pEos;
        while (*pData == '\n' || *pData == '\r')
            pData++;

        pPrefix = sPrefix;
        Cellular_ATRemoveAllWhiteSpaces(pPrefix);

        if (strlen(pPrefix) <= 0)
            continue;   /* empty line   */

        /* Check if the message is a data related response. */
        if (strcmp(pPrefix, "OK") == 0
         || strncmp(pPrefix, "+CADATAIND:", 11) == 0)
        {
            CellularLogDebug("%s recieved. just ignore", pPrefix);
            pktStatus = CELLULAR_PKT_STATUS_OK;
            continue;
        }

        if (strcmp(pPrefix, "+CARECV:0") == 0 )
        {
            CellularLogDebug("%s recieved. no more data", pPrefix);
            cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
            xEventGroupClearBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_IND);    /* recv data empty, need wait +CADATAIND: */
            pktStatus = CELLULAR_PKT_STATUS_OK;
            continue;
        }

        CellularLogError("not match(+CARECV).line: %s", sPrefix);
        pktStatus = CELLULAR_PKT_STATUS_PREFIX_MISMATCH;
    }

    return pktStatus;

ok: /* +CARECV: 1459,<data> come here */
    pPrefix = sPrefix;
    strncpy(sPrefix, pLine, pData - pLine);
    sPrefix[pData - pLine] = 0;

    if (CELLULAR_AT_SUCCESS != Cellular_ATRemovePrefix(&pPrefix))
    {
        CellularLogError("remove prefix(+CARECV) failed: %s", sPrefix);
        return CELLULAR_PKT_STATUS_PREFIX_MISMATCH;
    }

    if (CELLULAR_AT_SUCCESS != Cellular_ATGetNextTok(&pPrefix, &pToken))
    {
        CellularLogError("get recv length failed: %s", sPrefix);
        return CELLULAR_PKT_STATUS_PREFIX_MISMATCH;
    }

    int32_t nRecvCnt = 0;

    if (CELLULAR_AT_SUCCESS != Cellular_ATStrtoi(pToken, 10, &nRecvCnt))
    {
        CellularLogError("convert recv length failed: %s", pToken);
        return CELLULAR_PKT_STATUS_PREFIX_MISMATCH;
    }

    if (nRecvCnt < 0 || nRecvCnt > CELLULAR_MAX_RECV_DATA_LEN)
    {
        CellularLogError("Data response received with wrong size: %d", nRecvCnt);
        return CELLULAR_PKT_STATUS_PREFIX_MISMATCH;
    }

    if( lineLength < (uint32_t)(nRecvCnt +  pData - pLine) )    /*lineLength not enguth */
    {
        /* More data is required. */
        CellularLogDebug("need more data %u < %u", lineLength, (uint32_t)nRecvCnt+pData-pLine);
        return  CELLULAR_PKT_STATUS_SIZE_MISMATCH;      /* Modem recv continue*/
    }

    *pDataLength    = nRecvCnt;
    *pData++        = 0;      /* current line become +CARECV: <len>\0       */
    *ppDataStart    = pData;  /* pData will be saved to pResp->pItm->next   */
    CellularLogDebug( "Data: %p length: %d saved to nex pResp", pData, nRecvCnt);

    return CELLULAR_PKT_STATUS_OK;
}

/*-----------------------------------------------------------*/

static CellularError_t storeAccessModeAndAddress( CellularContext_t * pContext,
                                                  CellularSocketHandle_t socketHandle,
                                                  CellularSocketAccessMode_t dataAccessMode,
                                                  const CellularSocketAddress_t * pRemoteSocketAddress )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pRemoteSocketAddress == NULL ) || ( socketHandle == NULL ) )
    {
        CellularLogError( "storeAccessModeAndAddress: Invalid socket address" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( socketHandle->socketState != SOCKETSTATE_ALLOCATED )
    {
        CellularLogError( "storeAccessModeAndAddress, bad socket state %d",
                     socketHandle->socketState );
        cellularStatus = CELLULAR_INTERNAL_FAILURE;
    }
    else if( dataAccessMode != CELLULAR_ACCESSMODE_BUFFER )
    {
        CellularLogError( "storeAccessModeAndAddress, Access mode not supported %d",
                     dataAccessMode );
        cellularStatus = CELLULAR_UNSUPPORTED;
    }
    else
    {
        socketHandle->remoteSocketAddress.port = pRemoteSocketAddress->port;
        socketHandle->dataMode = dataAccessMode;
        socketHandle->remoteSocketAddress.ipAddress.ipAddressType =
            pRemoteSocketAddress->ipAddress.ipAddressType;
        ( void ) strncpy( socketHandle->remoteSocketAddress.ipAddress.ipAddress,
                          pRemoteSocketAddress->ipAddress.ipAddress,
                          CELLULAR_IP_ADDRESS_MAX_SIZE + 1U );
    }

    return cellularStatus;
}


/*-----------------------------------------------------------*/

static CellularError_t registerDnsEventCallback( cellularModuleContext_t * pModuleContext,
                                                 CellularDnsResultEventCallback_t dnsEventCallback,
                                                 char * pDnsUsrData )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    if( pModuleContext == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        pModuleContext->dnsEventCallback = dnsEventCallback;
        pModuleContext->pDnsUsrData = pDnsUsrData;
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

static void _dnsResultCallback( cellularModuleContext_t * pModuleContext,
                                char * pDnsResult,
                                char * pDnsUsrData )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pToken = NULL, * pDnsResultStr = pDnsResult;
    int32_t dnsResultNumber = 0;
    cellularDnsQueryResult_t dnsQueryResult = CELLULAR_DNS_QUERY_UNKNOWN;

    if( pModuleContext != NULL )
    {
        if( pModuleContext->dnsResultNumber == ( uint8_t ) 0 )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pDnsResultStr, &pToken );

            if( atCoreStatus == CELLULAR_AT_SUCCESS )
            {
                atCoreStatus = Cellular_ATGetNextTok( &pDnsResultStr, &pToken );
            }

            if( atCoreStatus == CELLULAR_AT_SUCCESS )
            {
                atCoreStatus = Cellular_ATStrtoi( pToken, 10, &dnsResultNumber );

                if( ( atCoreStatus == CELLULAR_AT_SUCCESS ) && ( dnsResultNumber >= 0 ) &&
                    ( dnsResultNumber <= ( int32_t ) UINT8_MAX ) )
                {
                    pModuleContext->dnsResultNumber = ( uint8_t ) dnsResultNumber;
                }
                else
                {
                    CellularLogDebug( "_dnsResultCallback convert string failed %s", pToken );
                }
            }
        }
        else if( ( pModuleContext->dnsIndex < pModuleContext->dnsResultNumber ) && ( pDnsResultStr != NULL ) )
        {
            pModuleContext->dnsIndex = pModuleContext->dnsIndex + ( uint8_t ) 1;

            ( void ) strncpy( pDnsUsrData, pDnsResultStr, CELLULAR_IP_ADDRESS_MAX_SIZE );
            ( void ) registerDnsEventCallback( pModuleContext, NULL, NULL );
            dnsQueryResult = CELLULAR_DNS_QUERY_SUCCESS;

            if( xQueueSend( pModuleContext->pktDnsQueue, &dnsQueryResult, ( TickType_t ) 0 ) != pdPASS )
            {
                CellularLogDebug( "_dnsResultCallback sends pktDnsQueue fail" );
            }
        }
        else
        {
            CellularLogDebug( "_dnsResultCallback spurious DNS response" );
        }
    }
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SetRatPriority( CellularHandle_t cellularHandle,
                                         const CellularRat_t * pRatPriorities,
                                         uint8_t ratPrioritiesLength )
{
    UNREFERENCED_PARAMETER(cellularHandle);
    UNREFERENCED_PARAMETER(pRatPriorities);
    UNREFERENCED_PARAMETER(ratPrioritiesLength);
    /*
    CELLULAR_RAT_GSM
    CELLULAR_RAT_CATM1
    CELLULAR_RAT_NBIOT
    */

    return CELLULAR_UNSUPPORTED;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetRatPriority( CellularHandle_t cellularHandle,
                                         CellularRat_t * pRatPriorities,
                                         uint8_t ratPrioritiesLength,
                                         uint8_t * pReceiveRatPrioritesLength )
{
    UNREFERENCED_PARAMETER(cellularHandle);
    UNREFERENCED_PARAMETER(pRatPriorities);
    UNREFERENCED_PARAMETER(ratPrioritiesLength);
    UNREFERENCED_PARAMETER(pReceiveRatPrioritesLength);

    return CELLULAR_UNSUPPORTED;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SetDns( CellularHandle_t cellularHandle,
                                 uint8_t contextId,
                                 const char * pDnsServerAddress )
{
    UNREFERENCED_PARAMETER(cellularHandle);
    UNREFERENCED_PARAMETER(contextId);
    UNREFERENCED_PARAMETER(pDnsServerAddress);

    return CELLULAR_UNSUPPORTED;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetPsmSettings( CellularHandle_t cellularHandle,
                                         CellularPsmSettings_t * pPsmSettings )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t atReqGetPsm =
    {
        "AT+CPSMS?",
        CELLULAR_AT_WITH_PREFIX,
        "+CPSMS",
        _Cellular_RecvFuncGetPsmSettings,
        pPsmSettings,
        sizeof( CellularPsmSettings_t ),
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pPsmSettings == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* initialize the data. */
        ( void ) memset( pPsmSettings, 0, sizeof( CellularPsmSettings_t ) );
        pPsmSettings->mode = 0xFF;

        /* we should always query the PSMsettings from the network. */
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetPsm );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_GetPsmSettings: couldn't retrieve PSM settings" );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

static uint32_t appendBinaryPattern( char * cmdBuf,
                                     uint32_t cmdLen,
                                     uint32_t value,
                                     bool endOfString )
{
    uint32_t retLen = 0;

    if( cmdBuf != NULL )
    {
        if( value != 0U )
        {
            ( void ) snprintf( cmdBuf, cmdLen, "\"" PRINTF_BINARY_PATTERN_INT8 "\"%c",
                               PRINTF_BYTE_TO_BINARY_INT8( value ), endOfString ? '\0' : ',' );
        }
        else
        {
            ( void ) snprintf( cmdBuf, cmdLen, "%c", endOfString ? '\0' : ',' );
        }

        retLen = strlen( cmdBuf );
    }

    return retLen;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t socketSendDataPrefix( void * pCallbackContext,
                                                 char * pLine,
                                                 uint32_t * pBytesRead )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;

    if( ( pLine == NULL ) || ( pBytesRead == NULL ) )
    {
        CellularLogError( "socketSendDataPrefix: pLine is invalid or pBytesRead is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( pCallbackContext != NULL )
    {
        CellularLogError( "socketSendDataPrefix: pCallbackContext is not NULL" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( *pBytesRead != 2U )
    {
        CellularLogDebug( "socketSendDataPrefix: pBytesRead %u %s is not 1", *pBytesRead, pLine );
    }
    else
    {
        /* After the data prefix, there should not be any data in stream.
         * Cellular commmon processes AT command in lines. Add a '\0' after '>'. */
        if( strcmp( pLine, "> " ) == 0 )
        {
            pLine[ 1 ] = '\n';
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SetPsmSettings( CellularHandle_t cellularHandle,
                                         const CellularPsmSettings_t * pPsmSettings )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { '\0' };
    uint32_t cmdBufLen = 0;
    CellularAtReq_t atReqSetPsm =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pPsmSettings == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_MAX_SIZE, "AT+CPSMS=%d,", pPsmSettings->mode );
        cmdBufLen = strlen( cmdBuf );
        cmdBufLen = cmdBufLen + appendBinaryPattern( &cmdBuf[ cmdBufLen ], ( CELLULAR_AT_CMD_MAX_SIZE - cmdBufLen ),
                                                     pPsmSettings->periodicRauValue, false );
        cmdBufLen = cmdBufLen + appendBinaryPattern( &cmdBuf[ cmdBufLen ], ( CELLULAR_AT_CMD_MAX_SIZE - cmdBufLen ),
                                                     pPsmSettings->gprsReadyTimer, false );
        cmdBufLen = cmdBufLen + appendBinaryPattern( &cmdBuf[ cmdBufLen ], ( CELLULAR_AT_CMD_MAX_SIZE - cmdBufLen ),
                                                     pPsmSettings->periodicTauValue, false );
        cmdBufLen = cmdBufLen + appendBinaryPattern( &cmdBuf[ cmdBufLen ], ( CELLULAR_AT_CMD_MAX_SIZE - cmdBufLen ),
                                                     pPsmSettings->activeTimeValue, true );

        CellularLogDebug( "PSM setting: %s ", cmdBuf );

        if( cmdBufLen < CELLULAR_AT_CMD_MAX_SIZE )
        {
            /* we should always query the PSMsettings from the network. */
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSetPsm );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                CellularLogError( "Cellular_SetPsmSettings: couldn't set PSM settings" );
                cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
            }
        }
        else
        {
            cellularStatus = CELLULAR_NO_MEMORY;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_DeactivatePdn( CellularHandle_t cellularHandle,
                                        uint8_t contextId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_BAD_PARAMETER;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqDeactPdn =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
//      "+APP PDP",
        NULL,
        NULL,
        NULL,
        0,
    };

    if(IsValidPDP(contextId))
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "AT+CNACT=%d,0", contextId );
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqDeactPdn, PDN_DEACTIVATION_PACKET_REQ_TIMEOUT_MS );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Can't deactivate PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_ActivatePdn( CellularHandle_t cellularHandle,
                                      uint8_t contextId )
{
    CellularContext_t * pContext        = ( CellularContext_t * ) cellularHandle;
    CellularError_t     cellularStatus  = CELLULAR_BAD_PARAMETER;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;

    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE * 2 ] = { '\0' }; /* APN & auth info is too long ...*/

    CellularAtReq_t atReqActPdn =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    if(!IsValidPDP(contextId))
    {
        CellularLogError("ActivatePdn: contexId err: %d", contextId);
        goto err;
    }

    /* Make sure the library is open. */
    cellularStatus = _Cellular_CheckLibraryStatus(pContext);
    if(cellularStatus != CELLULAR_SUCCESS)
        goto err;

    cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
    if(pSimContex ==NULL)
        goto err;

    if (pSimContex->pdnCfg.password && strlen(pSimContex->pdnCfg.password) > 0
        && pSimContex->pdnCfg.username && strlen(pSimContex->pdnCfg.username) > 0
        && pSimContex->pdnCfg.pdnAuthType > 0)
        (void)snprintf(cmdBuf, sizeof(cmdBuf), "AT+CNCFG=%d,%d,\"%s\",\"%s\",\"%s\",%d",
            contextId,
            0,      /* 0=Dual Stack, 1=IPV4, 2=IPV6*/
            pSimContex->pdnCfg.apnName, pSimContex->pdnCfg.username, pSimContex->pdnCfg.password, pSimContex->pdnCfg.pdnAuthType);
    else
        (void)snprintf(cmdBuf, sizeof(cmdBuf), "AT+CNCFG=%d,%d,\"%s\"",
            contextId,
            0,      /* 0=Dual Stack, 1=IPV4, 2=IPV6*/
            pSimContex->pdnCfg.apnName);

    pktStatus = _Cellular_AtcmdRequestWithCallback(pContext, atReqActPdn);

    if (pktStatus != CELLULAR_PKT_STATUS_OK)
    {
        CellularLogError("can't set PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus);
        cellularStatus = _Cellular_TranslatePktStatus(pktStatus);
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        xEventGroupClearBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_ACT);
        (void)snprintf( cmdBuf, sizeof(cmdBuf), "AT+CNACT=%d,1", contextId );

        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqActPdn, PDN_ACTIVATION_PACKET_REQ_TIMEOUT_MS );

        if (pktStatus == CELLULAR_PKT_STATUS_OK)
        {
            CellularLogDebug("Cellular module wait Pdn-%d ...", contextId);

            xEventGroupWaitBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_ACT, true, false,
                        pdMS_TO_TICKS(PDN_ACTIVATION_PACKET_REQ_TIMEOUT_MS));

            CellularLogInfo( "Cellular module wait Pdn-%d ready", contextId);
        }
        else
        {
            CellularLogError( "Can't activate PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

err:
    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SetPdnConfig( CellularHandle_t cellularHandle,
                                       uint8_t contextId,
                                       const CellularPdnConfig_t * pPdnConfig )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t     cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;

    char    cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE*2 ] = { '\0' };    /* APN and auth info is too long */

    CellularAtReq_t atReqSetPdn =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    if( pPdnConfig == NULL )
    {
        CellularLogError( "Cellular_ATCommandRaw: Input parameter is NULL" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        cellularStatus = IsValidPDP( contextId ) ? CELLULAR_SUCCESS : CELLULAR_BAD_PARAMETER; /* 0-3 */
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        const char  *pPdnType = "IP";    /*default: CELLULAR_PDN_CONTEXT_IPV4  */
        
        if( pPdnConfig->pdnContextType == CELLULAR_PDN_CONTEXT_IPV6 )
            pPdnType = "IPV6";
        else if( pPdnConfig->pdnContextType == CELLULAR_PDN_CONTEXT_IPV4V6 )
            pPdnType = "IPV4V6";

        ( void ) snprintf( cmdBuf, sizeof(cmdBuf), "AT+CGDCONT=%d,\"%s\",\"%s\"",
                            CELLULAR_PDN_CONTEXT_ID,
                            pPdnType,
                            pPdnConfig->apnName );
                           
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSetPdn );
        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_SetPdnConfig: can't set PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }


        if( pPdnConfig->pdnAuthType == 0 )
            ( void ) snprintf( cmdBuf, sizeof(cmdBuf), "AT+CGAUTH=%d,0", CELLULAR_PDN_CONTEXT_ID);
        else
            ( void ) snprintf( cmdBuf, sizeof(cmdBuf), "AT+CGAUTH=%d,%d,\"%s\",\"%s\"",
                                CELLULAR_PDN_CONTEXT_ID,
                               pPdnConfig->pdnAuthType,
                               pPdnConfig->password,
                               pPdnConfig->username );
     
        cellularModuleContext_t* pSimContex = (cellularModuleContext_t *)pContext->pModueContext;
        memcpy( &pSimContex->pdnCfg, pPdnConfig, sizeof(CellularPdnConfig_t) );

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSetPdn );
        
        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_SetPdnConfig: can't set PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetSignalInfo( CellularHandle_t cellularHandle,
                                        CellularSignalInfo_t * pSignalInfo )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularRat_t rat = CELLULAR_RAT_INVALID;
    CellularAtReq_t atReqQuerySignalInfo =
    {
        "AT+CPSI?",
        CELLULAR_AT_WITH_PREFIX,
        "+CPSI:",
        _Cellular_RecvFuncGetSignalInfo,
        pSignalInfo,
        sizeof( CellularSignalInfo_t ),
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pSignalInfo == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        cellularStatus = _Cellular_GetCurrentRat( pContext, &rat );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqQuerySignalInfo );

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            /* If the convert failed, the API will return CELLULAR_INVALID_SIGNAL_BAR_VALUE in bars field. */
            ( void ) _Cellular_ComputeSignalBars( rat, pSignalInfo );
        }

        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SocketRecv( CellularHandle_t cellularHandle,
                                     CellularSocketHandle_t socketHandle,
                                     uint8_t * pBuffer,
                                     uint32_t bufferLength,
                                     uint32_t * pReceivedDataLength )
{
    CellularContext_t *     pContext        = ( CellularContext_t * ) cellularHandle;
    CellularError_t         cellularStatus  = CELLULAR_SUCCESS;
    CellularPktStatus_t     pktStatus       = CELLULAR_PKT_STATUS_OK;

    char        cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    uint32_t    recvTimeout = DATA_READ_TIMEOUT_MS;
    uint32_t    recvLen      = bufferLength;
    _socketDataRecv_t dataRecv =
    {
        pReceivedDataLength,
        pBuffer,
        NULL
    };
    CellularAtReq_t atReqSocketRecv =
    {
        cmdBuf,
        CELLULAR_AT_MULTI_DATA_WO_PREFIX,
        "+CARECV",
        _Cellular_RecvFuncData,
        (void *)&dataRecv,
        bufferLength,
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pBuffer == NULL ) || ( pReceivedDataLength == NULL ) || ( bufferLength == 0U ) )
    {
        CellularLogError( "_Cellular_RecvData: Bad input Param" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Update recvLen to maximum module length. */
        if( CELLULAR_MAX_RECV_DATA_LEN <= bufferLength )
        {
            recvLen = ( uint32_t ) CELLULAR_MAX_RECV_DATA_LEN;
        }

        /* Update receive timeout to default timeout if not set with setsocketopt. */
        if( socketHandle->recvTimeoutMs != 0U )
        {
            recvTimeout = socketHandle->recvTimeoutMs;
        }

        CellularLogDebug("Cellular module wait +CDATAIND: %d ...", socketHandle->socketId);

        cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
        xEventGroupWaitBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_IND, false, false,
            pdMS_TO_TICKS(PDN_ACTIVATION_PACKET_REQ_TIMEOUT_MS));

        /* Form the AT command. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE,
                           "AT+CARECV=%ld,%ld", socketHandle->socketId, recvLen );
        pktStatus = _Cellular_TimeoutAtcmdDataRecvRequestWithCallback( pContext,
                                          atReqSocketRecv, recvTimeout, socketRecvDataPrefix, pContext);

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            /* Reset data handling parameters. */
            CellularLogError( "_Cellular_RecvData: Data Receive fail, pktStatus: %d", pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SocketSend( CellularHandle_t cellularHandle,
                                     CellularSocketHandle_t socketHandle,
                                     const uint8_t * pData,
                                     uint32_t dataLength,
                                     uint32_t * pSentDataLength )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    uint32_t sendTimeout = DATA_SEND_TIMEOUT_MS;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqSocketSend =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };
    CellularAtDataReq_t atDataReqSocketSend =
    {
        pData,
        dataLength,
        pSentDataLength,
        NULL,
        0
    };

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pData == NULL ) || ( pSentDataLength == NULL ) || ( dataLength == 0U ) )
    {
        CellularLogError( "Cellular_SocketSend: Invalid parameter" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Send data length check. */
        if( dataLength > ( uint32_t ) CELLULAR_MAX_SEND_DATA_LEN )
        {
            atDataReqSocketSend.dataLen = ( uint32_t ) CELLULAR_MAX_SEND_DATA_LEN;
        }

        /* Check send timeout. If not set by setsockopt, use default value. */
        if( socketHandle->sendTimeoutMs != 0U )
        {
            sendTimeout = socketHandle->sendTimeoutMs;
        }

        /* Form the AT command. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "AT+CASEND=%ld,%ld",
                           socketHandle->socketId, atDataReqSocketSend.dataLen );

        pktStatus = _Cellular_AtcmdDataSend( pContext, atReqSocketSend, atDataReqSocketSend,
                                             socketSendDataPrefix, NULL,
                                             PACKET_REQ_TIMEOUT_MS, sendTimeout, 0U );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_SocketSend: Data send fail, PktRet: %d", pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SocketClose( CellularHandle_t cellularHandle,
                                      CellularSocketHandle_t socketHandle )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqSockClose =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    /* Make sure the library is open. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        if( ( socketHandle->socketState == SOCKETSTATE_CONNECTING ) ||
            ( socketHandle->socketState == SOCKETSTATE_CONNECTED ) ||
            ( socketHandle->socketState == SOCKETSTATE_DISCONNECTED ) )
        {
            /* Form the AT command. */

            /* The return value of snprintf is not used.
             * The max length of the string is fixed and checked offline. */
            ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "AT+CACLOSE=%d", socketHandle->socketId );
            pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqSockClose,
                                                                   SOCKET_DISCONNECT_PACKET_REQ_TIMEOUT_MS );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                CellularLogError( "Cellular_SocketClose: Socket close failed, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            }
        }

        /* Ignore the result from the info, and force to remove the socket. */
        cellularStatus = _Cellular_RemoveSocketData( pContext, socketHandle );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_SocketConnect( CellularHandle_t cellularHandle,
                                        CellularSocketHandle_t socketHandle,
                                        CellularSocketAccessMode_t dataAccessMode,
                                        const CellularSocketAddress_t * pRemoteSocketAddress )
{
    CellularContext_t * pContext        = ( CellularContext_t * ) cellularHandle;
    CellularError_t     cellularStatus  = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;

    char            cmdBuf[CELLULAR_AT_CMD_MAX_SIZE];
    CellularAtReq_t atReqSocketConnect =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    /* Make sure the library is open. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
    }
    else if( pRemoteSocketAddress == NULL )
    {
        CellularLogError( "Cellular_SocketConnect: Invalid socket address" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else
    {
        cellularStatus = storeAccessModeAndAddress( pContext, socketHandle, dataAccessMode, pRemoteSocketAddress );
    }

#if 0
    if (cellularStatus == CELLULAR_SUCCESS)
    {
        snprintf( cmdBuf, sizeof(cmdBuf), "AT+CACID=%d", socketHandle->socketId );
        cellularStatus = _Cellular_TimeoutAtcmdRequestWithCallback(pContext, atReqSocketConnect, PACKET_REQ_TIMEOUT_MS);
    }
#endif


    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Builds the Socket connect command. */
        cellularStatus = buildSocketConnect( socketHandle, cmdBuf );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        cellularModuleContext_t* pSimContex = (cellularModuleContext_t*)pContext->pModueContext;
        xEventGroupClearBits(pSimContex->pdnEvent, CNACT_EVENT_BIT_IND);
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqSocketConnect,
                                                               SOCKET_CONNECT_PACKET_REQ_TIMEOUT_MS );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_SocketConnect: Socket connect failed, cmd:%s, PktRet: %d",
                cmdBuf, pktStatus );

            /* recovery a already opened socket.    */
            snprintf(cmdBuf, sizeof(cmdBuf), "AT+CACLOSE=%d", socketHandle->socketId);
            _Cellular_AtcmdRequestWithCallback(pContext, atReqSocketConnect);

            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
        else
        {
            socketHandle->socketState = SOCKETSTATE_CONNECTING;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetPdnStatus( CellularHandle_t cellularHandle,
                                       CellularPdnStatus_t * pPdnStatusBuffers,
                                       uint8_t numStatusBuffers,
                                       uint8_t * pNumStatus )
{
    CellularContext_t *         pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t             cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t         pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t             atReqGetPdnStatus =
    {
        "AT+CNACT?",
        CELLULAR_AT_MULTI_WITH_PREFIX,
        "+CNACT",
        _Cellular_RecvFuncGetPdnStatus,
        pPdnStatusBuffers,
        numStatusBuffers,
    };

    if( (pPdnStatusBuffers == NULL ) || ( pNumStatus == NULL ) || ( numStatusBuffers < 1u ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
        CellularLogWarn( "_Cellular_GetPdnStatus: Bad input Parameter " );
    }

    memset(pPdnStatusBuffers, 0, sizeof(CellularPdnStatus_t) * numStatusBuffers);

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetPdnStatus );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        const CellularPdnStatus_t*  pSt;
        uint8_t                     nSt, i;

        /* Populate the Valid number of statuses. */
        *pNumStatus = 0;

        for (nSt = 0; nSt == 0; )
        {
            pSt = pPdnStatusBuffers;
            for (i = 0; i < numStatusBuffers; i++)
            {
                if (pSt->state == 1)   /* Active PDP    */
                    nSt++;
            }
            if (!nSt)
                Sleep(pdMS_TO_TICKS(300));
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetSimCardStatus( CellularHandle_t cellularHandle, CellularSimCardStatus_t* pSimCardStatus )
{
    CellularContext_t * pContext        = ( CellularContext_t * ) cellularHandle;
    CellularError_t     cellularStatus  = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus       = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t     atReqGetSimCardStatus =
    {
        "AT+CPIN?",
        CELLULAR_AT_WITH_PREFIX,
        "+CPIN",
        _Cellular_RecvFuncGetSimCardStatus,
        pSimCardStatus,
        sizeof( CellularSimCardState_t ),
    };

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pSimCardStatus == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        /* Initialize the sim state and the sim lock state. */
        pSimCardStatus->simCardState = CELLULAR_SIM_CARD_UNKNOWN;
        pSimCardStatus->simCardLockState = CELLULAR_SIM_CARD_LOCK_UNKNOWN;

        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetSimCardStatus );

        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        CellularLogDebug( "_Cellular_GetSimStatus, Sim Insert State[%d], Lock State[%d]",
                     pSimCardStatus->simCardState, pSimCardStatus->simCardLockState );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetSimCardInfo( CellularHandle_t cellularHandle,
                                         CellularSimCardInfo_t * pSimCardInfo )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;

    CellularAtReq_t atReqGetIccid =
    {
        "AT+CCID",
        CELLULAR_AT_WO_PREFIX,
        NULL,
        _Cellular_RecvFuncGetIccid,
        pSimCardInfo->iccid,
        CELLULAR_ICCID_MAX_SIZE + 1U,
    };
    CellularAtReq_t atReqGetImsi =
    {
        "AT+CIMI",
        CELLULAR_AT_WO_PREFIX,
        NULL,
        _Cellular_RecvFuncGetImsi,
        pSimCardInfo->imsi,
        CELLULAR_IMSI_MAX_SIZE + 1U,
    };
    CellularAtReq_t atReqGetHplmn =
    {
        "AT+CRSM=176,28514,0,0,0",
        CELLULAR_AT_WITH_PREFIX,
        "+CRSM",
        _Cellular_RecvFuncGetHplmn,
        &pSimCardInfo->plmn,
        sizeof( CellularPlmnInfo_t ),
    };

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pSimCardInfo == NULL )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        ( void ) memset( pSimCardInfo, 0, sizeof( CellularSimCardInfo_t ) );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetImsi );

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetHplmn );
        }

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetIccid );
        }

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
        else
        {
            CellularLogDebug( "SimInfo updated: IMSI:%s, Hplmn:%s%s, ICCID:%s",
                         pSimCardInfo->imsi, pSimCardInfo->plmn.mcc, pSimCardInfo->plmn.mnc,
                         pSimCardInfo->iccid );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_RegisterUrcSignalStrengthChangedCallback( CellularHandle_t cellularHandle,
                                                                   CellularUrcSignalStrengthChangedCallback_t signalStrengthChangedCallback,
                                                                   void * pCallbackContext )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;

    /* pContext is checked in the common library. */
    cellularStatus = Cellular_CommonRegisterUrcSignalStrengthChangedCallback(
        cellularHandle, signalStrengthChangedCallback, pCallbackContext );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        if( signalStrengthChangedCallback != NULL )
        {
            cellularStatus = controlSignalStrengthIndication( pContext, true );
        }
        else
        {
            cellularStatus = controlSignalStrengthIndication( pContext, false );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
CellularError_t Cellular_GetHostByName( CellularHandle_t cellularHandle,
                                        uint8_t contextId,
                                        const char * pcHostName,
                                        char * pResolvedAddress )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_QUERY_DNS_MAX_SIZE ];
    cellularDnsQueryResult_t dnsQueryResult = CELLULAR_DNS_QUERY_UNKNOWN;
    cellularModuleContext_t * pModuleContext = NULL;
    CellularAtReq_t atReqQueryDns =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        CellularLogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( ( pcHostName == NULL ) || ( pResolvedAddress == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        cellularStatus = IsValidPDP(contextId) ? CELLULAR_SUCCESS : CELLULAR_BAD_PARAMETER;
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        cellularStatus = _Cellular_GetModuleContext( pContext, ( void ** ) &pModuleContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        PlatformMutex_Lock( &pModuleContext->dnsQueryMutex );
        pModuleContext->dnsResultNumber = 0;
        pModuleContext->dnsIndex = 0;
        ( void ) xQueueReset( pModuleContext->pktDnsQueue );
        cellularStatus = registerDnsEventCallback( pModuleContext, _dnsResultCallback, pResolvedAddress );
    }

    /* Send the AT command and wait the URC result. */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_QUERY_DNS_MAX_SIZE,
                           "AT+CDNSGIP=\"%s\",0,10000", pcHostName );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqQueryDns );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            CellularLogError( "Cellular_GetHostByName: couldn't resolve host name" );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
            PlatformMutex_Unlock( &pModuleContext->dnsQueryMutex );
        }
    }

    /* URC handler calls the callback to unblock this function. */
    if( cellularStatus == CELLULAR_SUCCESS )
    {
        if( xQueueReceive( pModuleContext->pktDnsQueue, &dnsQueryResult,
                           pdMS_TO_TICKS( DNS_QUERY_TIMEOUT_MS ) ) == pdTRUE )
        {
            if( dnsQueryResult != CELLULAR_DNS_QUERY_SUCCESS )
            {
                cellularStatus = CELLULAR_UNKNOWN;
            }
        }
        else
        {
            ( void ) registerDnsEventCallback( pModuleContext, NULL, NULL );
            cellularStatus = CELLULAR_TIMEOUT;
        }

        PlatformMutex_Unlock( &pModuleContext->dnsQueryMutex );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/
