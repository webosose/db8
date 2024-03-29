# Copyright (c) 2015-2018 LG Electronics, Inc.
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

include_directories(${WEBOS_GTEST_INCLUDES})
add_definitions(-Wall -Wno-deprecated -pthread -std=c++14)

foreach(filename ${DB_BACKEND_WRAPPER_SOURCES})
    set(DB_BACKEND_WRAPPER_SOURCES_CPP ${DB_BACKEND_WRAPPER_SOURCES_CPP} "${CMAKE_SOURCE_DIR}/${filename}")
endforeach ()


set (DB_CLIENT_TEST_SOURCES
     DbGenerator.cpp
     CpuMemStats.cpp
     DbClientPerfTest.cpp
     DbClientSearchPerfTest.cpp
     MojFakeService.cpp
     MojFakeMessage.cpp
     Runner.cpp
     ${DB_BACKEND_WRAPPER_SOURCES_CPP}
)

add_executable(gtest_dbclient_perf ${DB_CLIENT_TEST_SOURCES})
target_link_libraries(gtest_dbclient_perf
                      mojocore
                      mojodb
                      mojoluna
                      atomic
                      ${DB_BACKEND_LIB}
)
target_link_libraries(gtest_dbclient_perf ${WEBOS_GTEST_LIBRARIES})

install(PROGRAMS ${CMAKE_BINARY_DIR}/test/dbclient-perf/gtest_dbclient_perf
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

add_subdirectory(plotter)
