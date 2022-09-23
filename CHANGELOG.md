# Change Log for FreeRTOS Cellular Interface Library

## v1.3.0 (October 2022)
### Updates
 - [#102](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/102) MISRA C:2012 Compliance update.
 - [#101](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/101) Add Cellular common AT command timeout.
 - [#99](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/99) CBMC Starter kit update.
 - [#98](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/98) HL7802 Finish processing packets after remote disconnect.
 - [#96](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/96) Add config to disable hardware flow control.
 - [#94](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/94) Cellular Socket State design update.
 - [#92](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/92) Socket CONNECTING state change.
 - [#90](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/90) Handle network registration URC in AT Command response callback.
 - [#88](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/88) Set TCP Local Port.
 - [#86](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/86) Restore URC String when using generic URC callback.
 - [#85](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/85) SARA-R4 socket receive response handling.
 - [#84](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/84) HL7802 Signal processing function update.
 - [#82](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/82) Add default RAT selection list.
 - [#80](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/80) Hl7802 Delay after reboot.
 - [#79](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/79) SARA-R4 Get PDN Status.
 - [#77](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/77) HL7802 AT command timeout update.
 - [#76](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/76) HL7802 band config detection and changes to log messages.
 - [#75](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/75) SARA-R4 Reboot on init and changes to log messages.

## v1.2.0 (December 2021)
### Updates
 - [#62](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/62) Fix uintptr compile error.
 - [#67](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/67) Fix too many arguments log.
 - [#64](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/64) Remove strnlen usage.

### Other
 - [#63](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/63) Fix broken links.
 - [#61](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/61) Fix typo in README.md.
 - [#69](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/69) Loop invariant update.
 - [#65](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/65) Submodule community ports repo to third party folder.
 - [#70](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/70) fix CBMC potential issues.

## v1.1.0 (November 2021)
### Updates
 - [#40](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/40) Fix the module logging mechanism format.
 - [#53](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/53) Use the stdint.h header file for integer types.
 - [#55](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/55) Add function for validating the prefix character of cellular module response.
 - [#57](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/57) Fix unknown type issue.

### Other
 - [#52](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/52) Resolve ublox sara r4 build warnings.
 - [#42](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/42) Fix spelling check failure in CI tools.
 - [#43](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/43) [#46](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/46) [#54](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/54) Minor documentation updates.
 - [#48](https://github.com/FreeRTOS/FreeRTOS-Cellular-Interface/pull/48) Header Guards for C++ linkage.

## v1.0.0 (September 2021)

This is the first release of the FreeRTOS Cellular Interface library in this repository.

The FreeRTOS Cellular Interface library provides an implementation of the standard AT commands for the [3GPP TS v27.007](https://portal.3gpp.org/desktopmodules/Specifications/SpecificationDetails.aspx?specificationId=1515) and support for the 3 cellular modems [Quectel BG96](https://www.quectel.com/product/bg96.htm), [Sierra Wireless HL7802](https://www.sierrawireless.com/products-and-solutions/embedded-solutions/products/hl7802/) and [U-Blox Sara-R4](https://www.u-blox.com/en/product/sara-r4-series).
