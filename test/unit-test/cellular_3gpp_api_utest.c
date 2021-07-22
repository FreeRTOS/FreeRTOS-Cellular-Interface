/*
 * cellular_library v0.0.1
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
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
 */

/**
 * @file cellular_3gpp_api_utest.c
 * @brief Unit tests for functions in cellular_common_api.h.
 */

#include <string.h>
#include <stdbool.h>

#include "unity.h"

#include "cellular_config.h"
#include "cellular_config_defaults.h"

/* Include paths for public enums, structures, and macros. */
#include "cellular_platform.h"
#include "cellular_common_internal.h"
#include "cellular_common_api.h"
#include "cellular_types.h"

#include "mock_cellular_common.h"
#include "mock_cellular_at_core.h"


/**
 * @brief Cellular sample prefix string input.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_INPUT     "+CPIN:READY"

/**
 * @brief Cellular sample prefix string output.
 */
#define CELLULAR_SAMPLE_PREFIX_STRING_OUTPUT    "READY"

/**
 * @brief operator information.
 */
typedef struct cellularOperatorInfo
{
    CellularPlmnInfo_t plmnInfo;                             /* Device registered PLMN info (MCC and MNC).  */
    CellularRat_t rat;                                       /* Device registered Radio Access Technology (Cat-M, Cat-NB, GPRS etc).  */
    CellularNetworkRegistrationMode_t networkRegMode;        /* Network Registered mode of the device (Manual, Auto etc).   */
    CellularOperatorNameFormat_t operatorNameFormat;         /* Format of registered network operator name. */
    char operatorName[ CELLULAR_NETWORK_NAME_MAX_SIZE + 1 ]; /* Registered network operator name. */
} cellularOperatorInfo_t;

typedef enum parseTimeConditionState
{
    PARSE_TIME_FIRST_CALL_FAILURE_CONDITION = 6,
    PARSE_TIME_SECOND_CALL_FAILURE_CONDITION = 7,
    PARSE_TIME_THRID_CALL_FAILURE_CONDITION = 8,
    PARSE_TIME_FOURTH_CALL_FAILURE_CONDITION = 9,
    PARSE_TIME_FIFTH_CALL_FAILURE_CONDITION = 10,
    PARSE_TIME_SIXTH_CALL_FAILURE_CONDITION = 11
} parseTimeConditionState_t;

static int cbCondition = 0;
static int parseRegFailureCase = 0;
static int parseNetworkTimeFailureCase = 0;
static int parseNetworkNameFailureCase = 0;
static int cpsms_pos_mode_error = 0;
static int recvFuncGetHplmnCase = 0;
static int parseHplmn_test = 0;
static int simLockStateTestCase = 0;
static int psmSettingsTimerIndex = 0;
static int psmSettingsTimerError = 0;
static int parseEidrxTokenOutOfRange = 0;
static int mallocAllocFail = 0;

/* ============================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    cbCondition = 0;
    parseRegFailureCase = 0;
    parseNetworkTimeFailureCase = 0;
    parseNetworkNameFailureCase = 0;
    cpsms_pos_mode_error = 0;
    recvFuncGetHplmnCase = 0;
    parseHplmn_test = 0;
    simLockStateTestCase = 0;
    psmSettingsTimerIndex = 0;
    psmSettingsTimerError = 0;
    parseEidrxTokenOutOfRange = 0;
    mallocAllocFail = 0;
}

/* Called after each test method. */
void tearDown()
{
}

/* Called at the beginning of the whole suite. */
void suiteSetUp()
{
}

/* Called at the end of the whole suite. */
int suiteTearDown( int numFailures )
{
    return numFailures;
}

/* ========================================================================== */

/* static function declaration */

static CellularCommInterfaceError_t _prvCommIntfOpen( CellularCommInterfaceReceiveCallback_t receiveCallback,
                                                      void * pUserData,
                                                      CellularCommInterfaceHandle_t * pCommInterfaceHandle )
{
    CellularCommInterfaceError_t commIntRet = IOT_COMM_INTERFACE_SUCCESS;

    ( void ) receiveCallback;
    ( void ) pCommInterfaceHandle;
    CellularContext_t * pContext = ( CellularContext_t * ) pUserData;

    commIntRet = receiveCallback( pContext, NULL );

    return commIntRet;
}

static CellularCommInterfaceError_t _prvCommIntfClose( CellularCommInterfaceHandle_t commInterfaceHandle )
{
    CellularCommInterfaceError_t commIntRet = IOT_COMM_INTERFACE_SUCCESS;

    ( void ) commInterfaceHandle;

    return commIntRet;
}

static CellularCommInterfaceError_t _prvCommIntfSend( CellularCommInterfaceHandle_t commInterfaceHandle,
                                                      const uint8_t * pData,
                                                      uint32_t dataLength,
                                                      uint32_t timeoutMilliseconds,
                                                      uint32_t * pDataSentLength )
{
    CellularCommInterfaceError_t commIntRet = IOT_COMM_INTERFACE_SUCCESS;

    ( void ) commInterfaceHandle;
    ( void ) pData;
    ( void ) dataLength;
    ( void ) timeoutMilliseconds;
    ( void ) pDataSentLength;

    *pDataSentLength = dataLength;

    return commIntRet;
}

static CellularCommInterfaceError_t _prvCommIntfReceive( CellularCommInterfaceHandle_t commInterfaceHandle,
                                                         uint8_t * pBuffer,
                                                         uint32_t bufferLength,
                                                         uint32_t timeoutMilliseconds,
                                                         uint32_t * pDataReceivedLength )
{
    CellularCommInterfaceError_t commIntRet = IOT_COMM_INTERFACE_SUCCESS;

    ( void ) commInterfaceHandle;
    ( void ) pBuffer;
    ( void ) bufferLength;
    ( void ) timeoutMilliseconds;
    ( void ) pDataReceivedLength;

    return commIntRet;
}

bool MockPlatformMutex_Create( PlatformMutex_t * pNewMutex,
                               bool recursive )
{
    ( void )pNewMutex;
    pNewMutex->created = true;
    return true;
}

void MockPlatformMutex_Unlock( PlatformMutex_t * pMutex )
{
    ( void )pMutex;
}

void MockPlatformMutex_Lock( PlatformMutex_t * pMutex )
{
    ( void )pMutex;
}

uint16_t MockvQueueDelete( QueueHandle_t queue )
{
    free( queue );
    queue = 0;
    return 1;
}

void MockPlatformMutex_Destroy( PlatformMutex_t * pMutex )
{
    pMutex->created = false;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback( CellularContext_t * pContext,
                                                   CellularAtReq_t atReq,
                                                   int cmock_num_calls )
{
    cellularOperatorInfo_t * pOperatorInfo = ( cellularOperatorInfo_t * ) atReq.pData;

    ( void )pContext;
    ( void)cmock_num_calls;
    pOperatorInfo->rat = CELLULAR_RAT_INVALID;

    return CELLULAR_PKT_STATUS_OK;
}

static void _saveData( char * pLine,
                       CellularATCommandResponse_t * pResp,
                       uint32_t dataLen )
{
    CellularATCommandLine_t * pNew = NULL, * pTemp = NULL;

    ( void ) dataLen;

    CellularLogDebug( "_saveData : Save data %p with length %d", pLine, dataLen );

    pNew = ( CellularATCommandLine_t * ) malloc( sizeof( CellularATCommandLine_t ) );
    /* coverity[misra_c_2012_rule_10_5_violation] */
    configASSERT( ( int32_t ) ( pNew != NULL ) );

    /* Reuse the pktio buffer instead of allocate. */
    pNew->pLine = pLine;
    pNew->pNext = NULL;

    if( pResp->pItm == NULL )
    {
        pResp->pItm = pNew;
    }
    else
    {
        pTemp = pResp->pItm;

        while( pTemp->pNext != NULL )
        {
            pTemp = pTemp->pNext;
        }

        pTemp->pNext = pNew;
    }
}

CellularPktStatus_t handleCommonCallback( CellularContext_t * pContext,
                                          CellularAtReq_t atReq )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATCommandResponse_t atResp;

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    /* null context */
    if( cbCondition == 0 )
    {
        pktStatus = atReq.respCallback( NULL, NULL, NULL, 0 );
    }
    /* bad parameter */
    else if( cbCondition == 1 )
    {
        pktStatus = atReq.respCallback( pContext, NULL, NULL, 0 );
    }
    /* null data pointer */
    else if( cbCondition == 2 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( pContext, &atResp, NULL, 0 );
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime( CellularContext_t * pContext,
                                                                                    CellularAtReq_t atReq,
                                                                                    int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularATCommandResponse_t atResp;
    char pData[ 20 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( ( cbCondition == 3 ) || ( cbCondition == 4 ) )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( pContext, &atResp, pData, sizeof( pData ) );
    }
    else if( cbCondition == 5 )
    {
        char pLine[] = "-0519−00402";

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( pContext, &atResp, pData, sizeof( pData ) );
    }
    else if( cbCondition >= 6 )
    {
        char pLine[] = "+255";

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( pContext, &atResp, pData, sizeof( pData ) );
    }

    return pktStatus;
}

CellularATError_t Mock_Cellular_ATStrtoi( const char * pStr,
                                          int32_t base,
                                          int32_t * pResult,
                                          int cmock_num_calls )
{
    ( void )base;

    if( cbCondition < PARSE_TIME_FIRST_CALL_FAILURE_CONDITION )
    {
        *pResult = atoi( pStr );
    }
    else
    {
        if( cbCondition == PARSE_TIME_FIRST_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 1 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
        else if( cbCondition == PARSE_TIME_SECOND_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 2 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
        else if( cbCondition == PARSE_TIME_THRID_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 3 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
        else if( cbCondition == PARSE_TIME_FOURTH_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 4 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
        else if( cbCondition == PARSE_TIME_FIFTH_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 5 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
        else if( cbCondition == PARSE_TIME_SIXTH_CALL_FAILURE_CONDITION )
        {
            if( cmock_num_calls == 6 )
            {
                *pResult = atoi( "+655350" );
            }
            else
            {
                *pResult = atoi( pStr );
            }
        }
    }

    return CELLULAR_AT_SUCCESS;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetFirmwareVersion( CellularContext_t * pContext,
                                                                                                  CellularAtReq_t atReq,
                                                                                                  int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ CELLULAR_FW_VERSION_MAX_SIZE + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetImei( CellularContext_t * pContext,
                                                                                       CellularAtReq_t atReq,
                                                                                       int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ CELLULAR_IMEI_MAX_SIZE + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls > 0 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetModelId( CellularContext_t * pContext,
                                                                                CellularAtReq_t atReq,
                                                                                int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ CELLULAR_MODEL_ID_MAX_SIZE + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls > 1 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetManufactureId( CellularContext_t * pContext,
                                                                                      CellularAtReq_t atReq,
                                                                                      int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ CELLULAR_MANUFACTURE_ID_MAX_SIZE + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls > 2 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkReg( CellularContext_t * pContext,
                                                                                   CellularAtReq_t atReq,
                                                                                   int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ sizeof( CellularNetworkRegType_t ) ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls < 1 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc( CellularContext_t * pContext,
                                                                                  CellularAtReq_t atReq,
                                                                                  int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ sizeof( cellularOperatorInfo_t ) ];

    ( void )cmock_num_calls;
    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );

        if( parseNetworkNameFailureCase == 2 )
        {
            cellularOperatorInfo_t cellularOperatorInfo;
            memset( &cellularOperatorInfo, 0, sizeof( cellularOperatorInfo_t ) );
            cellularOperatorInfo.operatorNameFormat = OPERATOR_NAME_FORMAT_LONG;
            pktStatus = atReq.respCallback( &context, &atResp, &cellularOperatorInfo, sizeof( cellularOperatorInfo_t ) );
        }
        else if( parseNetworkNameFailureCase == 3 )
        {
            cellularOperatorInfo_t cellularOperatorInfo;
            memset( &cellularOperatorInfo, 0, sizeof( cellularOperatorInfo_t ) );
            cellularOperatorInfo.operatorNameFormat = OPERATOR_NAME_FORMAT_NUMERIC;
            pktStatus = atReq.respCallback( &context, &atResp, &cellularOperatorInfo, sizeof( cellularOperatorInfo_t ) );
        }
        else
        {
            pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_Cellular_RecvFuncGetEidrxSettings( CellularContext_t * pContext,
                                                                                               CellularAtReq_t atReq,
                                                                                               int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ 2 ];

    ( void )cmock_num_calls;
    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
    }
    else if( cbCondition == 4 )
    {
        char pLine[ 1 ];
        pLine[ 0 ] = '\0';

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, pData, sizeof( pData ) );
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings( CellularContext_t * pContext,
                                                                                    CellularAtReq_t atReq,
                                                                                    int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    CellularPsmSettings_t cellularPsmSettings;

    ( void )cmock_num_calls;
    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularPsmSettings, sizeof( CellularPsmSettings_t ) );
    }
    else if( cbCondition == 4 )
    {
        char pLine[] = "+CEDRXS: 0";

        _saveData( pLine, &atResp, sizeof( pLine ) );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularPsmSettings, sizeof( CellularPsmSettings_t ) );
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid( CellularContext_t * pContext,
                                                                              CellularAtReq_t atReq,
                                                                              int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ strlen( CELLULAR_SAMPLE_PREFIX_STRING_INPUT ) + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls == 2 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, CELLULAR_ICCID_MAX_SIZE + 1 );
        }
        else if( cbCondition == 4 )
        {
            char pLine[] = "This is testing string for greater than 20.";

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, CELLULAR_ICCID_MAX_SIZE + 1 );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi( CellularContext_t * pContext,
                                                                             CellularAtReq_t atReq,
                                                                             int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    char pData[ strlen( CELLULAR_SAMPLE_PREFIX_STRING_INPUT ) + 1 ];

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls == 0 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, CELLULAR_IMSI_MAX_SIZE + 1 );
        }
        else if( cbCondition == 4 )
        {
            char pLine[] = "This is the testing long string.";

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, pData, CELLULAR_IMSI_MAX_SIZE + 1 );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn( CellularContext_t * pContext,
                                                                              CellularAtReq_t atReq,
                                                                              int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    CellularPlmnInfo_t cellularPlmnInfo;

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cmock_num_calls == 1 )
    {
        if( cbCondition < 3 )
        {
            pktStatus = handleCommonCallback( pContext, atReq );
        }
        else if( cbCondition == 3 )
        {
            char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

            _saveData( pLine, &atResp, strlen( pLine ) + 1 );
            pktStatus = atReq.respCallback( &context, &atResp, &cellularPlmnInfo, sizeof( CellularPlmnInfo_t ) );
        }
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress( CellularContext_t * pContext,
                                                                               CellularAtReq_t atReq,
                                                                               int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    CellularEidrxSettingsList_t cellularEidrxSettingsList;

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularEidrxSettingsList, sizeof( CELLULAR_EDRX_LIST_MAX_SIZE ) );
    }
    else if( cbCondition == 4 )
    {
        char pLine[] = "+CEDRXS: 0";

        _saveData( pLine, &atResp, sizeof( pLine ) );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularEidrxSettingsList, sizeof( CELLULAR_EDRX_LIST_MAX_SIZE ) );
    }
    else if( cbCondition == 5 )
    {
        char pLine = '\0';

        _saveData( &pLine, &atResp, sizeof( pLine ) );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularEidrxSettingsList, sizeof( CELLULAR_EDRX_LIST_MAX_SIZE ) );
    }

    return pktStatus;
}

CellularPktStatus_t Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus( CellularContext_t * pContext,
                                                                                      CellularAtReq_t atReq,
                                                                                      int cmock_num_calls )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;
    CellularATCommandResponse_t atResp;
    CellularSimCardLockState_t cellularSimCardLockState;

    memset( &atResp, 0, sizeof( CellularATCommandResponse_t ) );

    if( cbCondition < 3 )
    {
        pktStatus = handleCommonCallback( pContext, atReq );
    }
    else if( cbCondition == 3 )
    {
        char pLine[] = CELLULAR_SAMPLE_PREFIX_STRING_INPUT;

        _saveData( pLine, &atResp, strlen( pLine ) + 1 );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularSimCardLockState, sizeof( CellularSimCardLockState_t ) );
    }
    else if( cbCondition == 4 )
    {
        char pLine[] = "+CEDRXS: 0";

        _saveData( pLine, &atResp, sizeof( pLine ) );
        pktStatus = atReq.respCallback( &context, &atResp, &cellularSimCardLockState, sizeof( CellularSimCardLockState_t ) );
    }

    return pktStatus;
}

void handle_simLockStateTestCase( char ** ppTokOutput,
                                  int simLockStateTestCase )
{
    char * pStsToken = NULL;

    switch( simLockStateTestCase )
    {
        case 1:
            *ppTokOutput = malloc( strlen( "READY" ) + 1 );
            strcpy( *ppTokOutput, "READY" );
            break;

        case 2:
            *ppTokOutput = malloc( strlen( "SIM PIN" ) + 1 );
            strcpy( *ppTokOutput, "SIM PIN" );
            break;

        case 3:
            *ppTokOutput = malloc( strlen( "SIM PUK" ) + 1 );
            strcpy( *ppTokOutput, "SIM PUK" );
            break;

        case 4:
            *ppTokOutput = malloc( strlen( "SIM PIN2" ) + 1 );
            strcpy( *ppTokOutput, "SIM PIN2" );
            break;

        case 5:
            *ppTokOutput = malloc( strlen( "SIM PUK2" ) + 1 );
            strcpy( *ppTokOutput, "SIM PUK2" );
            break;

        case 6:
            *ppTokOutput = malloc( strlen( "PH-NET PIN" ) + 1 );
            strcpy( *ppTokOutput, "PH-NET PIN" );
            break;

        case 7:
            *ppTokOutput = malloc( strlen( "PH-NET PUK" ) + 1 );
            strcpy( *ppTokOutput, "PH-NET PUK" );
            break;

        case 8:
            *ppTokOutput = malloc( strlen( "PH-NETSUB PIN" ) + 1 );
            strcpy( *ppTokOutput, "PH-NETSUB PIN" );
            break;

        case 9:
            *ppTokOutput = malloc( strlen( "PH-NETSUB PUK" ) + 1 );
            strcpy( *ppTokOutput, "PH-NETSUB PUK" );
            break;

        case 10:
            *ppTokOutput = malloc( strlen( "PH-SP PIN" ) + 1 );
            strcpy( *ppTokOutput, "PH-SP PIN" );
            break;

        case 11:
            *ppTokOutput = malloc( strlen( "PH-SP PUK" ) + 1 );
            strcpy( *ppTokOutput, "PH-SP PUK" );
            break;

        case 12:
            *ppTokOutput = malloc( strlen( "PH-CORP PIN" ) + 1 );
            strcpy( *ppTokOutput, "PH-CORP PIN" );
            break;

        case 13:
            *ppTokOutput = malloc( strlen( "PH-CORP PUK" ) + 1 );
            strcpy( *ppTokOutput, "PH-CORP PUK" );
            break;

        case 14:
            *ppTokOutput = malloc( strlen( "123456789" ) + 1 );
            strcpy( *ppTokOutput, "123456789" );

        default:
            break;
    }
}

CellularATError_t Mock_simLockStateTestCase_ATGetNextTok_Calback( char ** ppString,
                                                                  char ** ppTokOutput,
                                                                  int cmock_num_calls )
{
    char pFitNum[] = "1";
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    ( void )ppString;

    if( *ppTokOutput )
    {
        free( *ppTokOutput );
        *ppTokOutput = NULL;
    }

    if( simLockStateTestCase > 0 )
    {
        handle_simLockStateTestCase( ppTokOutput, simLockStateTestCase );
    }
    else
    {
        *ppTokOutput = malloc( sizeof( pFitNum ) );
        memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
    }

    return atCoreStatus;
}

CellularATError_t Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback( char ** ppString,
                                                                         char ** ppTokOutput,
                                                                         int cmock_num_calls )
{
    char pFitNum[] = "1";
    char pOutOfNameFormat[] = "3";
    char pMcc[] = "2";
    char pNumericNum[] = "123456";
    char pBigNum[] = "32";
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;

    ( void )ppString;

    if( *ppTokOutput )
    {
        free( *ppTokOutput );
        *ppTokOutput = NULL;
    }

    if( cmock_num_calls == 0 )
    {
        char pFitNum[] = "1";
        *ppTokOutput = malloc( sizeof( pFitNum ) );
        memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
    }
    else if( cmock_num_calls == 1 )
    {
        if( ( parseNetworkNameFailureCase == 3 ) ||
            ( parseNetworkNameFailureCase == 5 ) )
        {
            *ppTokOutput = malloc( sizeof( pMcc ) );
            memcpy( *ppTokOutput, pMcc, sizeof( pMcc ) );
        }
        else if( parseNetworkNameFailureCase == 4 )
        {
            *ppTokOutput = malloc( sizeof( pOutOfNameFormat ) );
            memcpy( *ppTokOutput, pOutOfNameFormat, sizeof( pOutOfNameFormat ) );
        }
        else
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
    }
    else if( cmock_num_calls == 2 )
    {
        /* null data pointer. */
        if( parseNetworkNameFailureCase == 1 )
        {
            *ppTokOutput = NULL;
        }
        /* OPERATOR_NAME_FORMAT_SHORT. */
        else if( parseNetworkNameFailureCase == 2 )
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
        /* OPERATOR_NAME_FORMAT_NUMERIC. */
        else if( parseNetworkNameFailureCase == 3 )
        {
            *ppTokOutput = malloc( sizeof( pNumericNum ) );
            memcpy( *ppTokOutput, pNumericNum, sizeof( pNumericNum ) );
        }
        /* OPERATOR_NAME_FORMAT_NUMERIC wrong length. */
        else if( parseNetworkNameFailureCase == 5 )
        {
            *ppTokOutput = malloc( sizeof( pBigNum ) );
            memcpy( *ppTokOutput, pBigNum, sizeof( pBigNum ) );
        }
        else
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
    }
    else if( cmock_num_calls == 3 )
    {
        if( parseNetworkNameFailureCase == 7 )
        {
            char pTmp[] = "123456789123";
            *ppTokOutput = malloc( sizeof( pTmp ) );
            memcpy( *ppTokOutput, pTmp, sizeof( pTmp ) );
        }
        else if( parseNetworkNameFailureCase == 8 )
        {
            char pTmp[] = "8";
            *ppTokOutput = malloc( sizeof( pTmp ) );
            memcpy( *ppTokOutput, pTmp, sizeof( pTmp ) );
        }
    }

    if( ( parseNetworkNameFailureCase == 6 ) ||
        ( parseNetworkNameFailureCase == 1 ) )
    {
        atCoreStatus = CELLULAR_AT_SUCCESS;
    }

    return atCoreStatus;
}

CellularATError_t Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback( char ** ppString,
                                                                  char ** ppTokOutput,
                                                                  int cmock_num_calls )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char pFitNum[] = "1";

    ( void )ppString;

    if( *ppTokOutput )
    {
        free( *ppTokOutput );
        *ppTokOutput = NULL;
    }

    if( cmock_num_calls == 0 )
    {
        if( recvFuncGetHplmnCase == 1 )
        {
            char pSts[] = "11";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        else
        {
            char pSts[] = "144";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
    }
    else if( cmock_num_calls == 1 )
    {
        if( ( recvFuncGetHplmnCase == 2 ) )
        {
            *ppTokOutput = NULL;
        }
        else if( recvFuncGetHplmnCase == 3 )
        {
            char pSts[] = "64";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        else if( recvFuncGetHplmnCase > 3 )
        {
            char pSts[] = "4";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
    }
    else if( cmock_num_calls == 2 )
    {
        if( recvFuncGetHplmnCase == 0 )
        {
            if( parseHplmn_test == 0 )
            {
                char pSts[] = "This is testing string.";
                *ppTokOutput = malloc( sizeof( pSts ) );
                memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
            }
            else
            {
                char pSts[] = "01Ftesting string.";
                *ppTokOutput = malloc( sizeof( pSts ) );
                memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
            }
        }
        else if( recvFuncGetHplmnCase == 4 )
        {
            *ppTokOutput = NULL;
        }
        else if( recvFuncGetHplmnCase == 6 )
        {
            char pSts[] = "This is testing string.";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        else if( recvFuncGetHplmnCase == 7 )
        {
            char pSts[] = "0xF2345678";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        else
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
    }

    if( ( recvFuncGetHplmnCase == 4 ) ||
        ( recvFuncGetHplmnCase == 2 ) )
    {
        atCoreStatus = CELLULAR_AT_SUCCESS;
    }

    return atCoreStatus;
}

CellularATError_t Mock_Cellular_ATGetNextTok_Calback( char ** ppString,
                                                      char ** ppTokOutput,
                                                      int cmock_num_calls )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char pFitNum[] = "1";
    char pBigNum[] = "32";
    char pNumericNum[] = "123456";
    int len = 10;

    ( void )ppString;

    if( *ppTokOutput )
    {
        free( *ppTokOutput );
        *ppTokOutput = NULL;
    }

    if( cmock_num_calls == 0 )
    {
        if( parseEidrxTokenOutOfRange == 1 )
        {
            char pSts[] = "300";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        else if( parseRegFailureCase == 1 )
        {
            *ppTokOutput = malloc( sizeof( pBigNum ) );
            memcpy( *ppTokOutput, pBigNum, sizeof( pBigNum ) );
        }
        else
        {
            if( cpsms_pos_mode_error == 1 )
            {
                *ppTokOutput = malloc( sizeof( pNumericNum ) );
                memcpy( *ppTokOutput, pNumericNum, sizeof( pNumericNum ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
    }
    else if( cmock_num_calls == 1 )
    {
        if( parseEidrxTokenOutOfRange == 1 )
        {
            char pSts[] = "300";
            *ppTokOutput = malloc( sizeof( pSts ) );
            memcpy( *ppTokOutput, pSts, sizeof( pSts ) );
        }
        /* null data pointer. */
        else if( parseNetworkTimeFailureCase == 1 )
        {
            *ppTokOutput = NULL;
        }
        /* big number case or _parseCopsNetworkNameToken parse error case. */
        else if( parseNetworkTimeFailureCase == 2 )
        {
            *ppTokOutput = malloc( sizeof( pBigNum ) );
            memcpy( *ppTokOutput, pBigNum, sizeof( pBigNum ) );
        }
        else
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
    }
    else if( cmock_num_calls == 2 )
    {
        *ppTokOutput = malloc( sizeof( pFitNum ) );
        memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
    }
    else if( cmock_num_calls < 7 )
    {
        if( cmock_num_calls == 3 )
        {
            if( psmSettingsTimerIndex > 0 )
            {
                int t3412TimerInx = ( psmSettingsTimerIndex << 5U );

                if( psmSettingsTimerError == 1 )
                {
                    t3412TimerInx = 0 - t3412TimerInx;
                }

                *ppTokOutput = malloc( len );
                snprintf( *ppTokOutput, len, "%d", t3412TimerInx );
            }
        }
        else
        {
            if( ( cmock_num_calls == 4 ) && ( psmSettingsTimerIndex > 0 ) )
            {
                int t3324TimerInx = ( psmSettingsTimerIndex << 5U );

                if( psmSettingsTimerError == 1 )
                {
                    t3324TimerInx = 0 - t3324TimerInx;
                }

                *ppTokOutput = malloc( len );
                snprintf( *ppTokOutput, len, "%d", t3324TimerInx );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
    }
    else
    {
        atCoreStatus = CELLULAR_AT_ERROR;
    }

    if( *ppTokOutput == NULL )
    {
        atCoreStatus = CELLULAR_AT_ERROR;
    }

    if( parseNetworkTimeFailureCase == 1 )
    {
        atCoreStatus = CELLULAR_AT_SUCCESS;
    }

    return atCoreStatus;
}

void * mock_malloc( size_t size )
{
    if( mallocAllocFail == 1 )
    {
        return NULL;
    }

    return malloc( size );
}

/* ========================================================================== */

/**
 * @brief Test that any NULL handler case Cellular_CommonGetEidrxSettings to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetEidrxSettings_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetEidrxSettings( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that any NULL EidrxSettingsList case Cellular_CommonGetEidrxSettings to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetEidrxSettings_Null_EidrxSettings( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonGetEidrxSettings to return CELLULAR_TIMEOUT.
 */
void test_Cellular_CommonGetEidrxSettings_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetEidrxSettings
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncGetEidrxSettings_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_Cellular_RecvFuncGetEidrxSettings );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncIpAddress_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncIpAddress_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncIpAddress_Null_Line( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that CELLULAR_CEDRXS_POS_ACT case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncIpAddress_CEDRXS_POS_ACT_OutOfRange( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    parseEidrxTokenOutOfRange = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetEidrxSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetEidrxSettings_Cb_Cellular_RecvFuncIpAddress_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetEidrxSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetEidrxSettings_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettingsList_t eidrxSettingsList;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetEidrxSettings( cellularHandle, &eidrxSettingsList );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that any NULL handler case Cellular_CommonSetEidrxSettings to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonSetEidrxSettings_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonSetEidrxSettings( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that any NULL EidrxSettingsList case Cellular_CommonSetEidrxSettings to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonSetEidrxSettings_Null_EidrxSettings( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonSetEidrxSettings( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonSetEidrxSettings to return CELLULAR_TIMEOUT.
 */
void test_Cellular_CommonSetEidrxSettings_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettings_t eidrxSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );
    cellularStatus = Cellular_CommonSetEidrxSettings( cellularHandle, &eidrxSettings );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonSetEidrxSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonSetEidrxSettings_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularEidrxSettings_t eidrxSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonSetEidrxSettings( cellularHandle, &eidrxSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonRfOn to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonRfOn_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonRfOn( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonRfOn to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonRfOn_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonRfOn( cellularHandle );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonRfOff to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonRfOff_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonRfOff( NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonRfOff to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonRfOff_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonRfOff( cellularHandle );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetRegisteredNetwork to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetRegisteredNetwork to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetRegisteredNetwork_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetRegisteredNetwork to return CELLULAR_NO_MEMORY.
 */
void test_Cellular_CommonGetRegisteredNetwork_No_Memory( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    mallocAllocFail = 1;
    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_NO_MEMORY, cellularStatus );
}

/**
 * @brief Test that invalid rat case Cellular_CommonGetRegisteredNetwork to return CELLULAR_UNKNOWN.
 */
void test_Cellular_CommonGetRegisteredNetwork_Invalid_Rat( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_UNKNOWN, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command response case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_Null_AtCmd_Resp( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command response case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /*_parseCopsNetworkNameFormatToken*/
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseCopsRatToken */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsRegModeToken too big number case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsRegModeToken_Big_Num_Error( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseRegFailureCase = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameFormatToken null pointer case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameFormatToken_Null_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkTimeFailureCase = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameFormatToken big number case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameFormatToken_Big_Number( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkTimeFailureCase = 2;
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameToken null pointer case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameToken_Null_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameToken name format long case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameToken_Name_Format_Long( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 2;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameToken name format numeric case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameToken_Name_Format_Numeric( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 3;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameToken name format numeric wrong length case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameToken_Name_Format_Numeric_Wrong_Length( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 5;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsNetworkNameToken name format numeric parse error case in callback
 * function _Cellular_RecvFuncUpdateMccMnc for Cellular_CommonGetRegisteredNetwork to return
 * CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsNetworkNameToken_Parse_Error( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 4;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsRatToken null token case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsRatToken_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 6;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsRatToken token out of range case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsRatToken_Token_OutOfRange( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 7;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseCopsRatToken happy path case in callback function _Cellular_RecvFuncUpdateMccMnc
 * for Cellular_CommonGetRegisteredNetwork to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetRegisteredNetwork_Cb__Cellular_RecvFuncUpdateMccMnc_parseCopsRatToken_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncUpdateMccMnc );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    /* _parseCops */
    parseNetworkNameFailureCase = 8;
    Cellular_ATGetNextTok_StubWithCallback( Mock_parseNetworkNameFailureCase_ATGetNextTok_Calback );
    /* _parseCopsRegModeToken */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetRegisteredNetwork to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetRegisteredNetwork_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPlmnInfo_t networkInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetRegisteredNetwork( cellularHandle, &networkInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetServiceStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetServiceStatus_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetServiceStatus( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetServiceStatus to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetServiceStatus_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetNetworkReg
 * for Cellular_CommonGetServiceStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetServiceStatus_Cb__Cellular_RecvFuncGetNetworkReg_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularServiceStatus_t serviceStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CREG. */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkReg );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    /* called by atcmdUpdateMccMnc. */
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, &serviceStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetNetworkReg
 * for Cellular_CommonGetServiceStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetServiceStatus_Cb__Cellular_RecvFuncGetNetworkReg_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularServiceStatus_t serviceStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CREG. */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkReg );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    /* called by atcmdUpdateMccMnc. */
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, &serviceStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that null at command response in callback function _Cellular_RecvFuncGetNetworkReg
 * for Cellular_CommonGetServiceStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetServiceStatus_Cb__Cellular_RecvFuncGetNetworkReg_Null_AtCmd_Resp( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularServiceStatus_t serviceStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CREG. */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkReg );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    /* called by atcmdUpdateMccMnc. */
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );


    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, &serviceStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path in callback function _Cellular_RecvFuncGetNetworkReg
 * for Cellular_CommonGetServiceStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetServiceStatus_Cb__Cellular_RecvFuncGetNetworkReg_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularServiceStatus_t serviceStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CREG. */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkReg );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    Cellular_ATRemoveLeadingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    /* _Cellular_ParseRegStatus */
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_BAD_PARAMETER );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_NetworkRegistrationCallback_Ignore();

    /* called by atcmdUpdateMccMnc. */
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, &serviceStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetServiceStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetServiceStatus_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularServiceStatus_t serviceStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CREG. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CGREG. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    /* called by atcmdQueryRegStatus -> queryNetworkStatus for CGREG -> queryNetworkStatus for CGREG. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    /* called by atcmdUpdateMccMnc. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetServiceStatus( cellularHandle, &serviceStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetNetworkTime to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetNetworkTime_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetNetworkTime( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetNetworkTime to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetNetworkTime_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that retrieve networ time failure case Cellular_CommonGetNetworkTime to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetNetworkTime_Retrieve_Network_Time_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );

    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that null context case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* Null context condition. */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command response case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Null_AtCmd( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* Null at command response case condition. */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Null_Data( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );

    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseTimeZoneInfo at command failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_TimeZone_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_BAD_PARAMETER );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );

    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseTimeZoneInCCLKResponse at command failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_TimeZone_CCLKResp_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = 5;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseYearMonthDayInCCLKResponse year atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_YearMonthDAy_CCLKResp_Year_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_FIRST_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseYearMonthDayInCCLKResponse month atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_YearMonthDAy_CCLKResp_Month_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_SECOND_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseYearMonthDayInCCLKResponse day atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_YearMonthDAy_CCLKResp_Day_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_THRID_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseTimeInCCLKResponse hour atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_TimeInCCLKResp_Hour_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_FOURTH_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseTimeInCCLKResponse minute atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_TimeInCCLKResp_Minute_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_FIFTH_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that _parseTimeInCCLKResponse second atoi failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Parse_TimeInCCLKResp_Second_Atoi_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* parse time failure case condition. */
    cbCondition = PARSE_TIME_SIXTH_CALL_FAILURE_CONDITION;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that at command failure case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* happy path case condition. */
    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_BAD_PARAMETER );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_BAD_PARAMETER, CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}


/**
 * @brief Test that happy path case in _Cellular_RecvFuncGetNetworkTime to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetNetworkTime_RecvFuncCallback_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* happy path case condition. */
    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetNetworkTime );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* Enter _parseTimeZoneInfo. */
    Cellular_ATRemoveOutermostDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeZoneInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseYearMonthDayInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    /* _parseTimeInCCLKResponse */
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    Cellular_ATGetSpecificNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATStrtoi_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetNetworkTime to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetNetworkTime_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularTime_t networkTime;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    cellularStatus = Cellular_CommonGetNetworkTime( cellularHandle, &networkTime );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetModemInfo to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetModemInfo_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetModemInfo( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetModemInfo to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetModemInfo_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that get firmware version failure case Cellular_CommonGetModemInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetModemInfo_Get_FW_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion CELLULAR_PKT_STATUS_BAD_PARAM failure*/
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    /* atReqGetImei */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetModelId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetManufactureId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get firmware version callback function null context failure
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_FW_Callback__Cellular_RecvFuncGetFirmwareVersion_Null_Context_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetFirmwareVersion );

    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get firmware version callback function null
 * at command response failure case Cellular_CommonGetModemInfo
 * to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_FW_Callback__Cellular_RecvFuncGetFirmwareVersion_Null_AtResp_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetFirmwareVersion );

    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get firmware version callback function null data pointer
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_FW_Callback__Cellular_RecvFuncGetFirmwareVersion_Null_Data_Pointer_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetFirmwareVersion );

    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get firmware version callback function happy path
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_FW_Callback__Cellular_RecvFuncGetFirmwareVersion_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetFirmwareVersion );
    Cellular_ATRemoveLeadingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveTrailingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that get imei version callback function null context failure
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetImei_Null_Context_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImei */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetImei );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get imei version callback function null at command
 * response failure case Cellular_CommonGetModemInfo
 * to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetImei_Null_AtResp_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImei */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetImei );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get imei version callback function null data pointer
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetImei_Null_Data_Pointer_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetImei */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetImei );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get imei version callback function happy path
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetImei_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetImei */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular__Cellular_RecvFuncGetImei );
    Cellular_ATRemoveLeadingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that get imei failure case Cellular_CommonGetModemInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetModemInfo_Get_IMEI_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion CELLULAR_PKT_STATUS_BAD_PARAM failure*/
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetImei */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    /* atReqGetModelId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetManufactureId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get model id callback function null context failure
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetModelId_Null_Context_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetModelId */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetModelId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get model id callback function null at command
 * response failure case Cellular_CommonGetModemInfo
 * to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetModelId_Null_AtResp_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetModelId */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetModelId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get model id callback function null data pointer
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetModelId_Null_Data_Pointer_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetModelId */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetModelId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get model id callback function happy path
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetModelId_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetModelId */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetModelId );
    Cellular_ATRemoveLeadingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveTrailingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that get model id failure case Cellular_CommonGetModemInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetModemInfo_Get_ModelId_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion CELLULAR_PKT_STATUS_BAD_PARAM failure*/
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetImei */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetModelId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    /* atReqGetManufactureId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get manufacture id callback function null context failure
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetManufactureId_Null_Context_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetManufactureId */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetManufactureId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get manufacture id callback function null at command
 * response failure case Cellular_CommonGetModemInfo
 * to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetManufactureId_Null_AtResp_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetManufactureId */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetManufactureId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get manufacture id callback function null data pointer
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetManufactureId_Null_Data_Pointer_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetManufactureId */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetManufactureId );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that get manufacture id callback function happy path
 * case Cellular_CommonGetModemInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetModemInfo_Get_Imei_Callback__Cellular_RecvFuncGetManufactureId_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    /* atReqGetManufactureId */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetManufactureId );
    Cellular_ATRemoveLeadingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveTrailingWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that get manufacture id failure case Cellular_CommonGetModemInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetModemInfo_Get_ManufactureId_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion CELLULAR_PKT_STATUS_BAD_PARAM failure*/
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetImei */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetModelId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetManufactureId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetModemInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetModemInfo_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularModemInfo_t modemInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetFirmwareVersion */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetImei */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetModelId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetManufactureId */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetModemInfo( cellularHandle, &modemInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetIPAddress to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetIPAddress_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetIPAddress( NULL, 0, NULL, 0 );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetIPAddress to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetIPAddress_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, NULL, 0 );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonGetIPAddress to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetIPAddress_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );

    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Happy_Path_CGPADDR( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 5;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncIpAddress
 * for Cellular_CommonGetIPAddress to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetIPAddress_Cb_Cellular_RecvFuncIpAddress_Null_Input_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncIpAddress );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetIPAddress to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetIPAddress_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    char pBuffer[ 10 ];

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetIPAddress( cellularHandle, 0, pBuffer, sizeof( pBuffer ) );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonSetPdnConfig to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonSetPdnConfig_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    cellularStatus = Cellular_CommonSetPdnConfig( NULL, 0, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that bad pdn context type case Cellular_CommonSetPdnConfig to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonSetPdnConfig_Bad_Pdn_Context_Type( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPdnConfig_t pdnConfig;

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonSetPdnConfig( cellularHandle, 0, &pdnConfig );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonSetPdnConfig to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonSetPdnConfig_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPdnConfig_t pdnConfig;

    pdnConfig.pdnContextType = CELLULAR_PDN_CONTEXT_IPV4;

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );
    cellularStatus = Cellular_CommonSetPdnConfig( cellularHandle, 0, &pdnConfig );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path ipv6 case Cellular_CommonSetPdnConfig to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonSetPdnConfig_IPV6_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPdnConfig_t pdnConfig;

    pdnConfig.pdnContextType = CELLULAR_PDN_CONTEXT_IPV6;

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonSetPdnConfig( cellularHandle, 0, &pdnConfig );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path ipv4v6 case Cellular_CommonSetPdnConfig to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonSetPdnConfig_IPV4V6_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPdnConfig_t pdnConfig;

    pdnConfig.pdnContextType = CELLULAR_PDN_CONTEXT_IPV4V6;

    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonSetPdnConfig( cellularHandle, 0, &pdnConfig );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetSimCardLockStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetSimCardLockStatus( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetSimCardLockStatus to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetSimCardLockStatus_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that at command failure case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_BAD_PARAMETER );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_BAD_PARAMETER, CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that SimLockState READY case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_READY( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState SIM_PIN case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_SIM_PIN( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 2;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState SIM_PUK case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_SIM_PUK( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 3;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState SIM_PIN2 case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_SIM_PIN2( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 4;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState SIM_PUK2 case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_SIM_PUK2( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 5;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_NET_PIN case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_NET_PIN( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 6;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_NET_PUK case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_NET_PUK( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 7;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_NETSUB_PIN case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_NETSUB_PIN( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 8;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_NETSUB_PUK case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_NETSUB_PUK( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 9;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_SP_PIN case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_SP_PIN( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 10;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_SP_PUK case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_SP_PUK( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 11;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_CORP_PIN case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_CORP_PIN( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 12;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState PH_CORP_PUK case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_PH_CORP_PUK( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 13;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that SimLockState Uknown case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_SimLockState_Unknown( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    simLockStateTestCase = 14;
    Cellular_ATGetNextTok_StubWithCallback( Mock_simLockStateTestCase_ATGetNextTok_Calback );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncGetSimLockStatus
 * for Cellular_CommonGetSimCardLockStatus to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardLockStatus_Cb_Cellular_RecvFuncGetSimLockStatus_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetSimLockStatus );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetSimCardLockStatus to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetSimCardLockStatus_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardStatus_t simCardStatus;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_OK, CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetSimCardLockStatus( cellularHandle, &simCardStatus );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetSimCardInfo to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetSimCardInfo_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetSimCardInfo( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetSimCardInfo to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetSimCardInfo_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonGetSimCardInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetSimCardInfo_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetHplmn case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetIccid case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetIccid
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetIccid_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncGetIccid
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetIccid_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetIccid
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetIccid_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );


    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncGetIccid
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetIccid_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that too long string case in callback function _Cellular_RecvFuncGetIccid
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetIccid_Too_Long_String( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetIccid );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetImsi
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetImsi_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncGetImsi
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetImsi_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetImsi
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetImsi_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that string too long case in callback function _Cellular_RecvFuncGetImsi
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetImsi_String_Too_Long( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );

    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncGetImsi
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetImsi_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetImsi );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_SUCCESS, CELLULAR_PKT_STATUS_OK );

    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_INVALID_HANDLE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that checkCrsmReadStatus error case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_checkCrsmReadStatus_Error( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_IgnoreAndReturn( CELLULAR_AT_SUCCESS );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that checkCrsmReadStatus illegal token case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_checkCrsmReadStatus_illegal_Token_Error( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseHplmn null token case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_parseHplmn_Null_Token( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 4;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseHplmn wrong data length case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_parseHplmn_Wrong_Data_Length( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 5;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseHplmn happy path case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_parseHplmn_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 6;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _parseHplmn happy path case II in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_parseHplmn_Happy_Path_II( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 7;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that CrsmMemoryStatus false memoryStatus case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_CrsmMemoryStatus_False_Memory_Status( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that _checkCrsmMemoryStatus false memoryStatus case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_checkCrsmMemoryStatus_False_Memory_Status( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case II in callback function _Cellular_RecvFuncGetHplmn
 * for Cellular_CommonGetSimCardInfo to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetSimCardInfo_Cb_Cellular_RecvFuncGetHplmn_Happy_Path_Case_II( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetHplmn case. */
    cbCondition = 3;
    recvFuncGetHplmnCase = 0;
    parseHplmn_test = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetHplmn );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_recvFuncGetHplmnCase_ATGetNextTok_Calback );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_PKT_STATUS_FAILURE );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetSimCardInfo to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetSimCardInfo_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularSimCardInfo_t simCardInfo;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    /* atReqGetImsi case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetHplmn case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    /* atReqGetIccid case. */
    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetSimCardInfo( cellularHandle, &simCardInfo );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonSetPsmSettings to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonSetPsmSettings_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonSetPsmSettings( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonSetPsmSettings to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonSetPsmSettings_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonSetPsmSettings( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonSetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonSetPsmSettings_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    psmSettings.periodicRauValue = 0;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );

    cellularStatus = Cellular_CommonSetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonSetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonSetPsmSettings_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonSetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that NULL handler case Cellular_CommonGetPsmSettings to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_CommonGetPsmSettings_Null_Handler( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_INVALID_HANDLE );
    cellularStatus = Cellular_CommonGetPsmSettings( NULL, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_INVALID_HANDLE, cellularStatus );
}

/**
 * @brief Test that bad parameter case Cellular_CommonGetPsmSettings to return CELLULAR_BAD_PARAMETER.
 */
void test_Cellular_CommonGetPsmSettings_Bad_Parameter( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, NULL );
    TEST_ASSERT_EQUAL( CELLULAR_BAD_PARAMETER, cellularStatus );
}

/**
 * @brief Test that at command failure case Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    psmSettings.periodicRauValue = 0;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_TIMED_OUT, CELLULAR_TIMEOUT );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_TIMEOUT, cellularStatus );
}

/**
 * @brief Test that null context case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Null_Context( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;


    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 0;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_FAILURE, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null at command failure case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Null_AtCmd_Failure( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;


    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 1;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that null data pointer case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Null_Data_Pointer( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;


    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 2;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    _Cellular_TranslatePktStatus_ExpectAndReturn( CELLULAR_PKT_STATUS_BAD_PARAM, CELLULAR_INTERNAL_FAILURE );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_INTERNAL_FAILURE, cellularStatus );
}

/**
 * @brief Test that CPSMS_POS_MODE error case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_INTERNAL_FAILURE.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_CPSMS_Error_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;


    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );

    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    cpsms_pos_mode_error = 1;
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 1;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 2 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_2( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 2;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 3 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_3( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 3;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 4 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_4( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 4;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 5 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_5( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 5;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 6 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_6( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 6;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path timer index is 7 case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Happy_Path_Idx_7( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 7;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that timer error return case in callback function _Cellular_RecvFuncGetPsmSettings
 * for Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Cb_Cellular_RecvFuncGetPsmSettings_Timer_Error_Return( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );
    _Cellular_IsValidPdn_IgnoreAndReturn( CELLULAR_SUCCESS );
    cbCondition = 3;
    _Cellular_AtcmdRequestWithCallback_StubWithCallback( Mock_AtcmdRequestWithCallback__Cellular_RecvFuncGetPsmSettings );
    psmSettingsTimerIndex = 2;
    psmSettingsTimerError = 1;
    Cellular_ATRemovePrefix_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( Mock_Cellular_ATGetNextTok_Calback );
    Cellular_ATStrtoi_StubWithCallback( Mock_Cellular_ATStrtoi );

    _Cellular_TranslateAtCoreStatus_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    _Cellular_TranslatePktStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}

/**
 * @brief Test that happy path case Cellular_CommonGetPsmSettings to return CELLULAR_SUCCESS.
 */
void test_Cellular_CommonGetPsmSettings_Happy_Path( void )
{
    CellularError_t cellularStatus = CELLULAR_SUCCESS;
    CellularHandle_t cellularHandle;
    CellularPsmSettings_t psmSettings;

    _Cellular_CheckLibraryStatus_IgnoreAndReturn( CELLULAR_SUCCESS );

    _Cellular_AtcmdRequestWithCallback_IgnoreAndReturn( CELLULAR_PKT_STATUS_OK );

    cellularStatus = Cellular_CommonGetPsmSettings( cellularHandle, &psmSettings );
    TEST_ASSERT_EQUAL( CELLULAR_SUCCESS, cellularStatus );
}
