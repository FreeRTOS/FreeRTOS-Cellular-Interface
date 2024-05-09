/*
 * FreeRTOS-Cellular-Interface v1.3.0
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
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
 * The purpose of this file is to compile cellular interface source file without
 * platform dependency.
 */

#ifndef __CELLULAR_PLATFORM_H__
#define __CELLULAR_PLATFORM_H__

#include <stdint.h>
#include <stdbool.h>

#ifndef CELLULAR_DO_NOT_USE_CUSTOM_CONFIG
    /* Include custom config file before other headers. */
    #include "cellular_config.h"
#endif
#include "cellular_config_defaults.h"

/*-----------------------------------------------------------*/

#define platformTRUE     1
#define platformFALSE    0
#define platformPASS     1
#define platformFAIL     0

#define platformMS_TO_TICKS( x )    ( x )

#define platformMAX_DELAY                     ( 0xFFFFFFFF )

#define PLATFORM_THREAD_DEFAULT_STACK_SIZE    ( 2048U )
#define PLATFORM_THREAD_DEFAULT_PRIORITY      ( 5U )

/*-----------------------------------------------------------*/

struct PlatformEventGroupDefinition;
typedef struct PlatformEventGroupDefinition   * PlatformEventGroupHandle_t;

typedef int32_t                               PlatformBaseType_t;
typedef uint32_t                              PlatformUBaseType_t;

typedef uint16_t                              PlatformEventBits_t;

typedef uint32_t                              PlatformTickType_t;

struct PlatformQueueDefinition;
typedef struct PlatformQueueDefinition        * PlatformQueueHandle_t;

typedef struct PlatformMutex
{
} PlatformMutex_t;

/*-----------------------------------------------------------*/

/* Delay functions. */
void Platform_Delay( const PlatformTickType_t xTicksToDelay );

/* Event group functions. */
void PlatformEventGroup_Delete( PlatformEventGroupHandle_t xEventGroup );
PlatformEventBits_t PlatformEventGroup_ClearBits( PlatformEventGroupHandle_t xEventGroup,
                                                  const PlatformEventBits_t uxBitsToClear );
PlatformEventGroupHandle_t PlatformEventGroup_Create( void );
PlatformEventBits_t PlatformEventGroup_GetBits( PlatformEventGroupHandle_t xEventGroup );
PlatformEventBits_t PlatformEventGroup_SetBits( PlatformEventGroupHandle_t xEventGroup,
                                                const PlatformEventBits_t uxBitsToSet );
PlatformBaseType_t PlatformEventGroup_SetBitsFromISR( PlatformEventGroupHandle_t xEventGroup,
                                                      const PlatformEventBits_t uxBitsToSet,
                                                      PlatformBaseType_t * pxHigherPriorityTaskWoken );
PlatformBaseType_t PlatformEventGroup_WaitBits( const PlatformEventGroupHandle_t xEventGroup,
                                                const PlatformEventBits_t uxBitsToWaitFor,
                                                const PlatformBaseType_t xClearOnExit,
                                                const PlatformBaseType_t xWaitForAllBits,
                                                PlatformTickType_t xTicksToWait );

/* Queue functions. */
void PlatformQueue_Delete( PlatformQueueHandle_t xQueue );
PlatformBaseType_t PlatformQueue_Send( PlatformQueueHandle_t xQueue,
                                       const void * pvItemToQueue,
                                       PlatformTickType_t xTicksToWait );
PlatformBaseType_t PlatformQueue_Receive( PlatformQueueHandle_t xQueue,
                                          void * pvBuffer,
                                          PlatformTickType_t xTicksToWait );
PlatformQueueHandle_t PlatformQueue_Create( PlatformUBaseType_t uxQueueLength,
                                            PlatformUBaseType_t uxItemSize );

/* Mutex functions. */
bool PlatformMutex_Create( PlatformMutex_t * pNewMutex,
                           bool recursive );
void PlatformMutex_Destroy( PlatformMutex_t * pMutex );
void PlatformMutex_Lock( PlatformMutex_t * pMutex );
bool PlatformMutex_TryLock( PlatformMutex_t * pMutex );
void PlatformMutex_Unlock( PlatformMutex_t * pMutex );

/* Memory allocate functions. */
void * Platform_Malloc( size_t xMallocSize );
void Platform_Free( void * vPointer );

#endif /* __CELLULAR_PLATFORM_H__ */
