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
 * @file cellular_3gpp_urc_handler.c
 * @brief Unit tests for functions in cellular_common_internal.h.
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

#include "mock_cellular_at_core.h"
#include "mock_cellular_common.h"

static int parseRegStatusCase = 0;
static int parseLacTacInRegCase = 0;
static int parseCellIdInRegCase = 0;
static int parseRatInfoInRegCase = 0;
static int parseRejectTypeInRegCase = 0;
static int parseRejectCauseInRegCase = 0;
/* ============================   UNITY FIXTURES ============================ */

/* Called before each test method. */
void setUp()
{
    parseRegStatusCase = 0;
    parseLacTacInRegCase = 0;
    parseCellIdInRegCase = 0;
    parseRatInfoInRegCase = 0;
    parseRejectTypeInRegCase = 0;
    parseRejectCauseInRegCase = 0;
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

void MockPlatformMutex_Destroy( PlatformMutex_t * pMutex )
{
    pMutex->created = false;
}

bool MockPlatformMutex_Create( PlatformMutex_t * pNewMutex,
                               bool recursive )
{
    pNewMutex->created = true;
    return true;
}

void MockPlatformMutex_Unlock( PlatformMutex_t * pMutex )
{
}

void MockPlatformMutex_Lock( PlatformMutex_t * pMutex )
{
}

CellularATError_t MOCK_Cellular_ATStrtoi_CALLBACK( const char * pStr,
                                                   int32_t base,
                                                   int32_t * pResult,
                                                   int cmock_num_calls )
{
    *pResult = atoi( pStr );

    return CELLULAR_AT_SUCCESS;
}

CellularATError_t MOCK_Cellular_ATGetNextTok_CALLBACK( char ** ppString,
                                                       char ** ppTokOutput,
                                                       int cmock_num_calls )
{
    CellularATError_t atCoreStatus = CELLULAR_AT_SUCCESS;
    char pFitNum[] = "1";

    if( *ppTokOutput )
    {
        free( *ppTokOutput );
        *ppTokOutput = NULL;
    }

    if( cmock_num_calls < 7 )
    {
        if( cmock_num_calls == 1 )
        {
            if( parseRegStatusCase == 1 )
            {
                char pTestString[] = "10";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRegStatusCase == 2 )
            {
                char pTestString[] = "5";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRegStatusCase == 3 )
            {
                char pTestString[] = "3";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRegStatusCase == 4 )
            {
                char pTestString[] = "8";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
        else if( cmock_num_calls == 2 )
        {
            if( parseLacTacInRegCase == 1 )
            {
                char pTestString[] = "65536";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
        else if( cmock_num_calls == 3 )
        {
            if( parseCellIdInRegCase == 1 )
            {
                char pTestString[] = "-10";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
        else if( cmock_num_calls == 4 )
        {
            if( parseRatInfoInRegCase == 1 )
            {
                char pTestString[] = "10";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRatInfoInRegCase == 2 )
            {
                char pTestString[] = "7";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRatInfoInRegCase == 3 )
            {
                char pTestString[] = "1";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                char pTestString[] = "0";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
        }
        else if( cmock_num_calls == 5 )
        {
            if( parseRejectTypeInRegCase == 1 )
            {
                char pTestString[] = "256";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRejectTypeInRegCase == 2 )
            {
                char pTestString[] = "3";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
        else if( cmock_num_calls == 6 )
        {
            if( parseRejectCauseInRegCase == 1 )
            {
                char pTestString[] = "256";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else if( parseRejectCauseInRegCase == 2 )
            {
                char pTestString[] = "3";
                *ppTokOutput = malloc( sizeof( pTestString ) );
                memcpy( *ppTokOutput, pTestString, sizeof( pTestString ) );
            }
            else
            {
                *ppTokOutput = malloc( sizeof( pFitNum ) );
                memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
            }
        }
        else
        {
            *ppTokOutput = malloc( sizeof( pFitNum ) );
            memcpy( *ppTokOutput, pFitNum, sizeof( pFitNum ) );
        }
    }
    else
    {
        atCoreStatus = CELLULAR_AT_ERROR;
    }

    return atCoreStatus;
}

CellularPktStatus_t MOCK__Cellular_TranslateAtCoreStatus_CALLBACK( CellularATError_t status,
                                                                   int cmock_num_calls )
{
    CellularPktStatus_t pktStatus;

    switch( status )
    {
        case CELLULAR_AT_SUCCESS:
            pktStatus = CELLULAR_PKT_STATUS_OK;
            break;

        case CELLULAR_AT_BAD_PARAMETER:
            pktStatus = CELLULAR_PKT_STATUS_BAD_PARAM;
            break;

        case CELLULAR_AT_NO_MEMORY:
        case CELLULAR_AT_UNSUPPORTED:
        case CELLULAR_AT_MODEM_ERROR:
        case CELLULAR_AT_ERROR:
        case CELLULAR_AT_UNKNOWN:
        default:
            CellularLogError( "_Cellular_TranslateAtCoreStatus: Status %d", status );
            pktStatus = CELLULAR_PKT_STATUS_FAILURE;
            break;
    }

    return pktStatus;
}

void cellularUrcNetworkRegistrationCallback( CellularUrcEvent_t urcEvent,
                                             const CellularServiceStatus_t * pServiceStatus,
                                             void * pCallbackContext )
{
}

/* ========================================================================== */

/**
 * @brief Test that any NULL handler case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_Null_Handler( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;

    packetStatus = _Cellular_ParseRegStatus( NULL, NULL, false, 0 );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_FAILURE, packetStatus );
}

/**
 * @brief Test that any NULL RegPayload case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_Null_RegPayload( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    packetStatus = _Cellular_ParseRegStatus( &context, NULL, false, 0 );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_BAD_PARAM, packetStatus );
}

/**
 * @brief Test that reject type/case for CELLULAR_NETWORK_REGISTRATION_STATUS_REGISTRATION_DENIED to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_POS_REJ_TYPE_Cause_Registration_Denied( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    context.libAtData.csRegStatus = CELLULAR_NETWORK_REGISTRATION_STATUS_REGISTRATION_DENIED;
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRegStatusCase = 3;
    parseRatInfoInRegCase = 3;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CREG: 2", false, CELLULAR_REG_TYPE_CREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_POS_RAT invalid value case to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_POS_RAT_Invalid_Value( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRatInfoInRegCase = 3;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CREG: 2", false, CELLULAR_REG_TYPE_CREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that register out of range case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_Regs_OutOfRange( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRegStatusCase = 1;
    parseLacTacInRegCase = 1;
    parseCellIdInRegCase = 1;
    parseRatInfoInRegCase = 1;
    parseRejectTypeInRegCase = 1;
    parseRejectCauseInRegCase = 1;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_FAILURE, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_POS_RAT CELLULAR_RAT_LTE case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_POS_RAT_Status_CELLULAR_RAT_LTE( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRatInfoInRegCase = 2;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_CEREG clear atlib data case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_CEREG_Status_Clear_AtLib_Data( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRegStatusCase = 4;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_CEREG status registration denied case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_CEREG_Status_Registration_Denied( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRegStatusCase = 3;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_CEREG status registration roaming case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_CEREG_Status_Registration_Roaming( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );
    parseRegStatusCase = 2;
    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_CEREG happy path case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_CEREG_Happy_Path( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    context.cbEvents.networkRegistrationCallback = cellularUrcNetworkRegistrationCallback;
    _Cellular_NetworkRegistrationCallback_Ignore();
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );

    packetStatus = _Cellular_ParseRegStatus( &context, "+CEREG: 2", false, CELLULAR_REG_TYPE_CEREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_CREG happy path case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_CREG_Happy_Path( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    context.cbEvents.networkRegistrationCallback = cellularUrcNetworkRegistrationCallback;
    _Cellular_NetworkRegistrationCallback_Ignore();
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );

    packetStatus = _Cellular_ParseRegStatus( &context, "+CREG: 2", false, CELLULAR_REG_TYPE_CREG );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that CELLULAR_REG_TYPE_UNKNOWN happy path case _Cellular_ParseRegStatus to return CELLULAR_INVALID_HANDLE.
 */
void test_Cellular_ParseRegStatus_CELLULAR_REG_TYPE_UNKNOWN( void )
{
    CellularPktStatus_t packetStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATRemoveAllWhiteSpaces_IgnoreAndReturn( CELLULAR_AT_SUCCESS );
    Cellular_ATGetNextTok_StubWithCallback( MOCK_Cellular_ATGetNextTok_CALLBACK );
    Cellular_ATStrtoi_StubWithCallback( MOCK_Cellular_ATStrtoi_CALLBACK );
    _Cellular_TranslateAtCoreStatus_StubWithCallback( MOCK__Cellular_TranslateAtCoreStatus_CALLBACK );

    packetStatus = _Cellular_ParseRegStatus( &context, "123456789", false, CELLULAR_REG_TYPE_UNKNOWN );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, packetStatus );
}

/**
 * @brief Test that happy path case case Cellular_CommonUrcProcessCreg to return CELLULAR_PKT_STATUS_OK.
 */
void test_Cellular_CommonUrcProcessCreg_AtCmd_Failure_Path( void )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    _Cellular_NetworkRegistrationCallback_Ignore();
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_ERROR );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );

    pktStatus = Cellular_CommonUrcProcessCreg( &context, "+QIURC: \"pdpdeact\",1" );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_FAILURE, pktStatus );
}

/**
 * @brief Test that happy path case case Cellular_CommonUrcProcessCgreg to return CELLULAR_PKT_STATUS_OK.
 */
void test_Cellular_CommonUrcProcessCgreg_AtCmd_Failure_Path( void )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    _Cellular_NetworkRegistrationCallback_Ignore();
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_ERROR );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );
    pktStatus = Cellular_CommonUrcProcessCgreg( &context, "+QIURC: \"pdpdeact\",1" );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_FAILURE, pktStatus );
}

/**
 * @brief Test that happy path case case Cellular_CommonUrcProcessCereg to return CELLULAR_PKT_STATUS_OK.
 */
void test_Cellular_CommonUrcProcessCereg_AtCmd_Failure_Path( void )
{
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularContext_t context;

    _Cellular_NetworkRegistrationCallback_Ignore();
    Cellular_ATRemoveAllDoubleQuote_IgnoreAndReturn( CELLULAR_AT_ERROR );
    _Cellular_TranslateAtCoreStatus_ExpectAndReturn( CELLULAR_AT_ERROR, CELLULAR_PKT_STATUS_FAILURE );

    pktStatus = Cellular_CommonUrcProcessCereg( &context, "+QIURC: \"pdpdeact\",1" );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_FAILURE, pktStatus );
}
