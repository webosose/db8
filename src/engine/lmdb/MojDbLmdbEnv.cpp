// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
#include <array>
#include <map>
#include "engine/lmdb/MojDbLmdbErr.h"
#include "core/MojLogDb8.h"
#include "engine/lmdb/MojDbLmdbEnv.h"
#include "engine/lmdb/MojDbLmdbEngine.h"
#define DEFAULT_MAX_DB_COUNT 6
#define DEFAULT_MAP_SIZE 1073741824
const MojChar* const MojDbLmdbEnv::LockFileName = _T("_lmdblock");

MojDbLmdbEnv::MojDbLmdbEnv()
: m_lockFile(MojInvalidFile),
  m_env(nullptr)
{
}

MojDbLmdbEnv::~MojDbLmdbEnv()
{
	MojErr err = close();
	MojErrCatchAll(err);
}

MojErr MojDbLmdbEnv::configure(const MojObject& conf)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	m_conf = conf;
	return MojErrNone;
}
MojErr MojDbLmdbEnv::create()
{
	MojAssert(!m_env);
	int dbErr = mdb_env_create(&m_env);
	MojLmdbErrCheck(dbErr, _T("mdb_env_create"));
	MojAssert(m_env);
	return MojErrNone;
}

MojErr MojDbLmdbEnv::open(const MojChar* dir)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = MojErrNone;
	err = create();
	MojAssert(dir);
	// lock env
	err = lockDir(dir);
	MojErrCheck(err);
	MojUInt32 maxDBCount;
	bool found;
	err = m_conf.get("maxDBCount", maxDBCount, found);
	MojErrCheck(err);
	if (!found) {
		 maxDBCount = DEFAULT_MAX_DB_COUNT;
	}
	int dbErr = mdb_env_set_maxdbs(m_env, maxDBCount);
	MojLmdbErrCheck(dbErr, _T("mdb_env_set_maxdbs"));
	MojUInt32 mapSize;
	err = m_conf.get("mapSize", mapSize, found);
	MojErrCheck(err);
	if (!found) {
		mapSize = DEFAULT_MAP_SIZE;
	}
	dbErr = mdb_env_set_mapsize(m_env, mapSize);
	MojLmdbErrCheck(dbErr, _T("mdb_env_set_mapsize"));
	MojUInt32 maxReaders;
	bool foundOut = false;
	err = m_conf.get("maxReaders", maxReaders, foundOut);
	MojErrCheck(err);
	if (foundOut) {
		dbErr = mdb_env_set_maxreaders(m_env, maxReaders);
		MojLmdbErrCheck(dbErr, _T("mdb_env_set_maxreaders"));
	}
	MojUInt32 mdbFlags = 0;
	MojInt8 index = -1;
	std::array<MojUInt32, 10> envFlags = {
	MDB_WRITEMAP, MDB_NOLOCK, MDB_RDONLY, MDB_NOMETASYNC, MDB_MAPASYNC,
	MDB_NOMEMINIT, MDB_NORDAHEAD, MDB_NOTLS, MDB_NOSUBDIR, MDB_FIXEDMAP };
	std::array<std::string, 10> confKeys = { "writeMap", "noLock", "readOnly",
			"noMetaSync", "mapAsyc", "noMemInit", "noReadAhead", "noTls", "noSubDir", "fixedMap" };
	std::map<std::string,MojUInt32> defaultConf =   {{"noTls", MDB_NOTLS},
							 {"writeMap", 0},
							 {"noLock", 0},
							 {"readOnly", 0},
							 {"noMetaSync", 0},
							 {"mapAsyc", 0},
							 {"noMemInit", 0},
							 {"noReadAhead", 0},
							 {"noSubDir", 0},
							 {"fixedMap", 0}
							};

	for(auto it = confKeys.begin();it != confKeys.end();it++) {
		index++;
		MojUInt32 flag = 0;
		bool found = false;
		err = m_conf.get(it->c_str(), flag, found);
		MojErrCheck(err);
		if (found && flag)
		{
			mdbFlags |= envFlags[index];
		} else {
			mdbFlags |= defaultConf.find(*it)->second;
		}
	}
	/* open env */
	dbErr = mdb_env_open(m_env, dir, mdbFlags, 0664);
	MojLmdbErrCheck(dbErr, _T("mdb_env_open"));
	return MojErrNone;
}

MojErr MojDbLmdbEnv::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = MojErrNone;
	MojErr errClose = MojErrNone;

	if (m_env) {
		mdb_env_close(m_env);
		m_env = nullptr;
	}
	errClose = unlockDir();
	MojErrAccumulate(err, errClose);

	return err;
}

MojErr MojDbLmdbEnv::lockDir(const MojChar* path)
{
	LOG_TRACE("Entering function %s", __FUNCTION__); MojAssert(path);

	MojErr err = MojCreateDirIfNotPresent(path);
	MojErrCheck(err);
	err = m_lockFileName.format(_T("%s/%s"), path, LockFileName);
	MojErrCheck(err);
	err = m_lockFile.open(m_lockFileName, MOJ_O_RDWR | MOJ_O_CREAT, MOJ_S_IRUSR | MOJ_S_IWUSR);
	MojErrCheck(err);
	err = m_lockFile.lock(LOCK_EX | LOCK_NB);
	MojErrCatch(err, MojErrWouldBlock) {
		(void) m_lockFile.close();
		MojErrThrowMsg(MojErrLocked, _T("Database at '%s' locked by another instance."), path);
	}
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbEnv::unlockDir()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = MojErrNone;
	if (m_lockFile.open()) {
		// unlink before we close to ensure that we hold
		// the lock to the bitter end
		MojErr errClose = MojUnlink(m_lockFileName);
		MojErrAccumulate(err, errClose);
		errClose = m_lockFile.close();
		MojErrAccumulate(err, errClose);
	}
	return err;
}
