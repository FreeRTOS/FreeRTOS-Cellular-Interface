cmake_minimum_required( VERSION 3.15 )

project( FreeRTOS-Cellular-Interface LANGUAGES C )

# ------------------------------------------------------------------------------
# Includes
# ------------------------------------------------------------------------------

include( ${CMAKE_CURRENT_LIST_DIR}/cellularInterfaceFilePaths.cmake )

# ------------------------------------------------------------------------------
# Library targets
# ------------------------------------------------------------------------------

add_library( cellular_interface INTERFACE )
target_sources( cellular_interface INTERFACE ${CELLULAR_COMMON_SOURCES} )

target_include_directories( cellular_interface INTERFACE
                            ${CELLULAR_INCLUDE_DIRS}
                            ${CELLULAR_COMMON_INCLUDE_DIRS}
                            ${CELLULAR_INTERFACE_INCLUDE_DIRS}
                            ${CELLULAR_PRIVATE_DIRS} )
