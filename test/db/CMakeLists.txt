# Copyright (c) 2013-2018 LG Electronics, Inc.
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

include_directories(${CMAKE_SOURCE_DIR}/inc)
set(DB_BACKEND_WRAPPER_SOURCES_CPP "")
foreach(filename ${DB_BACKEND_WRAPPER_SOURCES})
    set(DB_BACKEND_WRAPPER_SOURCES_CPP ${DB_BACKEND_WRAPPER_SOURCES_CPP} "${CMAKE_SOURCE_DIR}/${filename}")
endforeach ()

# --------------------------------
# dbcore target
# ---------------------------------
set (DB_TEST_SOURCES
     MojDbTestRunner.cpp
     MojDbTestStorageEngine.cpp
)

foreach(test
        MojDbAggregate
        MojDbBulk
        MojDbConcurrency
        MojDbCrud
        MojDbDatabaseId
        MojDbDumpLoad
        MojDbIndex
        MojDbKind
        MojDbLocale
        MojDbPermission
        MojDbPurge
        MojDbQuery
        MojDbQueryFilter
        MojDbQuota
        MojDbRev
        MojDbRevisionSet
        MojDbSearch
        MojDbSearchCache
        MojDbDistinct
        MojDbTextCollator
        MojDbTextTokenizer
        MojDbWatch
        MojDbWhere
        MojDbTxn
        MojDbCursorTxn
        MojDbNewId
        MojDbShardManager
        MojDbProfile
        )
	foreach (backend ${WEBOS_DB8_BACKEND})
		add_test(NAME ${backend}/${test} COMMAND env MOJODB_ENGINE=${backend} ${CMAKE_CURRENT_BINARY_DIR}/test_db_core ${test})
	endforeach()
    set(DB_TEST_SOURCES ${DB_TEST_SOURCES} ${test}Test.cpp)
endforeach()

add_executable(test_db_core ${DB_TEST_SOURCES} ${DB_BACKEND_WRAPPER_SOURCES_CPP})
target_link_libraries(test_db_core
                      mojocore
                      mojodb
                      atomic
                      ${DB_BACKEND_LIB}
)
install(PROGRAMS ${CMAKE_BINARY_DIR}/test/db/test_db_core
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

# --------------------------------
# performance test target
# ---------------------------------
set (DB_PERF_TEST_SOURCES
     MojDbPerfTestRunner.cpp
     MojDbPerfCacheReadTest.cpp
     MojDbPerfTest.cpp
     MojDbPerfIndexTest.cpp
     MojDbPerfCreateTest.cpp
     MojDbPerfDeleteTest.cpp
     MojDbPerfReadTest.cpp
     MojDbPerfUpdateTest.cpp
)

add_executable(test_db_performance ${DB_PERF_TEST_SOURCES} ${DB_BACKEND_WRAPPER_SOURCES_CPP})
target_link_libraries(test_db_performance
                      mojocore
                      mojodb
                      atomic
                      ${DB_BACKEND_LIB}
)
install(PROGRAMS ${CMAKE_BINARY_DIR}/test/db/test_db_performance
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

# --------------------------------
# stress test target
# ---------------------------------
set (DB_STRESS_TEST_SOURCES
     MojDbStressTest.cpp
)

add_executable(test_db_stress ${DB_STRESS_TEST_SOURCES} ${DB_BACKEND_WRAPPER_SOURCES_CPP})
target_link_libraries(test_db_stress
                      mojocore
                      mojodb
                      mojoluna
                      atomic
                      ${DB_BACKEND_LIB}
)
install(PROGRAMS ${CMAKE_BINARY_DIR}/test/db/test_db_stress
        DESTINATION ${WEBOS_INSTALL_LIBDIR}/${CMAKE_PROJECT_NAME}/tests
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ)

#add_test(NAME MojDbStressTest COMMAND test_db_stress)
