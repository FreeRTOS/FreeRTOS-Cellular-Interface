/* The config header is always included first. */
#include "cellular_config.h"
#include "cellular_config_defaults.h"

#include "cellular_platform.h"
#include "cellular_common_internal.h"

/* Implementation of safe malloc which returns NULL if the requested
 * size is 0.  Warning: The behavior of malloc(0) is platform
 * dependent.  It is possible for malloc(0) to return an address
 * without allocating memory.
 */
void * safeMalloc( size_t xWantedSize )
{
    __CPROVER_assert( xWantedSize < CBMC_MAX_OBJECT_SIZE, "mallocCanFail size is too big" );
    return nondet_bool() ? malloc( xWantedSize ) : NULL;
}

void * pvPortMalloc( size_t xWantedSize )
{
    return safeMalloc( xWantedSize );
}

void vPortFree( void * pv )
{
    ( void ) pv;
    free( pv );
}

void allocateSocket( void * pCellularHandle )
{
    CellularContext_t * pContext = ( CellularContext_t * ) pCellularHandle;
    CellularSocketContext_t * pSocketData = NULL;
    uint8_t socketId = 0;

    for( socketId = 0; socketId < CELLULAR_NUM_SOCKET_MAX; socketId++ )
    {
        if( pContext->pSocketData[ socketId ] == NULL )
        {
            pSocketData = ( CellularSocketContext_t * ) safeMalloc( sizeof( CellularSocketContext_t ) );

            if( pSocketData != NULL )
            {
                pSocketData->socketId = socketId;
                pContext->pSocketData[ socketId ] = pSocketData;
            }
        }
    }
}

bool MockxQueueReceive( int32_t * queue,
                        void * data,
                        uint32_t time )
{
    CellularPktStatus_t status;

    *( ( int32_t * ) data ) = status;

    return nondet_bool();
}

uint16_t MockPlatformEventGroup_Create()
{
    uint16_t ret;

    return ret;
}

uint16_t MockPlatformEventGroup_WaitBits()
{
    uint16_t ret;

    return ret;
}
