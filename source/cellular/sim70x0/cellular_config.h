/*
 * Amazon FreeRTOS Cellular Preview Release
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

/**
 * @file cellular_config.h
 * @brief cellular config options.
 */

#ifndef __CELLULAR_CONFIG_H__
#define __CELLULAR_CONFIG_H__

/* This is a project specific file and is used to override config values defined
 * in cellular_config_defaults.h. */

/**
 * Cellular comm interface make use of COM port on computer to communicate with
 * cellular module on windows simulator, for example "COM5".
 * #define CELLULAR_COMM_INTERFACE_PORT    "...insert here..."
 */
#define	CELLULAR_COMM_INTERFACE_PORT	"COM39"

/*
 * Default APN for network registartion.
 * #define CELLULAR_APN                    "...insert here..."
 */
/*
#define	CELLULAR_PDN_TYPE				CELLULAR_PDN_CONTEXT_IPV4
#define	CELLULAR_APN					"mopera.net"
#define	CELLULAR_AUTH					(CELLULAR_PDN_AUTH_NONE)
#define	CELLULAR_UID					""
#define	CELLULAR_PWD					""
*/
#define	CELLULAR_PDN_TYPE				CELLULAR_PDN_CONTEXT_IPV4
#define	CELLULAR_APN					"iijmobile.biz"
#define	CELLULAR_AUTH					(CELLULAR_PDN_AUTH_PAP)
#define	CELLULAR_UID					"mobile@iij"
#define	CELLULAR_PWD					"iij"

/*
 * PDN context id for cellular network.
 */
//#define	CELLULAR_PDN_CONTEXT_ID_MAX	16
//#define	CELLULAR_NUM_SOCKET_MAX		12

#define	CELLULAR_PDN_CONTEXT_ID_MIN		 0
#define	CELLULAR_PDN_CONTEXT_ID_MAX		 4
#define CELLULAR_PDN_CONTEXT_ID			(1)								/* PDP Prolfile ID=1-16				*/
#define CELLULAR_PDP_INDEX				(0)								/* PDP for SIM70x0 inside use: 0-4	*/
#define	CELLULAR_PDP_INDEX_MAX			 4

#define	CELLULAR_PDN_CONTEXT_TYPE		CELLULAR_PDN_CONTEXT_IPV4
 /*
 * PDN connect timeout for network registration.
 */
#define CELLULAR_PDN_CONNECT_TIMEOUT    ( 100000UL )

#define	CELLULAR_MAX_SEND_DATA_LEN			1459
#define	CELLULAR_MAX_RECV_DATA_LEN			1459

 /*
 * Overwrite default config for different cellular modules.
 */
/*
 * GetHostByName API is not used in the demo. IP address is used to store the hostname.
 * The value should be longer than the length of democonfigMQTT_BROKER_ENDPOINT in demo_config.h.
 */
#define CELLULAR_IP_ADDRESS_MAX_SIZE                    ( 64U )

#endif /* __CELLULAR_CONFIG_H__ */
