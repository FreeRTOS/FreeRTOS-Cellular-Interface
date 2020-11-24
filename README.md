# FreeRTOS Labs - Cellular HAL Libraries

The Cellular HAL libraries expose the capability of a few popular cellular modems through a uniform API.   

* Quectel BG96
* Sierra HL7802
* Ublox SARA R4 series

The current version of the Cellular HAL libraries encapsulate the TCP stack offered by those cellular modems. They all implement the same uniform [API](http://ec2-52-36-17-39.us-west-2.compute.amazonaws.com/cellular/index.html). The API hides the AT commands interface, and expose a socket-like interface for C programmers.  

Even though applications can choose to use the Cellular HAL API directly, it is not designed for such a purpose.  In a typical FreeRTOS system, applications use high level libraries, such as the [coreMQTT](https://github.com/FreeRTOS/coreMQTT) library and the [coreHTTP](https://github.com/FreeRTOS/coreHTTP) library, to communicate with other end points.  Those high level libraries use an abstract interface, [Transport Interface](https://github.com/FreeRTOS/coreMQTT/blob/main/source/interface/transport_interface.h), to send and receive data.  A Transport Interface can be implemented on top of the Cellular HAL libraries.  The F[reeRTOS Cellular Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project offers such an implementation.

Most cellular modems offer implementations of the AT commands defined by the 3GPP TS v27.007 standard.  This project offers an implementation of such standard AT commands in a reusable component (see [Cellular Common Library](http://ec2-52-36-17-39.us-west-2.compute.amazonaws.com/cellular_common/index.html)).  The three Cellular HAL libraries in this project all use that common code.  The library for each modem only implements the vendor-specific AT commands, then exposes the complete Cellular HAL API.

This [common component](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-HAL/tree/master/common) that implements the 3GPP TS v27.007 standard has been written in compliance to these standards:

* GNU Complexity scores are not over 8
* MISRA coding standard.  Any deviations from the MISRA C:2012 guidelines are documented in source code comments marked by “`coverity`”.

# Clone this repository

To clone using HTTPS:

```
git clone https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-HAL.git
```

Using SSH:

```
git clone git@github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-HAL.git
```

# Repository structure

The root of this repository contains these folders.

* include : cellular HAL API definitions
* common : reusable common code that implements the AT commands defined by 3GPP TS v27.007
* modules : vendor-specific code that implement non-3GPP AT commands
* doc : documentation of the Cellular HAL Libraries

# API documentation

Please use doxygen to generate documentation.

```
doxygen doc/config/cellular
```

The generated documents are under "doc/output/cellular".

There is dedicated documentation for the common code that implements the AT commands defined by 3GPP TS v27.007.

```
doxygen doc/config/cellular_common
```

The generated documents are under "doc/output/cellular_common".

# Configure and build this library

The Cellular HAL libraries are used as part of an applications.  In order to build the libraries, certain configurations must be provided.  The [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project provides [an example](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_config.h) of how to configure the build.  Also see doxygen-generated file "doc/output/cellular/cellular_hal_config.html". 

Please refer to the [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project for more information.

# Integrate Cellular HAL libraries with MCU platforms

> I am a MCU vendor. I want to integrate a cellular modem with my MCU platform.


The Cellular HAL libraries use an abstracted interface - the [Comm Interface](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-HAL/blob/master/include/cellular_comm_interface.h), to communicate with cellular modems.  A Comm Interface must be implemented on the MCU platform.  The most common implementations of the Comm Interface are over UART hardware.  Please refer to doxygen documentation ( doc/output/cellular/comm_if.html ) for definition of the Comm Interface. The [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project has example implementations:

* [FreeRTOS windows simulator comm interface](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/comm_if_windows.c)
* [Amazon-FreeRTOS common IO UART comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_uart.c)
* [STM32 L475 discovery board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_st.c)
* [Sierra sensorhub board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/sierra/boards/sensorhub/ports/comm_if/comm_if_sierra.c)

The Cellular HAL libraries uses kernel APIs for task synchronization and memory management.  Please reference "doc/output/cellular/cellular_platform.html" for more information.

The [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project has implemented such kernel APIs using FreeRTOS primitives.  If the target platform is FreeRTOS, [this implementation](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_platform.c) can be used as is. 

## Adding support for new cellular modems

> I am a cellular modem vendor. I want to implement a Cellular HAL library to support my cellular modem.


In order to add support for a new cellular modem, the developer need to perform the following:

1. Implement the cellular modem porting interface defined in cellular_common_portable.h.
2. Implement the subset of Cellular HAL API that use vendor-specific (non-3GPP) AT commands.
3. Implement Cellular HAL callback functions that handle vendor-specific (non-3GPP) Unsolicited Result Code (URC).

It is recommended that you start by cloning the implementation of one of existing modems, then make modifications where your modem’s vendor-specific (non-3GPP) AT commands are different.

