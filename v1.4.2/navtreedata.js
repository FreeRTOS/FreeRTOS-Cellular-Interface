/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "FreeRTOS: FreeRTOS Cellular Library", "index.html", [
    [ "Overview", "index.html", "index" ],
    [ "Design", "_design.html", [
      [ "Cellular Interface Design", "_design.html#FreeRTOS_Cellular_Library_Design", null ],
      [ "Cellular Socket API Design", "_design.html#cellular_socket_api_desing", null ]
    ] ],
    [ "Configurations", "cellular_config.html", [
      [ "CELLULAR_DO_NOT_USE_CUSTOM_CONFIG", "cellular_config.html#CELLULAR_DO_NOT_USE_CUSTOM_CONFIG", null ],
      [ "CELLULAR_MCC_MAX_SIZE", "cellular_config.html#CELLULAR_MCC_MAX_SIZE", null ],
      [ "CELLULAR_MNC_MAX_SIZE", "cellular_config.html#CELLULAR_MNC_MAX_SIZE", null ],
      [ "CELLULAR_ICCID_MAX_SIZE", "cellular_config.html#CELLULAR_ICCID_MAX_SIZE", null ],
      [ "CELLULAR_IMSI_MAX_SIZE", "cellular_config.html#CELLULAR_IMSI_MAX_SIZE", null ],
      [ "CELLULAR_FW_VERSION_MAX_SIZE", "cellular_config.html#CELLULAR_FW_VERSION_MAX_SIZE", null ],
      [ "CELLULAR_HW_VERSION_MAX_SIZE", "cellular_config.html#CELLULAR_HW_VERSION_MAX_SIZE", null ],
      [ "CELLULAR_SERIAL_NUM_MAX_SIZE", "cellular_config.html#CELLULAR_SERIAL_NUM_MAX_SIZE", null ],
      [ "CELLULAR_IMEI_MAX_SIZE", "cellular_config.html#CELLULAR_IMEI_MAX_SIZE", null ],
      [ "CELLULAR_NETWORK_NAME_MAX_SIZE", "cellular_config.html#CELLULAR_NETWORK_NAME_MAX_SIZE", null ],
      [ "CELLULAR_APN_MAX_SIZE", "cellular_config.html#CELLULAR_APN_MAX_SIZE", null ],
      [ "CELLULAR_PDN_USERNAME_MAX_SIZE", "cellular_config.html#CELLULAR_PDN_USERNAME_MAX_SIZE", null ],
      [ "CELLULAR_PDN_PASSWORD_MAX_SIZE", "cellular_config.html#CELLULAR_PDN_PASSWORD_MAX_SIZE", null ],
      [ "CELLULAR_IP_ADDRESS_MAX_SIZE", "cellular_config.html#CELLULAR_IP_ADDRESS_MAX_SIZE", null ],
      [ "CELLULAR_AT_CMD_MAX_SIZE", "cellular_config.html#CELLULAR_AT_CMD_MAX_SIZE", null ],
      [ "CELLULAR_NUM_SOCKET_MAX", "cellular_config.html#CELLULAR_NUM_SOCKET_MAX", null ],
      [ "CELLULAR_MANUFACTURE_ID_MAX_SIZE", "cellular_config.html#CELLULAR_MANUFACTURE_ID_MAX_SIZE", null ],
      [ "CELLULAR_MODEL_ID_MAX_SIZE", "cellular_config.html#CELLULAR_MODEL_ID_MAX_SIZE", null ],
      [ "CELLULAR_EDRX_LIST_MAX_SIZE", "cellular_config.html#CELLULAR_EDRX_LIST_MAX_SIZE", null ],
      [ "CELLULAR_PDN_CONTEXT_ID_MIN", "cellular_config.html#CELLULAR_PDN_CONTEXT_ID_MIN", null ],
      [ "CELLULAR_PDN_CONTEXT_ID_MAX", "cellular_config.html#CELLULAR_PDN_CONTEXT_ID_MAX", null ],
      [ "CELLULAR_MAX_RAT_PRIORITY_COUNT", "cellular_config.html#CELLULAR_MAX_RAT_PRIORITY_COUNT", null ],
      [ "CELLULAR_MAX_SEND_DATA_LEN", "cellular_config.html#CELLULAR_MAX_SEND_DATA_LEN", null ],
      [ "CELLULAR_MAX_RECV_DATA_LEN", "cellular_config.html#CELLULAR_MAX_RECV_DATA_LEN", null ],
      [ "CELLULAR_SUPPORT_GETHOSTBYNAME", "cellular_config.html#CELLULAR_SUPPORT_GETHOSTBYNAME", null ],
      [ "CELLULAR_COMM_IF_SEND_TIMEOUT_MS", "cellular_config.html#CELLULAR_COMM_IF_SEND_TIMEOUT_MS", null ],
      [ "CELLULAR_COMM_IF_RECV_TIMEOUT_MS", "cellular_config.html#CELLULAR_COMM_IF_RECV_TIMEOUT_MS", null ],
      [ "CELLULAR_CONFIG_STATIC_ALLOCATION_CONTEXT", "cellular_config.html#CELLULAR_CONFIG_STATIC_ALLOCATION_CONTEXT", null ],
      [ "CELLULAR_CONFIG_STATIC_COMM_CONTEXT_ALLOCATION", "cellular_config.html#CELLULAR_CONFIG_STATIC_COMM_CONTEXT_ALLOCATION", null ],
      [ "CELLULAR_CONFIG_DEFAULT_RAT", "cellular_config.html#CELLULAR_CONFIG_DEFAULT_RAT", null ],
      [ "CELLULAR_CONFIG_STATIC_SOCKET_CONTEXT_ALLOCATION", "cellular_config.html#CELLULAR_CONFIG_STATIC_SOCKET_CONTEXT_ALLOCATION", null ],
      [ "CELLULAR_COMMON_AT_COMMAND_TIMEOUT_MS", "cellular_config.html#CELLULAR_COMMON_AT_COMMAND_TIMEOUT_MS", null ],
      [ "CELLULAR_AT_COMMAND_RAW_TIMEOUT_MS", "cellular_config.html#CELLULAR_AT_COMMAND_RAW_TIMEOUT_MS", null ],
      [ "CELLULAR_CONFIG_MAX_PREFIX_STRING_LENGTH", "cellular_config.html#CELLULAR_CONFIG_MAX_PREFIX_STRING_LENGTH", null ],
      [ "CELLULAR_AT_MAX_STRING_SIZE", "cellular_config.html#CELLULAR_AT_MAX_STRING_SIZE", null ],
      [ "CELLULAR_CONFIG_USE_CCID_COMMAND", "cellular_config.html#CELLULAR_CONFIG_USE_CCID_COMMAND", null ],
      [ "CELLULAR_CONFIG_ASSERT", "cellular_config.html#CELLULAR_CONFIG_ASSERT", null ],
      [ "CELLULAR_CONFIG_PLATFORM_FREERTOS", "cellular_config.html#CELLULAR_CONFIG_PLATFORM_FREERTOS", null ],
      [ "CELLULAR_CONFIG_PKTIO_READ_BUFFER_SIZE", "cellular_config.html#CELLULAR_CONFIG_PKTIO_READ_BUFFER_SIZE", null ],
      [ "CELLULAR_MODEM_NO_EPS_NETWORK", "cellular_config.html#CELLULAR_MODEM_NO_EPS_NETWORK", null ],
      [ "LogError", "cellular_config.html#cellular_logerror", null ],
      [ "LogWarn", "cellular_config.html#cellular_logwarn", null ],
      [ "LogInfo", "cellular_config.html#cellular_loginfo", null ],
      [ "LogDebug", "cellular_config.html#cellular_logdebug", null ]
    ] ],
    [ "Functions", "cellular_functions.html", null ],
    [ "Porting Guide", "cellular_porting.html", [
      [ "Configuration Macros", "cellular_porting.html#cellular_porting_config", null ],
      [ "Cellular Communication Interface", "cellular_porting.html#cellular_porting_comm_if", null ],
      [ "Cellular platform dependency", "cellular_porting.html#cellular_platform_dependency", null ]
    ] ],
    [ "Porting Module Guide", "cellular_porting_module_guide.html", "cellular_porting_module_guide" ],
    [ "Data types and Constants", "modules.html", "modules" ]
  ] ]
];

var NAVTREEINDEX =
[
"_design.html",
"group__cellular__datatypes__paramstructs.html#ga9051ee90744120af61c4ea839dc01ea3"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';