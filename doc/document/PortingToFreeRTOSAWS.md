# FreeRTOS AWS porting guide

## Introduction

The FreeRTOS Cellular Library exposes the capability of cellular modems with AT command interface. This guide provides a method to integrate FreeRTOS Cellular Library with FreeRTOS AWS. We use windows simulator and BG96 cellular module as an example. FreeRTOS Cellular Library is sub-moduled in vendor’s porting folder. To communicate with cellular modem, an implementation of comm interface is required. There are some comm interface implementations can be referenced to reduced porting effort. An example implementation of secure sockets with FreeRTOS cellular APIs is also provided in this guide. FreeRTOS AWS demo application can make use of secure sockets cellular to connect to AWS IoT Core.


> Windows simulator and FreeRTOS AWS 202012.00 are used in this example. User may need to adapt to different platform or FreeRTOS AWS version.

> This document is provided as an example. User doesn't need to follow this porting method if the have their own implementation.

## Porting steps

1. Create the ports/cellular folder with cellular library and adapting functions
2. Create the ports/comm_if folder with comm interface implementation
3. Edit application board CMakeLists.txt to include cellular library
4. Edit main.c to call cellular library APIs

## Folder structure

Windows simulator is used as an example. Only files or folders related to porting are listed.

```
./vendors/pc/boards/windows
├── aws_demos
│   └── application_code
│       ├── cellular_setup.c
│       └── main.c ( Reference the sample code below )
├── CMakeLists.txt ( Reference the sample code below )
└── ports
    ├── cellular
    │   ├── CMakeLists.txt ( Reference the sample code below )
    │   ├── configs
    │   │   └── cellular_config.h
    │   ├── library ( submodule : FreeRTOS Cellular Library )
    │   ├── platform
    │   │   ├── cellular_platform.c
    │   │   └── cellular_platform.h
    │   └── secure_sockets
    │       └── iot_secure_sockets.c
    └── comm_if
        └── comm_if_windows.c
        
```
Reference implementation of these files
* [cellular_setup.c](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular_setup.c)
* [cellular_config.h](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/main/source/cellular/bg96/cellular_config.h)
* [cellular_platform.c](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_platform.c)
* [cellular_platform.h](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_platform.h)
* [iot_secure_sockets.c](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/blob/main/doc/samples/secure_sockets_cellular/iot_secure_sockets.c)
* [comm_if_windows.c](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/main/source/cellular/comm_if_windows.c)

## AFR module dependency

This section provides the description from the point of view of AFR modules. AFR::cellular represents the cellular library. It has three dependencies, AFR::cellular_module, AFR::cellular::mcu_port and AFR::cellular_platform. The AFR::cellular_module defines the cellular module to use. BG96 is used in this example. BG96 porting makes use of the AFR::cellular_common module, which is the common 3GPP implementation. The AFR::cellular::mcu_port defines the comm interface to send and receive AT commands. It is implemented with UART interface in this example while other communication interfaces are also possible for different platforms. The AFR::cellular_platform defines the platform APIs required by FreeRTOS cellular library. FreeRTOS APIs are used to implement AFR::cellular_platform module. The AFR::secure_sockets::mcu_port defines the secure sockets cellular implementation. Demo application make use of coreMQTT to connect to AWS IoT Core. The underneath transport interface of coreMQTT uses secure sockets cellular to send and receive data.

<p align="center"><img src="https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/blob/main/doc/plantuml/images/PortingToFreeRTOSAWS.png" width="70%"><br>


## Sample codes

### The sample code for “ports/cellular/CMakeLists.txt” to compile cellular library

Four AFR modules are defined in this CMakeLists.txt file.

* AFR::cellular
* AFR::cellular_platform
* AFR::cellular_module
* AFR::cellular_common

```cmake
# FreeRTOS Cellular Library

afr_module()

afr_set_lib_metadata(ID "cellular")
afr_set_lib_metadata(DESCRIPTION "This library implements Cellular interface.")
afr_set_lib_metadata(DISPLAY_NAME "FreeRTOS Cellular Library")
afr_set_lib_metadata(CATEGORY "Connectivity")
afr_set_lib_metadata(VERSION "1.0.1")
afr_set_lib_metadata(IS_VISIBLE "true")

set(inc_dir "${CMAKE_CURRENT_LIST_DIR}/library/include")

afr_module_sources(
    ${AFR_CURRENT_MODULE}
    PRIVATE
        "${inc_dir}/cellular_api.h"
        "${inc_dir}/cellular_types.h"
        "${inc_dir}/cellular_config_defaults.h"
        "${inc_dir}/cellular_comm_interface.h"
)

afr_module_include_dirs(
    ${AFR_CURRENT_MODULE}
    PUBLIC
        "${inc_dir}"
        "${CMAKE_CURRENT_LIST_DIR}/configs"
)

afr_module_dependencies(
    ${AFR_CURRENT_MODULE}
    PRIVATE
        AFR::cellular::mcu_port
        AFR::cellular_module
    PUBLIC
        AFR::cellular_platform
        AFR::platform
)

# ===============================================================================================

afr_module(NAME cellular_platform PUBLIC)

afr_module_sources(
    cellular_platform
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/platform/cellular_platform.c"
)

afr_module_include_dirs(
    cellular_platform
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/platform"
)

# ===============================================================================================

afr_module(NAME cellular_common PRIVATE)

afr_module_sources(
    cellular_common
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_3gpp_api.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_3gpp_urc_handler.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_common.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_common_api.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_pkthandler.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_at_core.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/common/src/cellular_pktio.c"
)

afr_module_include_dirs(
    cellular_common
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/library/common/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/library/common/include/private"
)

afr_module_dependencies(
    cellular_common
    PRIVATE
        AFR::cellular
)

# ===============================================================================================

afr_module(NAME cellular_module PRIVATE)

afr_module_sources(
    cellular_module
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/library/modules/bg96/cellular_bg96.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/modules/bg96/cellular_bg96_api.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/modules/bg96/cellular_bg96_urc_handler.c"
        "${CMAKE_CURRENT_LIST_DIR}/library/modules/bg96/cellular_bg96_wrapper.c"
)

afr_module_dependencies(
    cellular_module
    PRIVATE
        AFR::cellular
        AFR::cellular_common
)
```

### The sample code for board CMakeLists.txt to include cellular library

The “AFR::cellular::mcu_port” module is defined in this sample code.

```cmake
# FreeRTOS Cellular Library comm interface
afr_mcu_port(cellular)
target_sources(
    AFR::cellular::mcu_port
    INTERFACE "${afr_ports_dir}/comm_if/comm_if_windows.c"
)

# FreeRTOS Cellular Library
include("${afr_ports_dir}/cellular/CMakeLists.txt")

....

# Do not add demos or tests if they're turned off.
if(AFR_ENABLE_DEMOS OR AFR_ENABLE_TESTS)
    add_executable(
        ${exe_target}
        "${board_dir}/application_code/main.c"
        "${board_demos_dir}/application_code/aws_demo_logging.c"
        "${board_demos_dir}/application_code/aws_demo_logging.h"
        "${board_demos_dir}/application_code/aws_entropy_hardware_poll.c"
        "${board_demos_dir}/application_code/aws_run-time-stats-utils.c"
        "${board_demos_dir}/application_code/cellular_setup.c" 
        # Add cellular_setup to enable cellular network
    )
    target_include_directories(
        ${exe_target}
        PRIVATE
            "${board_demos_dir}/application_code"
    )
    target_link_libraries(
        ${exe_target}
        PRIVATE
            AFR::freertos_plus_tcp
            AFR::utils
            AFR::dev_mode_key_provisioning
            AFR::cellular # Add AFR::cellular dependency
    )
endif()

```

### The sample code for board CMakeLists.txt to include secure sockets cellular

The “AFR::secure_sockets::mcu_port” is defined in this sample code.

```cmake

# Secure sockets
afr_mcu_port(secure_sockets)
target_sources(
    AFR::secure_sockets::mcu_port
    INTERFACE
        "${afr_ports_dir}/cellular/secure_sockets/iot_secure_sockets.c"
)

target_include_directories(
    AFR::secure_sockets::mcu_port
    INTERFACE
        "${afr_ports_dir}/cellular/configs"
        "${afr_ports_dir}/cellular/library/include"
)

target_link_libraries(
    AFR::secure_sockets::mcu_port
    INTERFACE 
        AFR::platform
        AFR::tls
        AFR::crypto
        AFR::pkcs11
)
```

### The sample code for the main.c file

This is the sample code for windows simulator aws_demos. FreeRTOS Cellular Library will create one receive thread in Cellular_Init function. Developer has to make sure to call setupCellular in context which can create thread.

```c
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
    ... 
    /* If the network has just come up...*/
    if( eNetworkEvent == eNetworkUp )
    {
        /* Create the tasks that use the IP stack if they have not already been
         * created. */
        if( xTasksAlreadyCreated == pdFALSE )
        {
            /* A simple example to demonstrate key and certificate provisioning in
             * microcontroller flash using PKCS#11 interface. This should be replaced
             * by production ready key provisioning mechanism. */
            vDevModeKeyProvisioning( );

            /* Initialize AWS system libraries */
            SYSTEM_Init();
            
            /* Setup cellular connection. */
            setupCellular();

            /* Start the demo tasks. */
            DEMO_RUNNER_RunDemos();

            xTasksAlreadyCreated = pdTRUE;
        }
    ...
    }
```

## 

## Appendix : The examples of comm interface implementation

* [FreeRTOS windows simulator comm interface](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/main/source/cellular/comm_if_windows.c)
* [FreeRTOS Common IO UART comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_uart.c)
* [STM32 L475 discovery board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_uart.c)
* [Sierra Sensor Hub board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/sierra/boards/sensorhub/ports/comm_if/comm_if_sierra.c)

