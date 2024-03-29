# Copyright (c) 2012-2018 LG Electronics, Inc.
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

# -- source for generating mojodb-luna
set(LUNA_BIN_SOURCES
    MojDbLunaServiceApp.cpp
    MojDbLunaServiceDb.cpp
    )

foreach(filename ${DB_BACKEND_WRAPPER_SOURCES})
    set(DB_BACKEND_WRAPPER_SOURCES_FULL_PATH ${DB_BACKEND_WRAPPER_SOURCES_FULL_PATH} "${CMAKE_SOURCE_DIR}/${filename}")
endforeach ()

add_executable(mojodb-luna ${LUNA_BIN_SOURCES} ${DB_BACKEND_WRAPPER_SOURCES_FULL_PATH})
target_link_libraries(mojodb-luna
                      ${GLIB2_LDFLAGS}
                      ${GTHREAD2_LDFLAGS}
                      mojocore
                      mojodb
                      mojoluna
                      atomic
                      ${DB_BACKEND_LIB}
                      ${LS2_LDFLAGS}
                      ${ICU}
                      ${ICUI18N})

# specifying files/launch allows creating upstart scripts
# of all files named as <name>.in into upstart/name
# (see cmake_webos)
if(WANT_DYNAMIC)
webos_build_system_bus_files(files/sysbus/dynamic)
webos_build_daemon(NAME mojodb-luna LAUNCH files/launch/dynamic/)
else ()
webos_build_system_bus_files(files/sysbus/static)
webos_build_daemon(NAME mojodb-luna LAUNCH files/launch/static)
endif()
