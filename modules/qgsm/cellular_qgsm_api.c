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

#include "cellular_types.h"
#include "cellular_api.h"
#include "cellular_common_api.h"
#include "cellular_common.h"
#include "cellular_at_core.h"
#include "cellular_qgsm.h"

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

#define CELLULAR_PDN_STATUS_POS_CONTEXT_ID       ( 0U )
#define CELLULAR_PDN_STATUS_POS_CONTEXT_STATE    ( 1U )
#define CELLULAR_PDN_STATUS_POS_CONTEXT_TYPE     ( 2U )
#define CELLULAR_PDN_STATUS_POS_IP_ADDRESS       ( 3U )

#define INVALID_PDN_INDEX                        ( 0xFFU )

#define DATA_PREFIX_STRING                       "+QIRD:"
#define DATA_PREFIX_STRING_LENGTH                ( 6U )

#define MAX_QIRD_STRING_PREFIX_STRING            ( 40U )  /* The max data prefix string is "+QIRD: <255.255.255.255>:<65535>,<TCP or UDP>,<1500>\r\n"  40 */

#define DEFAULT_FOREGROUND_CONTEXT               ( 0U )   /* The Default value for foreground context (the range is 0-1) */

#define USE_OPERATION_MODE                       ( 1U )   /* Select the operation mode of the modem: 1 for client; 2 for server*/
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
static CellularPktStatus_t _Cellular_RecvFuncGetSimLockStatus( CellularContext_t * pContext,
                                                               const CellularATCommandResponse_t * pAtResp,
                                                               void * pData,
                                                               uint16_t dataLen );
static CellularATError_t parsePdnStatusContextId( char * pToken,
                                                  CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t parsePdnStatusContextState( char * pToken,
                                                     CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t parsePdnStatusContextType( char * pToken,
                                                    CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t getPdnStatusParseToken( char * pToken,
                                                 uint8_t tokenIndex,
                                                 CellularPdnStatus_t * pPdnStatusBuffers );
static CellularATError_t getPdnStatusParseLine( char * pRespLine,
                                                CellularPdnStatus_t * pPdnStatusBuffers );
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

static CellularPktStatus_t _Cellular_GetSocketInfo( CellularContext_t * pContext,
                                                    const CellularATCommandResponse_t * pAtResp,
                                                    void * pData,
                                                    uint16_t dataLen );

/*-----------------------------------------------------------*/

static bool _parseSignalQuality( char * pQcsqPayload,
                                 CellularSignalInfo_t * pSignalInfo )
{
    char * pToken = NULL, * pTmpQcsqPayload = pQcsqPayload;
    int32_t tempValue = 0;
    bool parseStatus = true;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    if( ( pSignalInfo == NULL ) || ( pQcsqPayload == NULL ) )
    {
        LogError( "_parseSignalQuality: Invalid Input Parameters" );
        parseStatus = false;
    }

    if( ( parseStatus == true ) && ( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) == CELLULAR_AT_SUCCESS ) )
    {
        if( ( strcmp( pToken, "GSM" ) != 0 ) &&
            ( strcmp( pToken, "CAT-M1" ) != 0 ) &&
            ( strcmp( pToken, "CAT-NB1" ) != 0 ) )
        {
            parseStatus = false;
        }
    }
    else
    {
        LogDebug( "_parseSignalQuality: No Valid RAT in QCSQ Response" );
        parseStatus = false;
    }

    if( ( parseStatus == true ) && ( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) == CELLULAR_AT_SUCCESS ) )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            pSignalInfo->rssi = ( int16_t ) tempValue;
        }
        else
        {
            LogError( "_parseSignalQuality: Error in processing RSSI. Token %s", pToken );
            parseStatus = false;
        }
    }
    else
    {
        parseStatus = false;
    }

    if( ( parseStatus == true ) && ( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) == CELLULAR_AT_SUCCESS ) )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            pSignalInfo->rsrp = ( int16_t ) tempValue;
        }
        else
        {
            LogError( "_parseSignalQuality: Error in processing RSRP. Token %s", pToken );
            parseStatus = false;
        }
    }
    else
    {
        parseStatus = false;
    }

    if( ( parseStatus == true ) && ( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) == CELLULAR_AT_SUCCESS ) )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* SINR is reported as an integer value ranging from 0 to 250 representing 1/5 of a dB.
             * Value 0 correspond to -20 dBm and 250 corresponds to +30 dBm. */
            pSignalInfo->sinr = ( int16_t ) ( SIGNAL_QUALITY_SINR_MIN_VALUE + ( ( tempValue ) / ( SIGNAL_QUALITY_SINR_DIVISIBILITY_FACTOR ) ) );
        }
        else
        {
            LogError( "_parseSignalQuality: Error in processing SINR. pToken %s", pToken );
            parseStatus = false;
        }
    }
    else
    {
        parseStatus = false;
    }

    if( ( parseStatus == true ) && ( Cellular_ATGetNextTok( &pTmpQcsqPayload, &pToken ) == CELLULAR_AT_SUCCESS ) )
    {
        atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            pSignalInfo->rsrq = ( int16_t ) tempValue;
        }
        else
        {
            LogError( "_parseSignalQuality: Error in processing RSRQ. Token %s", pToken );
            parseStatus = false;
        }
    }
    else
    {
        parseStatus = false;
    }

    return parseStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */
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
        LogError( "GetSignalInfo: Input Line passed is NULL" );
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
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    uint8_t enable_value = 0;
    CellularAtReq_t atReqControlSignalStrengthIndication =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    if( enable == true )
    {
        enable_value = 1;
    }
    else
    {
        enable_value = 0;
    }

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "AT+QINDCFG=\"csq\",%u", enable_value );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqControlSignalStrengthIndication );
        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */
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
        LogError( "getIccid: Response in invalid " );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pRespLine = pAtResp->pItm->pLine;
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pRespLine );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            /* Removing QCCID Prefix in AT Response. */
            atCoreStatus = Cellular_ATRemovePrefix( &pRespLine );
        }

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
/* coverity[misra_c_2012_rule_8_13_violation] */
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
        LogError( "getImsi: Response in invalid" );
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
        LogError( "Input Parameter NULL" );
        memoryStatus = false;
    }

    if( memoryStatus )
    {
        /* checking the value sw2 in AT command response for memory problem during CRSM read.
         * Refer 3GPP Spec TS 51.011 Section 9.4. */
        if( strcmp( pToken, "64" ) == 0 )
        {
            LogError( "_checkCrsmMemoryStatus: Error in Processing HPLMN: CRSM Memory Error" );
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
        LogError( "Input Parameter NULL" );
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
            LogError( "_checkCrsmReadStatus: Error in Processing HPLMN: CRSM Read Error" );
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
        LogError( "Input Parameter NULL" );
        parseStatus = false;
    }

    if( parseStatus == true )
    {
        /* Checking if the very first HPLMN entry in AT command Response is valid*/
        if( ( strlen( pToken ) < ( CRSM_HPLMN_RAT_LENGTH ) ) || ( strncmp( pToken, "FFFFFF", 6 ) == 0 ) )
        {
            LogError( "_parseHplmn: Error in Processing HPLMN: Invalid Token %s", pToken );
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
/* coverity[misra_c_2012_rule_8_13_violation] */
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
        LogError( "GetHplmn: Response is invalid " );
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
/* coverity[misra_c_2012_rule_8_13_violation] */
static CellularPktStatus_t _Cellular_RecvFuncGetSimCardStatus( CellularContext_t * pContext,
                                                               const CellularATCommandResponse_t * pAtResp,
                                                               void * pData,
                                                               uint16_t dataLen )
{
    char * pInputLine = NULL;
    const char * pTokenPtr = NULL;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularSimCardState_t * pSimCardState = ( CellularSimCardState_t * ) pData;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        LogError( "GetSimStatus: response is invalid" );
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
                pktStatus = _Cellular_ParseSimstat( pInputLine, pSimCardState );
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
            LogError( "Unknown SIM Lock State %s", pToken );
        }
    }

    return tempState;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */
static CellularPktStatus_t _Cellular_RecvFuncGetSimLockStatus( CellularContext_t * pContext,
                                                               const CellularATCommandResponse_t * pAtResp,
                                                               void * pData,
                                                               uint16_t dataLen )
{
    char * pToken = NULL, * pInputStr = NULL;
    CellularSimCardLockState_t * pSimLockState = NULL;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    if( pContext == NULL )
    {
        pktStatus = CELLULAR_PKT_STATUS_INVALID_HANDLE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) ||
             ( pData == NULL ) || ( dataLen != sizeof( CellularSimCardLockState_t ) ) )
    {
        LogError( " Get SIM lock State: Response data is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputStr = pAtResp->pItm->pLine;
        pSimLockState = ( CellularSimCardLockState_t * ) pData;

        if( strlen( pInputStr ) == 0U )
        {
            LogError( "Get SIM lock State: Input data is invalid" );
            pktStatus = CELLULAR_PKT_STATUS_FAILURE;
        }
    }

    if( pktStatus == CELLULAR_PKT_STATUS_OK )
    {
        atCoreStatus = Cellular_ATRemoveAllWhiteSpaces( pInputStr );

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATRemovePrefix( &pInputStr );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            atCoreStatus = Cellular_ATGetNextTok( &pInputStr, &pToken );
        }

        if( atCoreStatus == CELLULAR_AT_SUCCESS )
        {
            LogDebug( "SIM Lock State: %s", pToken );
            *pSimLockState = _getSimLockState( pToken );
        }

        if( atCoreStatus != CELLULAR_AT_SUCCESS )
        {
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parsePdnStatusContextId( char * pToken,
                                                  CellularPdnStatus_t * pPdnStatusBuffers )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( tempValue >= ( int32_t ) CELLULAR_PDN_CONTEXT_ID_MIN ) &&
            ( tempValue <= ( int32_t ) CELLULAR_PDN_CONTEXT_ID_MAX ) )
        {
            pPdnStatusBuffers->contextId = ( uint8_t ) tempValue;
        }
        else
        {
            LogError( "Error in Processing Context Id. Token %s", pToken );
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
        if( ( tempValue >= 0 ) &&
            ( tempValue <= ( int32_t ) UINT8_MAX ) )
        {
            pPdnStatusBuffers->state = ( uint8_t ) tempValue;
        }
        else
        {
            LogError( "Error in processing PDN Status Buffer state. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t parsePdnStatusContextType( char * pToken,
                                                    CellularPdnStatus_t * pPdnStatusBuffers )
{
    int32_t tempValue = 0;
    CellularATError_t atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( ( tempValue >= 0 ) && ( tempValue < ( int32_t ) CELLULAR_PDN_CONTEXT_TYPE_MAX ) )
        {
            /* Variable "tempValue" is ensured that it is valid and within
             * a valid range. Hence, assigning the value of the variable to
             * pdnContextType with a enum cast. */
            /* coverity[misra_c_2012_rule_10_5_violation] */
            pPdnStatusBuffers->pdnContextType = ( CellularPdnContextType_t ) tempValue;
        }
        else
        {
            LogError( "Error in processing PDN State Buffer Status. Token %s", pToken );
            atCoreStatus = CELLULAR_AT_ERROR;
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getPdnStatusParseToken( char * pToken,
                                                 uint8_t tokenIndex,
                                                 CellularPdnStatus_t * pPdnStatusBuffers )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    switch( tokenIndex )
    {
        case ( CELLULAR_PDN_STATUS_POS_CONTEXT_ID ):
            LogDebug( "Context Id: %s", pToken );
            atCoreStatus = parsePdnStatusContextId( pToken, pPdnStatusBuffers );
            break;

        case ( CELLULAR_PDN_STATUS_POS_CONTEXT_STATE ):
            LogDebug( "Context State: %s", pToken );
            atCoreStatus = parsePdnStatusContextState( pToken, pPdnStatusBuffers );
            break;

        case ( CELLULAR_PDN_STATUS_POS_CONTEXT_TYPE ):
            LogDebug( "Context Type: %s", pToken );
            atCoreStatus = parsePdnStatusContextType( pToken, pPdnStatusBuffers );
            break;

        case ( CELLULAR_PDN_STATUS_POS_IP_ADDRESS ):
            LogDebug( "IP address: %s", pToken );
            ( void ) memcpy( ( void * ) pPdnStatusBuffers->ipAddress.ipAddress,
                             ( void * ) pToken, CELLULAR_IP_ADDRESS_MAX_SIZE + 1U );

            if( pPdnStatusBuffers->pdnContextType == CELLULAR_PDN_CONTEXT_IPV4 )
            {
                pPdnStatusBuffers->ipAddress.ipAddressType = CELLULAR_IP_ADDRESS_V4;
            }
            else if( pPdnStatusBuffers->pdnContextType == CELLULAR_PDN_CONTEXT_IPV6 )
            {
                pPdnStatusBuffers->ipAddress.ipAddressType = CELLULAR_IP_ADDRESS_V6;
            }
            else
            {
                LogError( "Unknown pdnContextType %d", pPdnStatusBuffers->pdnContextType );
                atCoreStatus = CELLULAR_AT_ERROR;
            }

            break;

        default:
            LogError( "Unknown token in getPdnStatusParseToken %s %d",
                      pToken, tokenIndex );
            atCoreStatus = CELLULAR_AT_ERROR;
            break;
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getPdnStatusParseLine( char * pRespLine,
                                                CellularPdnStatus_t * pPdnStatusBuffers )
{
    char * pToken = NULL;
    char * pLocalRespLine = pRespLine;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    uint8_t tokenIndex = 0;

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
        tokenIndex = 0;

        while( ( pToken != NULL ) && ( atCoreStatus == CELLULAR_AT_SUCCESS ) )
        {
            atCoreStatus = getPdnStatusParseToken( pToken, tokenIndex, pPdnStatusBuffers );

            if( atCoreStatus != CELLULAR_AT_SUCCESS )
            {
                LogInfo( "getPdnStatusParseToken %s index %d failed", pToken, tokenIndex );
            }

            tokenIndex++;

            if( Cellular_ATGetNextTok( &pLocalRespLine, &pToken ) != CELLULAR_AT_SUCCESS )
            {
                break;
            }
        }
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */
static CellularPktStatus_t _Cellular_RecvFuncGetPdnStatus( CellularContext_t * pContext,
                                                           const CellularATCommandResponse_t * pAtResp,
                                                           void * pData,
                                                           uint16_t dataLen )
{
    char * pRespLine = NULL;
    CellularPdnStatus_t * pPdnStatusBuffers = ( CellularPdnStatus_t * ) pData;
    uint8_t numStatusBuffers = ( uint8_t ) dataLen;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    const CellularATCommandLine_t * pCommnadItem = NULL;

    if( pContext == NULL )
    {
        LogError( "GetPdnStatus: invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) )
    {
        LogError( "GetPdnStatus: Response is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pPdnStatusBuffers == NULL ) || ( numStatusBuffers < 1U ) )
    {
        LogError( "GetPdnStatus: PDN Status bad parameters" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        LogError( "GetPdnStatus: no activated PDN" );
        pPdnStatusBuffers[ 0 ].contextId = INVALID_PDN_INDEX;
        pktStatus = CELLULAR_PKT_STATUS_OK;
    }
    else
    {
        pRespLine = pAtResp->pItm->pLine;

        pCommnadItem = pAtResp->pItm;

        while( ( numStatusBuffers != 0U ) && ( pCommnadItem != NULL ) )
        {
            pRespLine = pCommnadItem->pLine;
            atCoreStatus = getPdnStatusParseLine( pRespLine, pPdnStatusBuffers );
            pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                LogError( "getPdnStatusParseLine parse %s failed", pRespLine );
                break;
            }

            pPdnStatusBuffers++;
            numStatusBuffers--;
            pCommnadItem = pCommnadItem->pNext;
        }
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularError_t buildSocketConnect( CellularSocketHandle_t socketHandle,
                                           char * pCmdBuf )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    char protocol[ 15 ];

    if( pCmdBuf == NULL )
    {
        LogError( "buildSocketConnect: Invalid command buffer" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        if( socketHandle->socketProtocol == CELLULAR_SOCKET_PROTOCOL_TCP )
        {
            ( void ) strcpy( protocol, "TCP" );
        }
        else
        {
            ( void ) strcpy( protocol, "UDP" );
        }

        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( pCmdBuf, CELLULAR_AT_CMD_MAX_SIZE,
                           "%s%ld,\"%s\",\"%s\",%d,%d,%d",
                           "AT+QIOPEN=",
                           socketHandle->socketId,
                           protocol,
                           socketHandle->remoteSocketAddress.ipAddress.ipAddress,
                           socketHandle->remoteSocketAddress.port,
                           socketHandle->localPort,
                           socketHandle->dataMode );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

static CellularATError_t getDataFromResp( const CellularATCommandResponse_t * pAtResp,
                                          const _socketDataRecv_t * pDataRecv,
                                          uint32_t outBufSize )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char * pInputLine = NULL;
    uint32_t dataLenToCopy = 0;
    char * location = NULL;

    /* Check if the received data size is greater than the output buffer size. */
    if( *pDataRecv->pDataLen > outBufSize )
    {
        LogError( "Data is turncated, received data length %d, out buffer size %d",
                  *pDataRecv->pDataLen, outBufSize );
        dataLenToCopy = outBufSize;
        *pDataRecv->pDataLen = outBufSize;
    }
    else
    {
        dataLenToCopy = *pDataRecv->pDataLen;
    }

    /* Data is stored in the next intermediate response. */
    if( ( pAtResp->pItm->pNext != NULL ) && ( dataLenToCopy != 0 ) )
    {
        pInputLine = pAtResp->pItm->pNext->pLine;

        location = strstr( pInputLine, "QIRD" );

        while( location != NULL )
        {
            if( pAtResp->pItm->pNext != NULL )
            {
                pInputLine = pAtResp->pItm->pNext->pNext->pLine;
                location = strstr( pInputLine, "QIRD" );
            }
            else
            {
                /* */
            }
        }

        if( ( pInputLine != NULL ) && ( dataLenToCopy > 0U ) )
        {
            /* Copy the data to the out buffer. */
            ( void ) memcpy( ( void * ) pDataRecv->pData, ( const void * ) pInputLine, dataLenToCopy );
        }
        else
        {
            LogError( "Receive Data: Data pointer NULL" );
            atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
        }
    }
    else if( *pDataRecv->pDataLen == 0U )
    {
        /* Receive command success but no data. */
        LogDebug( "Receive Data: no data" );
    }
    else
    {
        LogError( "Receive Data: Intermediate response empty" );
        atCoreStatus = CELLULAR_AT_BAD_PARAMETER;
    }

    return atCoreStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */
static CellularPktStatus_t _Cellular_RecvFuncData( CellularContext_t * pContext,
                                                   const CellularATCommandResponse_t * pAtResp,
                                                   void * pData,
                                                   uint16_t dataLen )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char * pInputLine = NULL, * pToken = NULL;
    const _socketDataRecv_t * pDataRecv = ( _socketDataRecv_t * ) pData;
    int32_t tempValue = 0;
    char * location = NULL;

    if( pContext == NULL )
    {
        LogError( "Receive Data: invalid context" );
        pktStatus = CELLULAR_PKT_STATUS_FAILURE;
    }
    else if( ( pAtResp == NULL ) || ( pAtResp->pItm == NULL ) || ( pAtResp->pItm->pLine == NULL ) )
    {
        if( pAtResp->status == true )
        {
            *pDataRecv->pDataLen = 0;
        }
        else
        {
            LogError( "Receive Data: response is invalid" );
            pktStatus = CELLULAR_PKT_STATUS_FAILURE;
        }
    }
    else if( ( pDataRecv == NULL ) || ( pDataRecv->pData == NULL ) || ( pDataRecv->pDataLen == NULL ) )
    {
        LogError( "Receive Data: Bad param" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        pInputLine = pAtResp->pItm->pLine;
        location = strstr( pInputLine, "QIRDI" );

        while( location != NULL )
        {
            if( pAtResp->pItm->pNext != NULL )
            {
                pInputLine = pAtResp->pItm->pNext->pLine;
                location = strstr( pInputLine, "QIRDI" );
            }
            else
            {
                /* */
            }
        }

        {
            atCoreStatus = Cellular_ATRemovePrefix( &pInputLine );
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
            atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );

            /* parse the datalen. */
            if( atCoreStatus == CELLULAR_AT_SUCCESS )
            {
                atCoreStatus = Cellular_ATGetNextTok( &pInputLine, &pToken );
            }

            if( atCoreStatus == CELLULAR_AT_SUCCESS )
            {
                atCoreStatus = Cellular_ATStrtoi( pToken, 10, &tempValue );

                if( atCoreStatus == CELLULAR_AT_SUCCESS )
                {
                    if( ( tempValue >= ( int32_t ) 0 ) && ( tempValue < ( ( int32_t ) CELLULAR_MAX_RECV_DATA_LEN + 1 ) ) )
                    {
                        *pDataRecv->pDataLen = ( uint32_t ) tempValue;
                    }
                    else
                    {
                        LogError( "Error in Data Length Processing: No valid digit found. Token %s", pToken );
                        atCoreStatus = CELLULAR_AT_ERROR;
                    }
                }
            }

            /* Process the data buffer. */
            if( atCoreStatus == CELLULAR_AT_SUCCESS )
            {
                atCoreStatus = getDataFromResp( pAtResp, pDataRecv, dataLen );
            }
        }


        pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    }

    return pktStatus;
}

/*-----------------------------------------------------------*/

static CellularPktStatus_t socketRecvDataPrefix( void * pCallbackContext,
                                                 char * pLine,
                                                 uint32_t lineLength,
                                                 char ** ppDataStart,
                                                 uint32_t * pDataLength )
{
    char * pDataStart = NULL;
    char * pSearchStr = NULL;
    uint32_t tempStrlen = 0;
    int32_t tempValue = 0;
    CellularATError_t atResult = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    uint32_t i = 0;
    char * pToken = NULL;
    char pLocalLine[ MAX_QIRD_STRING_PREFIX_STRING + 1 ] = "\0";

    uint32_t localLineLength = MAX_QIRD_STRING_PREFIX_STRING > lineLength ? lineLength : MAX_QIRD_STRING_PREFIX_STRING;

    ( void ) pCallbackContext;

    if( ( pLine == NULL ) || ( ppDataStart == NULL ) || ( pDataLength == NULL ) )
    {
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else
    {
        /* Check if the message is a data response. */
        if( strncmp( pLine, DATA_PREFIX_STRING, DATA_PREFIX_STRING_LENGTH ) == 0 )
        {
            strncpy( pLocalLine, pLine, MAX_QIRD_STRING_PREFIX_STRING );
            pLocalLine[ MAX_QIRD_STRING_PREFIX_STRING ] = '\0';
            pDataStart = pLocalLine;

            /*[+QIRD: <ipAddress>:<port>,<type>,<length><CR><LF><data>]
             * pToken = +QIRD: <ipAddress>:<port>
             * pDataStart=<type>,<length><CR><LF><data> */
            atResult = Cellular_ATGetNextTok( &pDataStart, &pToken );

            /* pToken : <type>
             * pDataStart=<length><CR><LF><data>*/
            atResult = Cellular_ATGetNextTok( &pDataStart, &pToken );

            /* Add a '\0' char at the end +QIRD of the line. */
            for( i = 0; i < localLineLength; i++ )
            {
                if( ( pDataStart[ i ] == '\r' ) || ( pDataStart[ i ] == '\n' ) )
                {
                    pDataStart[ i ] = '\0';
                    break;
                }
            }

            /* Add "," to distinguish the data length from the IP address and port  */

            pSearchStr = Platform_Malloc( strlen( "," ) + strlen( pDataStart ) + 2 );
            sprintf( pSearchStr, "%s%s", ",", pDataStart );

            if( i == localLineLength )
            {
                LogDebug( "Data prefix invalid line : %s", pLocalLine );
                pDataStart = NULL;
            }
        }

        if( pDataStart != NULL )
        {
            atResult = Cellular_ATStrtoi( pDataStart, 10, &tempValue );

            if( ( atResult == CELLULAR_AT_SUCCESS ) && ( tempValue >= 0 ) &&
                ( tempValue <= ( int32_t ) CELLULAR_MAX_RECV_DATA_LEN ) )
            {
                /* Save the start of data point in pTemp. */
                if( ( uint32_t ) ( strnlen( pDataStart, MAX_QIRD_STRING_PREFIX_STRING ) + 2 ) > lineLength )
                {
                    /* More data is required. */
                    *pDataLength = 0;
                    pDataStart = NULL;
                    pktStatus = CELLULAR_PKT_STATUS_SIZE_MISMATCH;
                }
                else
                {
                    pDataStart = strstr( pLine, pSearchStr ) + strlen( pDataStart ) + 1;
                    pDataStart[ 0 ] = '\0';
                    pDataStart = &pDataStart[ 2 ];
                    *pDataLength = ( uint32_t ) tempValue;
                }

                LogDebug( "DataLength %p at pktIo = %d", pDataStart, *pDataLength );
            }
            else
            {
                *pDataLength = 0;
                pDataStart = NULL;
                LogError( "Data response received with wrong size" );
            }
        }

        *ppDataStart = pDataStart;
    }

    return pktStatus;
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
        LogError( "storeAccessModeAndAddress: Invalid socket address" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else if( socketHandle->socketState != SOCKETSTATE_ALLOCATED )
    {
        LogError( "storeAccessModeAndAddress, bad socket state %d",
                  socketHandle->socketState );
        cellularStatus = CELLULAR_INTERNAL_FAILURE;
    }
    else if( dataAccessMode != CELLULAR_ACCESSMODE_BUFFER )
    {
        LogError( "storeAccessModeAndAddress, Access mode not supported %d",
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
                    LogDebug( "_dnsResultCallback convert string failed %s", pToken );
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
                LogDebug( "_dnsResultCallback sends pktDnsQueue fail" );
            }
        }
        else
        {
            LogDebug( "_dnsResultCallback spurious DNS response" );
        }
    }
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_SetDns( CellularHandle_t cellularHandle,
                                 uint8_t contextId,
                                 const char * pDnsServerAddress )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqSetDns =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( pDnsServerAddress == NULL )
    {
        LogError( "Cellular_SetDns: Invalid parameter" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        cellularStatus = _Cellular_IsValidPdn( contextId );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_MAX_SIZE, "%s%d,\"%s\"", "AT+QIDNSCFG=", contextId, pDnsServerAddress );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSetDns );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_SetDns: couldn't set the DNS, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
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
            /* The return value of snprintf is not used.
             * The max length of the string is fixed and checked offline. */
            /* coverity[misra_c_2012_rule_21_6_violation]. */
            ( void ) snprintf( cmdBuf, cmdLen, "\"" PRINTF_BINARY_PATTERN_INT8 "\"%c",
                               PRINTF_BYTE_TO_BINARY_INT8( value ), endOfString ? '\0' : ',' );
        }
        else
        {
            /* The return value of snprintf is not used.
             * The max length of the string is fixed and checked offline. */
            /* coverity[misra_c_2012_rule_21_6_violation]. */
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
        LogError( "socketSendDataPrefix: pLine is invalid or pBytesRead is invalid" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( pCallbackContext != NULL )
    {
        LogError( "socketSendDataPrefix: pCallbackContext is not NULL" );
        pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
    }
    else if( *pBytesRead != 2U )
    {
        LogDebug( "socketSendDataPrefix: pBytesRead %u %s is not 1", *pBytesRead, pLine );
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
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_DeactivatePdn( CellularHandle_t cellularHandle,
                                        uint8_t contextId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqDeactPdn =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    cellularStatus = _Cellular_IsValidPdn( contextId );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "%s%d", "AT+QIDEACT=", contextId );
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqDeactPdn, PDN_DEACTIVATION_PACKET_REQ_TIMEOUT_MS );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_DeactivatePdn: can't deactivate PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_ActivatePdn( CellularHandle_t cellularHandle,
                                      uint8_t contextId )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };

    CellularAtReq_t atReqActPdn =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0,
    };

    cellularStatus = _Cellular_IsValidPdn( contextId );

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "%s", "AT+QIACT" );
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqActPdn, PDN_ACTIVATION_PACKET_REQ_TIMEOUT_MS );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_ActivatePdn: can't activate PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_SetPdnConfig( CellularHandle_t cellularHandle,
                                       uint8_t contextId,
                                       const CellularPdnConfig_t * pPdnConfig )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { '\0' };
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
        LogError( "Cellular_ATCommandRaw: Input parameter is NULL" );
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        cellularStatus = _Cellular_IsValidPdn( contextId );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Make sure the library is open. */
        cellularStatus = _Cellular_CheckLibraryStatus( pContext );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_MAX_SIZE, "%s%d,\"%s\",\"%s\",\"%s\",%d",
                           "AT+QICSGP=",
                           contextId,
                           pPdnConfig->apnName,
                           pPdnConfig->username,
                           pPdnConfig->password,
                           pPdnConfig->pdnAuthType );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqSetPdn );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_SetPdnConfig: can't set PDN, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_GetSignalInfo( CellularHandle_t cellularHandle,
                                        CellularSignalInfo_t * pSignalInfo )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularRat_t rat = CELLULAR_RAT_INVALID;
    CellularAtReq_t atReqQuerySignalInfo =
    {
        "AT+QCSQ",
        CELLULAR_AT_WITH_PREFIX,
        "+QCSQ",
        _Cellular_RecvFuncGetSignalInfo,
        pSignalInfo,
        sizeof( CellularSignalInfo_t ),
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
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
/* coverity[misra_c_2012_rule_8_7_violation] */
/* coverity[misra_c_2012_rule_8_13_violation] */
CellularError_t Cellular_SocketRecv( CellularHandle_t cellularHandle,
                                     CellularSocketHandle_t socketHandle,
                                     /* coverity[misra_c_2012_rule_8_13_violation] */
                                     uint8_t * pBuffer,
                                     uint32_t bufferLength,
                                     /* coverity[misra_c_2012_rule_8_13_violation] */
                                     uint32_t * pReceivedDataLength )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_TYPICAL_MAX_SIZE ] = { '\0' };
    uint32_t recvTimeout = DATA_READ_TIMEOUT_MS;
    uint32_t recvLen = bufferLength;
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
        "+QIRD",
        _Cellular_RecvFuncData,
        ( void * ) &dataRecv,
        bufferLength,
    };

    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pBuffer == NULL ) || ( pReceivedDataLength == NULL ) || ( bufferLength == 0U ) )
    {
        LogError( "_Cellular_RecvData: Bad input Param" );
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

        /* Form the AT command. */

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */

        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE,
                           "AT+QIRD=%d,%d,%ld,%ld", DEFAULT_FOREGROUND_CONTEXT, USE_OPERATION_MODE, socketHandle->socketId, recvLen );
        pktStatus = _Cellular_TimeoutAtcmdDataRecvRequestWithCallback( pContext,
                                                                       atReqSocketRecv, recvTimeout, socketRecvDataPrefix, NULL );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            /* Reset data handling parameters. */
            LogError( "_Cellular_RecvData: Data Receive fail, pktStatus: %d", pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
/* coverity[misra_c_2012_rule_8_13_violation] */
CellularError_t Cellular_SocketSend( CellularHandle_t cellularHandle,
                                     CellularSocketHandle_t socketHandle,
                                     const uint8_t * pData,
                                     uint32_t dataLength,
                                     /* coverity[misra_c_2012_rule_8_13_violation] */
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
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( socketHandle == NULL )
    {
        cellularStatus = CELLULAR_INVALID_HANDLE;
    }
    else if( ( pData == NULL ) || ( pSentDataLength == NULL ) || ( dataLength == 0U ) )
    {
        LogError( "Cellular_SocketSend: Invalid parameter" );
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

        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "%s%ld,%ld",
                           "AT+QISEND=", socketHandle->socketId, atDataReqSocketSend.dataLen );

        pktStatus = _Cellular_AtcmdDataSend( pContext, atReqSocketSend, atDataReqSocketSend,
                                             socketSendDataPrefix, NULL,
                                             PACKET_REQ_TIMEOUT_MS, sendTimeout, 0U );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_SocketSend: Data send fail, PktRet: %d", pktStatus );
            cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
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
            /* coverity[misra_c_2012_rule_21_6_violation]. */
            ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_TYPICAL_MAX_SIZE, "%s%ld%s", "AT+QICLOSE=", socketHandle->socketId, ";+QICLOSE=?" );
            pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqSockClose,
                                                                   SOCKET_DISCONNECT_PACKET_REQ_TIMEOUT_MS );

            if( pktStatus != CELLULAR_PKT_STATUS_OK )
            {
                LogError( "Cellular_SocketClose: Socket close failed, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
            }
        }

        /* Ignore the result from the info, and force to remove the socket. */
        cellularStatus = _Cellular_RemoveSocketData( pContext, socketHandle );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_SocketConnect( CellularHandle_t cellularHandle,
                                        CellularSocketHandle_t socketHandle,
                                        CellularSocketAccessMode_t dataAccessMode,
                                        const CellularSocketAddress_t * pRemoteSocketAddress )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    char cmdBuf[ CELLULAR_AT_CMD_MAX_SIZE ] = { '\0' };
    CellularAtReq_t atReqSocketConnect =
    {
        cmdBuf,
        CELLULAR_AT_NO_RESULT,
        NULL,
        NULL,
        NULL,
        0
    };

    /* Make sure the library is open. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
    }
    else if( pRemoteSocketAddress == NULL )
    {
        LogError( "Cellular_SocketConnect: Invalid socket address" );
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

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        /* Builds the Socket connect command. */
        cellularStatus = buildSocketConnect( socketHandle, cmdBuf );
    }

    if( cellularStatus == CELLULAR_SUCCESS )
    {
        pktStatus = _Cellular_TimeoutAtcmdRequestWithCallback( pContext, atReqSocketConnect,
                                                               SOCKET_CONNECT_PACKET_REQ_TIMEOUT_MS );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_SocketConnect: Socket connect failed, cmdBuf:%s, PktRet: %d", cmdBuf, pktStatus );
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

/* FreeRTOS Cellular Library types. */
/* coverity[misra_c_2012_rule_8_13_violation] */


/*-----------------------------------------------------------*/
/* internal function of _parseSocketOpen to reduce complexity. */
static CellularPktStatus_t _parseSocketOpenNextTok( const char * pToken,
                                                    uint32_t sockIndex,
                                                    CellularSocketContext_t * pSocketData )
{
    int32_t sockStatus = 0;
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;

    if( atCoreStatus == CELLULAR_AT_SUCCESS )
    {
        if( strcmp( pToken, "CONNECTOK" ) != 0 )
        {
            pSocketData->socketState = SOCKETSTATE_DISCONNECTED;
            LogError( "_parseSocketOpen: Socket open failed, conn %d, status %d", sockIndex, sockStatus );
        }
        else
        {
            pSocketData->socketState = SOCKETSTATE_CONNECTED;
            LogDebug( "_parseSocketOpen: Socket open success, conn %d", sockIndex );
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
            LogError( "_parseSocketOpen: Socket open callback for conn %d is not set!!", sockIndex );
        }
    }

    pktStatus = _Cellular_TranslateAtCoreStatus( atCoreStatus );
    return pktStatus;
}
/*-----------------------------------------------------------*/
/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
/* coverity[misra_c_2012_rule_8_13_violation] */
CellularError_t Cellular_GetPdnStatus( CellularHandle_t cellularHandle,
                                       CellularPdnStatus_t * pPdnStatusBuffers,
                                       uint8_t numStatusBuffers,
                                       uint8_t * pNumStatus )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    const CellularPdnStatus_t * pTempPdnStatusBuffer = pPdnStatusBuffers;
    uint8_t numBuffers = 0;
    CellularAtReq_t atReqGetPdnStatus =
    {
        "AT+QIACT?",
        CELLULAR_AT_WITH_PREFIX,
        "+QIACT",
        _Cellular_RecvFuncGetPdnStatus,
        pPdnStatusBuffers,
        numStatusBuffers,
    };

    if( ( pTempPdnStatusBuffer == NULL ) || ( pNumStatus == NULL ) || ( numStatusBuffers < 1u ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
        LogWarn( "_Cellular_GetPdnStatus: Bad input Parameter " );
    }

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
        /* Populate the Valid number of statuses. */
        *pNumStatus = 0;
        numBuffers = numStatusBuffers;

        while( numBuffers != 0U )
        {
            /* Check if the PDN state is valid. The context ID of the first
             * invalid PDN status is set to FF. */
            if( ( pTempPdnStatusBuffer->contextId <= CELLULAR_PDN_CONTEXT_ID_MAX ) &&
                ( pTempPdnStatusBuffer->contextId != INVALID_PDN_INDEX ) )
            {
                ( *pNumStatus ) += 1U;
            }
            else
            {
                break;
            }

            numBuffers--;
            pTempPdnStatusBuffer++;
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_GetSimCardStatus( CellularHandle_t cellularHandle,
                                           CellularSimCardStatus_t * pSimCardStatus )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtReq_t atReqGetSimCardStatus =
    {
        "AT+QSIMSTAT?",
        CELLULAR_AT_WITH_PREFIX,
        "+QSIMSTAT",
        _Cellular_RecvFuncGetSimCardStatus,
        &pSimCardStatus->simCardState,
        sizeof( CellularSimCardState_t ),
    };
    CellularAtReq_t atReqGetSimLockStatus =
    {
        "AT+CPIN?",
        CELLULAR_AT_WITH_PREFIX,
        "+CPIN",
        _Cellular_RecvFuncGetSimLockStatus,
        &pSimCardStatus->simCardLockState,
        sizeof( CellularSimCardLockState_t ),
    };

    /* pContext is checked in _Cellular_CheckLibraryStatus function. */
    cellularStatus = _Cellular_CheckLibraryStatus( pContext );

    if( cellularStatus != CELLULAR_SUCCESS )
    {
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
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

        if( pktStatus == CELLULAR_PKT_STATUS_OK )
        {
            pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqGetSimLockStatus );
        }

        cellularStatus = _Cellular_TranslatePktStatus( pktStatus );
        LogDebug( "_Cellular_GetSimStatus, Sim Insert State[%d], Lock State[%d]",
                  pSimCardStatus->simCardState, pSimCardStatus->simCardLockState );
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
CellularError_t Cellular_GetSimCardInfo( CellularHandle_t cellularHandle,
                                         CellularSimCardInfo_t * pSimCardInfo )
{
    CellularContext_t * pContext = ( CellularContext_t * ) cellularHandle;
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;

    CellularAtReq_t atReqGetIccid =
    {
        "AT+QCCID",
        CELLULAR_AT_WITH_PREFIX,
        "+QCCID",
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
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
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
            LogDebug( "SimInfo updated: IMSI:%s, Hplmn:%s%s, ICCID:%s",
                      pSimCardInfo->imsi, pSimCardInfo->plmn.mcc, pSimCardInfo->plmn.mnc,
                      pSimCardInfo->iccid );
        }
    }

    return cellularStatus;
}

/*-----------------------------------------------------------*/

/* FreeRTOS Cellular Library API. */
/* coverity[misra_c_2012_rule_8_7_violation] */
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
/* coverity[misra_c_2012_rule_8_7_violation] */
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
        LogDebug( "_Cellular_CheckLibraryStatus failed" );
    }
    else if( ( pcHostName == NULL ) || ( pResolvedAddress == NULL ) )
    {
        cellularStatus = CELLULAR_BAD_PARAMETER;
    }
    else
    {
        cellularStatus = _Cellular_IsValidPdn( contextId );
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
        /* The return value of snprintf is not used.
         * The max length of the string is fixed and checked offline. */
        /* coverity[misra_c_2012_rule_21_6_violation]. */
        ( void ) snprintf( cmdBuf, CELLULAR_AT_CMD_QUERY_DNS_MAX_SIZE,
                           "AT+QIDNSGIP=%u,\"%s\"", contextId, pcHostName );
        pktStatus = _Cellular_AtcmdRequestWithCallback( pContext, atReqQueryDns );

        if( pktStatus != CELLULAR_PKT_STATUS_OK )
        {
            LogError( "Cellular_GetHostByName: couldn't resolve host name" );
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
