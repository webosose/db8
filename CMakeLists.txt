# Copyright (c) 2012-2019 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 2.8.7)

project(db8 CXX)

enable_testing()

include(webOS/webOS)
webos_modules_init(1 0 0 QUALIFIER RC7)
webos_component(3 2 0)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Available build options
option (USE_PMLOG                "Enable PmLogLib logging"   ON)
option (BUILD_SEARCH_QUERY_CACHE "Enable Search Query Cache" ON)
option (BUILD_LS2                "Enable Luna Bus handler"   ON)
option (WEBOS_CONFIG_BUILD_TESTS "Build unit tests"          ON)
option (WANT_SNAPPY_COMPRESSION  "Enable compression"        ON)
option (WANT_DYNAMIC             "Switch to dynamic service" OFF)
option (WANT_PROFILING           "Enable profiling"          OFF)
option (BUILD_LMDB               "Enable lmdb"               OFF)

include(FindPkgConfig)

# -- add local include paths (should override already installed ones)
include_directories(inc)

# -- check for glib 2.0
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

# -- check for gthread 2.0
pkg_check_modules(GTHREAD2 REQUIRED gthread-2.0)
include_directories(${GTHREAD2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GTHREAD2_CFLAGS_OTHER})

# -- Check for Boost library
find_package(Boost 1.40 COMPONENTS regex REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

if (USE_PMLOG)
    # -- check for pmLogLib
    pkg_check_modules(PMLOG REQUIRED PmLogLib>=3)
    include_directories(${PMLOG_INCLUDE_DIRS})
    webos_add_compiler_flags(ALL ${PMLOG_CFLAGS_OTHER} -DUSE_PMLOG)
endif()

if (WANT_DYNAMIC)
    webos_add_compiler_flags(ALL -DWANT_DYNAMIC)
endif()

if (BUILD_LMDB)
    webos_add_compiler_flags(ALL -DLMDB_ENGINE_SUPPORT)
endif()

if (WANT_PROFILING)
    set(WANT_PROFILING true)
else()
    set(WANT_PROFILING false)
endif()

# -- check for ICU
find_library(ICU NAMES icuuc)
if(ICU STREQUAL "ICU-NOTFOUND")
    message(FATAL_ERROR "Failed to find ICU4C libaries. Please install.")
endif()

find_library(ICUI18N NAMES icui18n)
if(ICUI18N STREQUAL "ICUI18N-NOTFOUND")
    message(FATAL_ERROR "Failed to find ICU-I18N libaries. Please install.")
endif()

# -- check for snappy library
set (WITH_SNAPPY_COMPRESSION 0)
find_package(Snappy 1.1.2 QUIET)
if (SNAPPY_FOUND)
    webos_add_compiler_flags(ALL -DWITH_SNAPPY_COMPRESSION)
    if (WANT_SNAPPY_COMPRESSION)
        set (WITH_SNAPPY_COMPRESSION 1)
    else()
        set (WITH_SNAPPY_COMPRESSION 0)
    endif()

    message(STATUS "Configured with Snappy Compression support, the package version is ${SNAPPY_VERSION}")
else()
    message(STATUS "Configured with Snappy Compression support, but the package was not found.")
endif()

# configure database backend.
include(${CMAKE_SOURCE_DIR}/src/db-luna/BackendBuildRules.cmake)

# provide both db8 and mojodb pkgconfigs
webos_build_pkgconfig(files/pkgconfig/mojodb)
webos_build_pkgconfig(files/pkgconfig/db8)

webos_add_compiler_flags(ALL -Wall -Wextra -fPIC -Wconversion --std=c++0x
                         -Wno-unused-but-set-variable -Wno-unused-variable
                         -fno-exceptions -frename-registers -DMOJ_LINUX -fno-strict-aliasing
                         -Wno-psabi
                         -DMOJ_INTERNAL -D_REENTRANT)


# Some really useful errors with low false-postivies
webos_add_compiler_flags(ALL
    -Werror=return-type
    )

# TODO: fix warnings
webos_add_compiler_flags(ALL
    -Wno-unused-parameter
    -Wno-empty-body
    )

set(WEBOS_USE_WERROR FALSE CACHE BOOL "Set to TRUE to enable -Werror")
if(WEBOS_USE_WERROR)
    message("Build with treating most warnings as errors")
    webos_add_compiler_flags(ALL
        -Werror
        -Wno-error=conversion
        -Wno-error=missing-field-initializers
        -Wno-error=type-limits
        -Wno-error=deprecated-declarations
        )
endif()

# Note: no-psabi will disable notice for gcc: the mangling of 'va_list' has changed in GCC 4.4

# to turn on extra debug information (like output to log dirty buffers data) add debug compile flag -DMOJ_DEBUG_LOGGING
webos_add_compiler_flags(DEBUG -DMOJ_DEBUG)
webos_add_linker_options(ALL --no-undefined)


add_subdirectory(src/core)

if (BUILD_SEARCH_QUERY_CACHE)
    webos_add_compiler_flags(ALL -DWITH_SEARCH_QUERY_CACHE)
endif()

add_subdirectory(src/db)

if(BUILD_LS2)
    # -- check for LS2 (internally depends on pmloglib)
    pkg_check_modules(LS2 REQUIRED luna-service2>=3.10.0)
    include_directories(${LS2_INCLUDE_DIRS})
    webos_add_compiler_flags(ALL ${LS2_CFLAGS_OTHER})

    add_subdirectory(src/luna)

    webos_add_compiler_flags(ALL ${DB_BACKEND_WRAPPER_CFLAGS})

    include_directories (${DB_BACKEND_INCLUDES})
    add_subdirectory(src/db-luna)

    #install error handling scripts
    install (FILES files/scripts/errorOpenMainDb.bash DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/bin PERMISSIONS OWNER_EXECUTE OWNER_READ)
    install (FILES files/scripts/errorOpenMediaDb.bash DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/bin PERMISSIONS OWNER_EXECUTE OWNER_READ)
    install (FILES files/scripts/errorOpenTempDb.bash DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/bin PERMISSIONS OWNER_EXECUTE OWNER_READ)
    install (FILES files/scripts/errorNoSpace.bash DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/bin PERMISSIONS OWNER_EXECUTE OWNER_READ)

    # configuration files per database
    webos_build_configured_file(files/conf/maindb.conf WEBOS_SYSCONFDIR db8)
    webos_build_configured_file(files/conf/mediadb.conf WEBOS_SYSCONFDIR db8)
    webos_build_configured_file(files/conf/tempdb.conf WEBOS_SYSCONFDIR db8)

    # extras
    install(FILES src/db-luna/backup-com.palm.db DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/backup)
    install(FILES src/db-luna/activity-com.palm.db.purge.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.palm.db)
    install(FILES src/db-luna/activity-com.palm.db.space.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.palm.db)
    install(FILES src/db-luna/activity-com.palm.tempdb.space.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.palm.tempdb)
    install(FILES src/db-luna/activity-com.palm.tempdb.purge.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.palm.tempdb)
    install(FILES src/db-luna/activity-com.webos.mediadb.purge.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.webos.mediadb)
    install(FILES src/db-luna/activity-com.webos.mediadb.space.json DESTINATION ${WEBOS_INSTALL_WEBOS_SYSCONFDIR}/activities/com.webos.mediadb)
endif() # BUILD_LS2

install(DIRECTORY inc/ DESTINATION ${WEBOS_INSTALL_INCLUDEDIR}/mojodb/)

if (WEBOS_CONFIG_BUILD_TESTS)
    add_subdirectory(test/core)
    add_subdirectory(test/db)
    add_subdirectory(test/dbclient)

    if (BUILD_LS2)
        add_subdirectory(test/luna-service)
    endif()
    add_subdirectory(test/db-scripts)

    webos_use_gtest()
    add_subdirectory(test/mojocore)
    add_subdirectory(test/db-core)
    foreach (backend ${WEBOS_DB8_BACKEND})
        if (backend STREQUAL "leveldb")
            add_subdirectory(test/leveldb)
        endif ()
        if (backend STREQUAL "sandwich")
            add_subdirectory(test/sandwich)
        endif ()
        if (backend STREQUAL "lmdb")
            add_subdirectory(test/lmdb)
        endif ()
    endforeach ()
    add_subdirectory(test/dbclient-perf)
    add_subdirectory(test/volume)
    #add_subdirectory(test/db-media)

    include(ProcessorCount)
    ProcessorCount(n)
    if (NOT n EQUAL 0)
        set(ctest_args ${ctest_args} -j${n})
    endif ()

endif()
add_subdirectory(tool/dbgen)

if (WEBOS_DB8_MIGRATE_TOOL STREQUAL "lmdb")
    add_subdirectory(tool/lmdbmigrate)
else ()
    add_subdirectory(tool/sandwichmigrate)
endif()

if(LDBVIEWER)
    add_subdirectory(tool/ldbviewer)
endif()

add_subdirectory(test/performance)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure ${ctest_args})
