# Change Log for FreeRTOS Cellular Interface Library

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
