/*
 * FreeRTOS memory safety proofs with CBMC.
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE   OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/* Standard includes. */
#include <stdint.h>

/* Cellular APIs includes. */
#include "cellular_config_defaults.h"
#include "cellular_types.h"
#include "cellular_common_internal.h"
#include "cellular_common_api.h"


#define ensure_memory_is_valid( px, length )    ( px != NULL ) && __CPROVER_w_ok( ( px ), length )

/* Extern the com interface in comm_if_windows.c */
extern CellularCommInterface_t CellularCommInterface;

/****************************************************************
* The signature of the function under test.
****************************************************************/

CellularError_t Cellular_CommonCreateSocket( CellularHandle_t cellularHandle,
                                             uint8_t pdnContextId,
                                             CellularSocketDomain_t socketDomain,
                                             CellularSocketType_t socketType,
                                             CellularSocketProtocol_t socketProtocol,
                                             CellularSocketHandle_t * pSocketHandle );

/****************************************************************
* The proof of Cellular_CommonCreateSocket
****************************************************************/
void harness()
{
    CellularHandle_t pHandle = NULL;
    uint8_t CellularSocketPdnContextId;
    CellularSocketDomain_t cellularSocketDomain;
    CellularSocketType_t cellularSocketType;
    CellularSocketProtocol_t cellularSocketProtocol;
    CellularSocketHandle_t * pTcpSocket = safeMalloc( sizeof( CellularSocketHandle_t * ) );
    CellularError_t socketStatus = CELLULAR_INVALID_HANDLE;

    /****************************************************************
    * Initialize the member of Cellular_CommonInit.
    ****************************************************************/
    Cellular_CommonInit( nondet_bool() ? NULL : &pHandle, &CellularCommInterface );

    if( ( pHandle != NULL ) && ensure_memory_is_valid( pHandle, sizeof( CellularContext_t ) ) )
    {
        if( ( pTcpSocket == NULL ) ||
            ( ( pTcpSocket != NULL ) && ensure_memory_is_valid( pTcpSocket, sizeof( CellularSocketHandle_t * ) ) ) )
        {
            for( CellularSocketPdnContextId = 0; CellularSocketPdnContextId < CELLULAR_NUM_SOCKET_MAX + 2U; CellularSocketPdnContextId++ )
            {
                /* Create a new TCP socket. */
                socketStatus = Cellular_CommonCreateSocket( pHandle,
                                                            CellularSocketPdnContextId,
                                                            cellularSocketDomain,
                                                            cellularSocketType,
                                                            cellularSocketProtocol,
                                                            pTcpSocket );
            }
        }
    }
}
