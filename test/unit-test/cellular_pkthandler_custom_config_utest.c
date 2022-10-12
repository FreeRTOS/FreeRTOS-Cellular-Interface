/*
 * FreeRTOS-Cellular-Interface v1.3.0
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
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 */

/**
 * @file cellular_pkthandler_custom_config_utest.c
 * @brief Unit tests for functions in cellular_pkthandler_internal.h.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "unity.h"

#include "cellular_config.h"
#include "cellular_config_defaults.h"

/* Include paths for public enums, structures, and macros. */
#include "cellular_platform.h"
#include "cellular_common_internal.h"
#include "cellular_pkthandler_internal.h"
#include "cellular_types.h"

#include "mock_cellular_at_core.h"
#include "mock_cellular_common.h"
#include "mock_cellular_pktio_internal.h"

/*-----------------------------------------------------------*/

#define TEST_CUSTOM_LEADING_CHAR_URC_PREFIX     "TESTURC"
#define TEST_CUSTOM_LEADING_CHAR_URC_PAYLOAD    "testPayload"
#define TEST_CUSTOM_LEADING_CHAR_URC_STRING     "^"TEST_CUSTOM_LEADING_CHAR_URC_PREFIX ":"TEST_CUSTOM_LEADING_CHAR_URC_PAYLOAD

/*-----------------------------------------------------------*/

static bool customLeadingCharTestResult;

/*-----------------------------------------------------------*/

static void prvCustomLeadingCharURCHandler( CellularContext_t * pContext,
                                            char * pInputStr )
{
    TEST_ASSERT( pContext != NULL );
    TEST_ASSERT( pInputStr != NULL );
    TEST_ASSERT_EQUAL_STRING( TEST_CUSTOM_LEADING_CHAR_URC_PAYLOAD, pInputStr );
    customLeadingCharTestResult = true;
}

/*-----------------------------------------------------------*/

void * mock_malloc( size_t size )
{
    return ( void * ) malloc( size );
}

/*-----------------------------------------------------------*/

void dummyDelay( uint32_t milliseconds )
{
    ( void ) milliseconds;
}

/*-----------------------------------------------------------*/

bool MockPlatformMutex_Create( PlatformMutex_t * pNewMutex,
                               bool recursive )
{
    ( void ) recursive;
    return true;
}

/*-----------------------------------------------------------*/

void MockPlatformMutex_Lock( PlatformMutex_t * pMutex )
{
    ( void ) pMutex;
}

/*-----------------------------------------------------------*/

void MockPlatformMutex_Unlock( PlatformMutex_t * pMutex )
{
    ( void ) pMutex;
}

/*-----------------------------------------------------------*/

QueueHandle_t MockxQueueCreate( int32_t uxQueueLength,
                                uint32_t uxItemSize )
{
    ( void ) uxQueueLength;
    ( void ) uxItemSize;
}

/*-----------------------------------------------------------*/

BaseType_t MockxQueueSend( QueueHandle_t queue,
                           void * data,
                           uint32_t time )
{
    ( void ) queue;
    ( void ) data;
    ( void ) time;
}

/*-----------------------------------------------------------*/

BaseType_t MockxQueueReceive( QueueHandle_t queue,
                              void * data,
                              uint32_t time )
{
    ( void ) queue;
    ( void ) data;
    ( void ) time;
}

/*-----------------------------------------------------------*/

void MockPlatformMutex_Destroy( PlatformMutex_t * pMutex )
{
    ( void ) pMutex;
}

/*-----------------------------------------------------------*/

uint16_t MockvQueueDelete( QueueHandle_t queue )
{
    ( void ) queue;
}

/*-----------------------------------------------------------*/

CellularATError_t _CMOCK_Cellular_ATStrDup_CALLBACK( char ** ppDst,
                                                     const char * pSrc,
                                                     int cmock_num_calls )
{
    char * p = NULL;
    CellularATError_t atStatus = CELLULAR_AT_SUCCESS;
    const char * pTempSrc = pSrc;

    ( void ) cmock_num_calls;

    if( ( ppDst == NULL ) || ( pTempSrc == NULL ) || ( strnlen( pTempSrc, CELLULAR_AT_MAX_STRING_SIZE ) >= CELLULAR_AT_MAX_STRING_SIZE ) )
    {
        atStatus = CELLULAR_AT_BAD_PARAMETER;
    }

    if( atStatus == CELLULAR_AT_SUCCESS )
    {
        *ppDst = ( char * ) Platform_Malloc( sizeof( char ) * ( strlen( pTempSrc ) + 1U ) );

        if( *ppDst != NULL )
        {
            p = *ppDst;

            while( *pTempSrc != '\0' )
            {
                *p = *pTempSrc;
                p++;
                pTempSrc++;
            }

            *p = '\0';
        }
    }

    return atStatus;
}

/*-----------------------------------------------------------*/

/**
 * @brief Test CELLULAR_CHECK_IS_URC_LEADING_CHAR.
 *
 * Cellular interface allows modem porting to override URC leadding char with
 * CELLULAR_CHECK_IS_URC_LEADING_CHAR macros. Input line with custom prefix leading
 * char, for example "^TESTURC:payload", should be handled without problem.
 */
void test__Cellular_HandlePacket_AT_UNSOLICITED_custom_leading_char_happy_paty( void )
{
    CellularContext_t context;
    CellularPktStatus_t pktStatus = CELLULAR_PKT_STATUS_OK;
    CellularAtParseTokenMap_t cellularUrcHandlerTable[] =
    {
        { TEST_CUSTOM_LEADING_CHAR_URC_PREFIX, prvCustomLeadingCharURCHandler }
    };

    /* Clear cellular context. */
    memset( &context, 0, sizeof( CellularContext_t ) );

    /* Add a custom leading char URC callback. */
    context.tokenTable.pCellularUrcHandlerTable = cellularUrcHandlerTable;
    context.tokenTable.cellularPrefixToParserMapSize = 1;

    Cellular_ATStrDup_StubWithCallback( _CMOCK_Cellular_ATStrDup_CALLBACK );

    /* Reset the callback result. */
    customLeadingCharTestResult = false;

    /* Input with custom URC leading char should be handled without problem. */
    pktStatus = _Cellular_HandlePacket( &context, AT_UNSOLICITED, TEST_CUSTOM_LEADING_CHAR_URC_STRING );
    TEST_ASSERT_EQUAL( CELLULAR_PKT_STATUS_OK, pktStatus );

    /* The callback function should be called. The input line will be checked in
     * callback function. */
    TEST_ASSERT_EQUAL( customLeadingCharTestResult, true );
}

/*-----------------------------------------------------------*/
