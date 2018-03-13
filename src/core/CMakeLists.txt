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

set_source_files_properties(MojSchema.cpp PROPERTIES COMPILE_FLAGS -fexceptions)

# -- source for generating libmojocore.so
set(CORE_LIB_SOURCES
    MojApp.cpp
    MojBuffer.cpp
    MojDataSerialization.cpp
    MojDecimal.cpp
    MojEpollReactor.cpp
    MojErr.cpp
    MojFile.cpp
    MojGmainReactor.cpp
    MojHashBase.cpp
    MojJson.cpp
    MojList.cpp
    MojLog.cpp
    MojLogDb8.cpp
    MojLogEngine.cpp
    MojMessageDispatcher.cpp
    MojObject.cpp
    MojObjectBuilder.cpp
    MojObjectFilter.cpp
    MojObjectSerialization.cpp
    MojOs.cpp
    MojPmLogAppender.cpp
    MojRbTreeBase.cpp
    MojRefCount.cpp
    MojSchema.cpp
    MojService.cpp
    MojServiceApp.cpp
    MojServiceMessage.cpp
    MojServiceRequest.cpp
    MojSignal.cpp
    MojSock.cpp
    MojString.cpp
    MojTestRunner.cpp
    MojThread.cpp
    MojTokenSet.cpp
    MojUtil.cpp
    )

add_library(mojocore SHARED ${CORE_LIB_SOURCES})

target_link_libraries(mojocore
                      ${GLIB2_LDFLAGS}
                      ${GTHREAD2_LDFLAGS}
                      ${LS2_LDFLAGS}
                      ${PMLOG_LDFLAGS}
                      ${Boost_LIBRARIES}
                      )
webos_build_library(TARGET mojocore NOHEADERS)