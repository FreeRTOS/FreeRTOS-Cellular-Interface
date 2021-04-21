# Include filepaths for source and include.
# include( ${MODULE_ROOT_DIR}/mqttFilePaths.cmake )
# Set the source path and include path
# set( CELLULAR_COMMON_SOURCE ${MODULE_ROOT_DIR}/common/src )
# set( CELLULAR_INCLUDE_DIRS  ${MODULE_ROOT_DIR}/include )
# set( CELLULAR_COMMON_INCLUDE_DIRS ${MODULE_ROOT_DIR}/common/include )
# set( CELLULAR_COMMON_INCLUDE_PRIVATE_DIRS ${MODULE_ROOT_DIR}/common/include/private )

# ====================  Define your project name (edit) ========================
set(project_name "cellular")

# =====================  Create your mock here  (edit)  ========================

# ================= Create the library under test here (edit) ==================

# list the files you would like to test here
list(APPEND real_source_files
            ${CELLULAR_COMMON_SOURCE}/cellular_at_core.c
        )
# list the directories the module under test includes
list(APPEND real_include_directories
            .
            ${CELLULAR_COMMON_INCLUDE_DIRS}
            ${CELLULAR_COMMON_INCLUDE_PRIVATE_DIRS}
        )

# =====================  Create UnitTest Code here (edit)  =====================

# list the directories your test needs to include
list(APPEND test_include_directories
            .
            ${CELLULAR_COMMON_INCLUDE_DIRS}
        )

# =============================  (end edit)  ===================================

set(real_name "${project_name}_at_core_real")

create_real_library(${real_name}
                    "${real_source_files}"
                    "${real_include_directories}"
                    "${mock_name}"
        )

list(APPEND utest_link_list
            lib${real_name}.a
        )

list(APPEND utest_dep_list
            ${real_name}
        )

set(utest_name "${project_name}_at_core_utest")
set(utest_source "${project_name}_at_core_utest.c")
create_test(${utest_name}
            ${utest_source}
            "${utest_link_list}"
            "${utest_dep_list}"
            "${test_include_directories}"
        )
