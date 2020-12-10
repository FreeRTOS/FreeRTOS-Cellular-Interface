# FreeRTOS Labs - FreeRTOS Cellular Library

The FreeRTOS Cellular Library exposes the capability of a few popular cellular modems through a uniform API.  Currently, this FreeRTOS Labs project contains libraries for these three cellular modems.  

* Quectel BG96
* Sierra Wireless HL7802
* U-Blox Sara-R4


The current version of the FreeRTOS Cellular Library encapsulates the TCP stack offered by those cellular modems.  They all implement the same uniform [FreeRTOS Cellular Library API](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular.zip).  That API hides the complexity of AT commands, and exposes a socket-like interface to C programmers.  

Even though applications can choose to use the FreeRTOS Cellular Library API directly, the API is not designed for such a purpose. In a typical FreeRTOS system, applications use high level libraries, such as the [coreMQTT](https://github.com/FreeRTOS/coreMQTT) library and the [coreHTTP](https://github.com/FreeRTOS/coreHTTP) library, to communicate with other end points. Those high level libraries use an abstract interface, the [Transport Interface](https://github.com/FreeRTOS/coreMQTT/blob/main/source/interface/transport_interface.h), to send and receive data. A Transport Interface can be implemented on top of the FreeRTOS Cellular Library. The [FreeRTOS Cellular Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project uses such an implementation.

Most cellular modems implement more or less the AT commands defined by the [3GPP TS v27.007](https://portal.3gpp.org/desktopmodules/Specifications/SpecificationDetails.aspx?specificationId=1515) standard. This project provides an implementation of such standard AT commands in a reusable [common component](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/tree/master/common) (see [documentation here](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular_common.zip)). The three cellular modules in this project all take advantage of that common code. The library for each modem only implements the vendor-specific AT commands, then exposes the complete FreeRTOS Cellular Library API.

The common component that implements the 3GPP TS v27.007 standard has been written in compliance to these criteria:

* GNU Complexity scores are not over 8.
* MISRA coding standard.  Any deviations from the MISRA C:2012 guidelines are documented in source code comments marked by “`coverity`”.

# Clone the source code

To clone using HTTPS:

```
git clone https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library.git
```

Using SSH:

```
git clone git@github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library.git
```

# Repository structure

At the root of this repository are these folders:

* include : FreeRTOS Cellular Library API definitions
* common : reusable common code that implements the standard AT commands defined by 3GPP TS v27.007
* modules : vendor-specific code that implements non-3GPP AT commands for each cellular modem
* doc : documentations

# API documentation

The pre-generated doxygen documents can be found in [**“doc/document/cellular.zip"**](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular.zip).  After unzipping it,  the **“celluar/index.html”** file is the entry page.

There is dedicated documentation for the common component that implements the standard AT commands defined by 3GPP TS v27.007.  Find that documentation in [**“doc/document/cellular_common.zip”**](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular_common.zip).

# Configure and build the libraries

The FreeRTOS Cellular Library should be built as part of an application. In order to build a library as part of an application, certain configurations must be provided. The [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project provides [an example](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/bg96/cellular_config.h) of how to configure the build. Also see doxygen-generated file "cellular/cellular_config.html" after unzipping the doc.

Please refer to the README of [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project for more information.

# Integrate FreeRTOS Cellular Library with MCU platforms

> I am a MCU vendor. I want to integrate a cellular modem with my MCU platform.


The FreeRTOS Cellular Library runs on MCUs.  It use an abstracted interface - the [Comm Interface](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/blob/master/include/cellular_comm_interface.h), to communicate with cellular modems.  A Comm Interface must be implemented on the MCU platform.  The most common implementations of the Comm Interface are over UART hardware, but it can be implemented over other physical interfaces such as SPI as well.  Find doxygen documentation of Comm Interface in “cellular/comm_if.html” after unzipping the doc. These are example implementations of the Comm Interface:

* [FreeRTOS windows simulator comm interface](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/comm_if_windows.c)
* [FreeRTOS Common IO UART comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_uart.c)
* [STM32 L475 discovery board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/st/boards/stm32l475_discovery/ports/comm_if/comm_if_st.c)
* [Sierra Sensor Hub board comm interface](https://github.com/aws/amazon-freertos/blob/feature/cellular/vendors/sierra/boards/sensorhub/ports/comm_if/comm_if_sierra.c)

The FreeRTOS Cellular Library uses kernel APIs for task synchronization and memory management.  Please refer to "cellular/cellular_platform.html" after unzipping the doc to see the needed API.

The [Lab-Project-FreeRTOS-Cellular-Demo](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo) project has implemented such kernel APIs by using FreeRTOS primitives.  If the target platform is FreeRTOS, [this implementation](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_platform.c) provides a pre-integrated interface.  If your platform is not FreeRTOS, you need to re-implement the APIs contained in [this file](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Demo/blob/master/source/cellular/cellular_platform.c). 

# Adding support for new cellular modems

> I am a cellular modem vendor. I want to implement a FreeRTOS Cellular Library to support my cellular modem.


In order to add support for a new cellular modem, the developer can reuse the [common component](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/tree/master/common) that implemented 3GPP standard AT commands, then do the following:

1. Implement the cellular modem porting interface defined in [cellular_common_portable.h](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/blob/main/common/include/cellular_common_portable.h).
2. Implement the subset of FreeRTOS Cellular Library APIs that use vendor-specific (non-3GPP) AT commands.  The APIs not marked with an “o” in [cellular_common/cellular_common_APIs.html](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular_common.zip) are what you need to implement.
3. Implement FreeRTOS Cellular Library callback functions that handle vendor-specific (non-3GPP) Unsolicited Result Code (URC). The URC handlers not marked with an “o” in [cellular_common/cellular_common_URC_handlers.html](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular_common.zip) are what you need to implement. 

[Cellular common APIs document](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/raw/main/doc/document/cellular_common.zip)provides detail information required in each steps.
It is recommended that you start by cloning the implementation of one of the existing modems, then make modifications where your modem’s vendor-specific (non-3GPP) AT commands are different.

 Current Example Implementations: 

* [Quectel BG96](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/tree/master/modules/bg96)
* [Sierra Wireless HL7802](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/tree/main/modules/hl7802)
* [U-Blox Sara-R4](https://github.com/FreeRTOS/Lab-Project-FreeRTOS-Cellular-Library/tree/main/modules/sara_r4)

