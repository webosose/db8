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

# -- source for generating libmojoluna.so
set(LUNA_LIB_SOURCES
    MojLunaErr.cpp
    MojLunaMessage.cpp
    MojLunaRequest.cpp
    MojLunaService.cpp
    )

add_library(mojoluna SHARED ${LUNA_LIB_SOURCES})
target_link_libraries(mojoluna
                      ${GLIB2_LDFLAGS}
                      ${GTHREAD2_LDFLAGS}
                      mojocore
                      ${LS2_LDFLAGS}
                      ${ICU}
                      ${ICUI18N}
                      )
webos_build_library(TARGET mojoluna NOHEADERS)
