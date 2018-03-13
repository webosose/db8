# @@@LICENSE
#
# Copyright (c) 2013-2017 LG Electronics, Inc.
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
# LICENSE@@@

# This CMakeLists.txt contains build instructions for database backend.
# This script MUST provide such variables:
# DB_BACKEND_INCLUDES           - includes for database headers (like leveldb/db.h)
# DB_BACKEND_LIB                - requre libraries for database (like -lleveldb)
# DB_BACKEND_WRAPPER_SOURCES    - wrapper cpp files inside this directory (like MojDbBerkeleyEngine.cpp)
# Optional variable:
# DB_BACKEND_WRAPPER_CFLAGS     - compiller flags

set(WEBOS_DB8_BACKEND "sandwich" CACHE STRING "Backend(s) to use with DB8")

foreach (backend ${WEBOS_DB8_BACKEND})
	message (STATUS "Use database frontend: ${backend}")

	if (backend STREQUAL "berkeleydb")
		# -- check for BerkeleyDB
		# (add an alternate standard path in case building BDB locally: does not override)
		find_library(BDB NAMES db-4.8 PATH /usr/local/BerkeleyDB.4.8/lib)
		if(BDB STREQUAL "BDB-NOTFOUND")
			MESSAGE(FATAL_ERROR "Failed to find BerkleyDB libaries. Please install.")
		endif()

		find_path(BDB_INC db.h
				  PATHS /usr/local/BerkeleyDB.4.8/include
				  PATH_SUFFIXES db4.8)
		if(BDB_INC STREQUAL "BDB_INC-NOTFOUND")
			MESSAGE(FATAL_ERROR "Failed to find BerkleyDB includes. Please install.")
		endif()

		set (DB_BACKEND_INCLUDES ${DB_BACKEND_INCLUDES} ${BDB_INC})
		set (DB_BACKEND_LIB ${DB_BACKEND_LIB} ${BDB})

		set(DB_BACKEND_WRAPPER_SOURCES
			${DB_BACKEND_WRAPPER_SOURCES}
			src/engine/berkeleydb/MojDbBerkeleyEngine.cpp
			src/engine/berkeleydb/MojDbBerkeleyFactory.cpp
			src/engine/berkeleydb/MojDbBerkeleyQuery.cpp
		)

		set (DB_BACKEND_WRAPPER_CFLAGS "${DB_BACKEND_WRAPPER_CFLAGS} -DMOJ_USE_BDB")
	elseif (backend STREQUAL "leveldb")

		# -- check for LevelDB backend
		find_library(LDB NAMES leveldb ${WEBOS_INSTALL_ROOT}/lib)
		if(LDB STREQUAL "LDB-NOTFOUND")
			MESSAGE(FATAL_ERROR "Failed to find LevelDB libaries. Please install.")
		endif()

		set (DB_BACKEND_INCLUDES ${DB_BACKEND_INCLUDES} ${WEBOS_INSTALL_ROOT}/include)
		set (DB_BACKEND_LIB ${DB_BACKEND_LIB} ${LDB})

		set(DB_BACKEND_WRAPPER_SOURCES
			${DB_BACKEND_WRAPPER_SOURCES}
			src/engine/leveldb/defs.cpp
			src/engine/leveldb/MojDbLevelEngine.cpp
			src/engine/leveldb/MojDbLevelFactory.cpp
			src/engine/leveldb/MojDbLevelDatabase.cpp
			src/engine/leveldb/MojDbLevelQuery.cpp
			src/engine/leveldb/MojDbLevelTxn.cpp
			src/engine/leveldb/MojDbLevelSeq.cpp
			src/engine/leveldb/MojDbLevelCursor.cpp
			src/engine/leveldb/MojDbLevelEnv.cpp
			src/engine/leveldb/MojDbLevelIndex.cpp
			src/engine/leveldb/MojDbLevelItem.cpp
			src/engine/leveldb/MojDbLevelTxnIterator.cpp
			src/engine/leveldb/MojDbLevelIterator.cpp
			src/engine/leveldb/MojDbLevelContainerIterator.cpp
	   )

		set (DB_BACKEND_WRAPPER_CFLAGS "${DB_BACKEND_WRAPPER_CFLAGS} -DMOJ_USE_LDB")
	elseif (backend STREQUAL "sandwich")

		# -- check for LevelDB backend
		find_library(LDB NAMES leveldb ${WEBOS_INSTALL_ROOT}/lib)
		if(LDB STREQUAL "LDB-NOTFOUND")
			MESSAGE(FATAL_ERROR "Failed to find LevelDB libaries. Please install.")
		endif()

		set (DB_BACKEND_INCLUDES ${DB_BACKEND_INCLUDES} ${WEBOS_INSTALL_ROOT}/include)
		set (DB_BACKEND_LIB ${DB_BACKEND_LIB} ${LDB})

		set(DB_BACKEND_WRAPPER_SOURCES
			${DB_BACKEND_WRAPPER_SOURCES}
			src/engine/sandwich/MojDbSandwichEngine.cpp
			src/engine/sandwich/MojDbSandwichFactory.cpp
			src/engine/sandwich/MojDbSandwichDatabase.cpp
			src/engine/sandwich/MojDbSandwichQuery.cpp
			src/engine/sandwich/MojDbSandwichTxn.cpp
			src/engine/sandwich/MojDbSandwichSeq.cpp
			src/engine/sandwich/MojDbSandwichEnv.cpp
			src/engine/sandwich/MojDbSandwichIndex.cpp
			src/engine/sandwich/MojDbSandwichItem.cpp
                        src/engine/sandwich/MojDbSandwichLazyUpdater.cpp
		)

		set (DB_BACKEND_WRAPPER_CFLAGS "${DB_BACKEND_WRAPPER_CFLAGS} -DMOJ_USE_SANDWICH")
	elseif (backend STREQUAL "lmdb")
		# -- check for lmdb backend
		find_library(LMDB NAMES lmdb ${WEBOS_INSTALL_ROOT}/lib)
		if(LMDB STREQUAL "LMDB-NOTFOUND")
			MESSAGE(FATAL_ERROR "Failed to find LMDB libaries. Please install.")
		endif()

		set (DB_BACKEND_INCLUDES ${DB_BACKEND_INCLUDES} ${WEBOS_INSTALL_ROOT}/include)
		set (DB_BACKEND_LIB ${DB_BACKEND_LIB} ${LMDB})

		set(DB_BACKEND_WRAPPER_SOURCES
			${DB_BACKEND_WRAPPER_SOURCES}
			src/engine/lmdb/MojDbLmdbEngine.cpp
			src/engine/lmdb/MojDbLmdbFactory.cpp
			src/engine/lmdb/MojDbLmdbDatabase.cpp
			src/engine/lmdb/MojDbLmdbQuery.cpp
			src/engine/lmdb/MojDbLmdbTxn.cpp
			src/engine/lmdb/MojDbLmdbSeq.cpp
			src/engine/lmdb/MojDbLmdbCursor.cpp
			src/engine/lmdb/MojDbLmdbEnv.cpp
			src/engine/lmdb/MojDbLmdbIndex.cpp
			src/engine/lmdb/MojDbLmdbItem.cpp
		)
		set (DB_BACKEND_WRAPPER_CFLAGS "${DB_BACKEND_WRAPPER_CFLAGS} -DMOJ_USE_LMDB")
	else ()
		message(FATAL_ERROR "WEBOS_DB8_BACKEND: unsuported value '${backend}'")
	endif ()
endforeach ()

# -- check for errors
if(NOT DEFINED DB_BACKEND_LIB)
        message(FATAL_ERROR "Not set DB_BACKEND_LIB in ${CMAKE_SOURCE_DIR}/src/db-luna/CMakeLists.txt")
endif()
if (NOT DEFINED DB_BACKEND_INCLUDES)
        message (FATAL_ERROR "Not set DB_BACKEND_INCLUDES in ${CMAKE_SOURCE_DIR}/src/db-luna/CMakeLists.txt")
endif()
if (NOT DEFINED DB_BACKEND_WRAPPER_SOURCES)
        message (FATAL_ERROR "Not set DB_BACKEND_WRAPPER_SOURCES in ${CMAKE_SOURCE_DIR}/src/db-luna/CMakeLists.txt")
endif()
if (NOT DEFINED DB_BACKEND_WRAPPER_CFLAGS)
        message (FATAL_ERROR "Not set DB_BACKEND_WRAPPER_CFLAGS in ${CMAKE_SOURCE_DIR}/src/db-luna/CMakeLists.txt")
endif()
