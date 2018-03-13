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

# -- source for generating libmojodb.so
set(DB_LIB_SOURCES
    MojDb.cpp
    MojDbAdmin.cpp
    MojDbAggregateFilter.cpp
    MojDbClient.cpp
    MojDbCursor.cpp
    MojDbExtractor.cpp
    MojDbIdGenerator.cpp
    MojDbIndex.cpp
    MojDbIsamQuery.cpp
    MojDbKey.cpp
    MojDbKind.cpp
    MojDbKindEngine.cpp
    MojDbKindState.cpp
    MojDbKindIdList.cpp
    MojDbMediaLinkManager.cpp
    MojDbMediaHandler.cpp
    MojDbObjectHeader.cpp
    MojDbObjectItem.cpp
    MojDbPermissionEngine.cpp
    MojDbPutHandler.cpp
    MojDbQuery.cpp
    MojDbQueryFilter.cpp
    MojDbQueryPlan.cpp
    MojDbQuotaEngine.cpp
    MojDbReq.cpp
    MojDbRevisionSet.cpp
    MojDbServiceClient.cpp
    MojDbServiceDefs.cpp
    MojDbServiceHandler.cpp
    MojDbServiceHandlerBase.cpp
    MojDbServiceSchemas.cpp
    MojDbStorageEngine.cpp
    MojDbShardIdCache.cpp
    MojDbShardEngine.cpp
    MojDbShardKindHash.cpp
    MojDbSpaceAlert.cpp
    MojDbShardInfo.cpp
    MojDbTextCollator.cpp
    MojDbTextTokenizer.cpp
    MojDbTextUtils.cpp
    MojDbUtils.cpp
    MojDbWatcher.cpp
    MojDbServiceHandlerInternal.cpp
    MojDbProfileEngine.cpp
    MojDbProfileApplication.cpp
    MojDbProfileStat.cpp
    )

if (BUILD_SEARCH_QUERY_CACHE)
    set(DB_LIB_SOURCES
        ${DB_LIB_SOURCES}
        MojDbSearchCache.cpp
        MojDbSearchCacheCursor.cpp
        )
else()
    set(DB_LIB_SOURCES
        ${DB_LIB_SOURCES}
        MojDbSearchCursor.cpp
    )
endif()

add_library(mojodb SHARED ${DB_LIB_SOURCES})
target_link_libraries(mojodb
                      ${GLIB2_LDFLAGS}
                      ${GTHREAD2_LDFLAGS}
                      mojocore
                      ${LS2_LDFLAGS}
                      ${ICU}
                      ${ICUI18N}
                      )
webos_build_library(TARGET mojodb NOHEADERS)
