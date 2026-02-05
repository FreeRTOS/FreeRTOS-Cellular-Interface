# This file is to add source files and include directories
# into variables so that it can be reused from different repositories
# in their Cmake based build system by including this file.
#
# Files specific to the repository such as test runner, platform tests
# are not added to the variables.

# Cellular library source files.
set( CELLULAR_COMMON_SOURCES
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_at_core.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_common.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_common_api.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_3gpp_urc_handler.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_3gpp_api.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_pkthandler.c
     ${CMAKE_CURRENT_LIST_DIR}/source/cellular_pktio.c )

# Cellular library include directory.
set( CELLULAR_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/source/include )
set( CELLULAR_COMMON_INCLUDE_DIRS ${CELLULAR_INCLUDE_DIRS}/common )
set( CELLULAR_PRIVATE_DIRS ${CMAKE_CURRENT_LIST_DIR}/source/include/private )

# Cellular interface include directory.
set( CELLULAR_INTERFACE_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/source/interface )

