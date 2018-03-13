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

#include "db/MojDb.h"
#include "core/MojUtil.h"
#include <iostream>
#include <deque>
#include <vector>
#include <string>

#include <core/MojOs.h>
#include <leveldb/txn_db.hpp>
#include <leveldb/bottom_db.hpp>
#include "leveldb/sandwich_db.hpp"
#include <engine/lmdb/MojDbLmdbErr.h>
#include <lmdb.h>
#define DEFAULT_MAX_DB_COUNT 6
#define DEFAULT_MAP_SIZE 1073741824

MojErr doMigrate(MojObject& confObj)
{
    int idx = 0;

    std::deque<std::string> parts_ldb = { "UsageDbName","indexIds.db", "indexes.ldb", "kinds.db", "objects.db", "seq.ldb",};
    std::deque<std::string> parts_mdb = { "UsageDbName","indexIds.db", "indexes.mdb", "kinds.db", "objects.db", "seq.mdb",};

    MojErr err;
    int result = 0;
    MojObject dbConf;
    err = confObj.getRequired("db", dbConf);
    MojErrCheck(err);

    MojObject dbPath;
    err = dbConf.getRequired("path", dbPath);
    MojErrCheck(err);

    MojString dbName;
    MojString path;
    err = dbPath.stringValue(path);
    MojErrCheck(err);

    MojString checkFolder;
    err = checkFolder.append(path);
    MojErrCheck(err);
    err = checkFolder.append("/data.mdb");
    MojErrCheck(err);
    MojStatT stat;
    err = MojStat(checkFolder, &stat);
    if (err != MojErrNotFound)
    {
        LOG_INFO(MSGID_MOJ_DB_WARNING, 0, _T("Database is already converted"));
        return MojErrNone;
    }

    MojSize pos = path.rfind('/');
    path.substring(pos, path.length()-pos, dbName);

    MojString lockFile;
    lockFile.append(path);
    lockFile.append("/migration.lock");
    if (!access(lockFile.data(), F_OK))
    {
        MojErrThrowMsg(MojErrExists, _T("Database is locked"));
    }

    typedef leveldb::SandwichDB<leveldb::TxnDB<leveldb::BottomDB>> BackendDbTxn;
    typedef leveldb::SandwichDB<leveldb::BottomDB> BackendDb;
    BackendDb cdb;

    //check - valid old database
    auto s = cdb->Open(path.data());
    if (!s.ok())
    {
        MojErrThrowMsg(MojErrDbIO, _T("Failed to open destination database %s:%s"), path.data(), s.ToString().c_str());
    }
    BackendDbTxn txnDb = cdb.ref<leveldb::TxnDB>();

    MojString bkPath;
    bkPath.append(WEBOS_PERSISTENTSTORAGEDIR "/db8/migration");
    bkPath.append(dbName);

    //remove temp
    MojString execCommand;
    err = MojRmDirRecursive(bkPath.data());
    MojErrCatch(err, MojErrNotFound) {};
    MojErrCheck(err);

    //create temporary folder
    err = MojCreateDirIfNotPresent(bkPath.data());
    MojErrCheck(err);

    //create file identifer for identification any troubles during process
    FILE* pFile = fopen(lockFile.data(), "a");
    if (!pFile)
        return MojErrAccessDenied;
    fclose(pFile);

    int rc_lmdb;
    MDB_env *env_lmdb;
    bool found = false;
    rc_lmdb = mdb_env_create(&env_lmdb);
    MojLmdbErrCheck(rc_lmdb, _T("mdb_env_create"));
    MojUInt32 maxDBCount = 0;
    err = confObj.get("maxDBCount", maxDBCount, found);
    MojErrCheck(err);
    if (!found) {
        maxDBCount = DEFAULT_MAX_DB_COUNT;
    }
    rc_lmdb = mdb_env_set_maxdbs(env_lmdb, maxDBCount);
    MojLmdbErrCheck(rc_lmdb, _T("mdb_env_set_maxdbs"));
    MojUInt32 mapSize;
    err = confObj.get("mapSize", mapSize, found);
    MojErrCheck(err);
    if (!found) {
        mapSize = DEFAULT_MAP_SIZE;
    }
    rc_lmdb = mdb_env_set_mapsize(env_lmdb, mapSize);
    MojLmdbErrCheck(rc_lmdb, _T("mdb_env_set_mapsize"));
    rc_lmdb = mdb_env_open(env_lmdb, bkPath, 0, 0664);
    MojLmdbErrCheck(rc_lmdb, _T("mdb_env_open"));

    for (const auto &part : parts_ldb)
    {
        auto pdb = txnDb.use(part);
        decltype(pdb)::Walker it {pdb };

        MDB_val m_key;
        MDB_val m_value;
        MDB_dbi db_lmdb;
        MDB_txn *txn_lmdb;

        std::vector<MojByte*> chunks;

        rc_lmdb = mdb_txn_begin(env_lmdb, NULL, 0, &txn_lmdb);
        MojLmdbErrCheck(rc_lmdb, _T("txn begin"));
        rc_lmdb = mdb_open(txn_lmdb, parts_mdb[idx++].c_str(), MDB_CREATE, &db_lmdb);
        MojLmdbErrCheck(rc_lmdb, _T("mdb_open"));
        for (it.SeekToFirst(); it.Valid(); it.Next())
        {
            m_key.mv_data = nullptr;
            m_key.mv_size = 0;
            m_value.mv_data = nullptr;
            m_value.mv_size = 0;

            m_key.mv_size = static_cast<size_t>(it.key().size());
            MojByte* newKey = (MojByte*)MojMalloc(m_key.mv_size);
            MojAllocCheck(newKey);
            memset (newKey,'\0',m_key.mv_size);
            MojMemCpy(newKey, it.key().data(), m_key.mv_size);
            m_key.mv_data = newKey;
            chunks.push_back(newKey);

            m_value.mv_size = static_cast<size_t>(it.value().size());
            MojByte* newValue = (MojByte*)MojMalloc(m_value.mv_size);
            MojAllocCheck(newValue);
            memset (newValue,'\0',m_value.mv_size);
            MojMemCpy(newValue, it.value().data(),m_value.mv_size);
            m_value.mv_data = newValue;
            chunks.push_back(newValue);

            rc_lmdb = mdb_put(txn_lmdb, db_lmdb, &m_key, &m_value, 0);
            if (rc_lmdb)
            {
                mdb_close(env_lmdb, db_lmdb);
                mdb_env_close(env_lmdb);
                for (int i = 0; i < chunks.size(); ++i)
                {
                    MojFree(chunks[i]);
                }
                chunks.clear();
                MojLmdbErrCheck(rc_lmdb, _T("Database Put error"));
            }
        }//end inner for
        rc_lmdb = mdb_txn_commit(txn_lmdb);
        for (int i = 0; i < chunks.size(); ++i)
        {
            MojFree(chunks[i]);
        }
        chunks.clear();
        MojLmdbErrCheck(rc_lmdb, _T("Commit error"));
        mdb_close(env_lmdb, db_lmdb);
    }//end outer for

    mdb_env_close(env_lmdb);

    //remove old-database
    execCommand.clear();
    execCommand.append("rm -rf ");
    execCommand.append(path);
    execCommand.append("/*");
    result = system(execCommand.data());
    MojErrCheck(result);

    //copy
    execCommand.clear();
    execCommand.append("cp -rf ");
    execCommand.append(bkPath);
    execCommand.append("/* ");
    execCommand.append(path);
    result = system(execCommand.data());
    MojErrCheck(result);

    //remove temp
    execCommand.clear();
    execCommand.append("rm -r ");
    execCommand.append(bkPath);
    result = system(execCommand.data());
    MojErrCheck(result);

    return MojErrNone;
}

int main(int argc, char**argv)
{
    if (argc != 2)
    {
        LOG_ERROR(MSGID_DB_ERROR, 0, "Invalid arg, This program need args(config file path)");
        return -1;
    }

    MojObject confObj;
    MojErr err;
    MojString confStr;
    err = MojFileToString(argv[1], confStr);
    if (err != MojErrNone)
    {
        LOG_ERROR(MSGID_DB_ERROR, 0, "Can't read configuration file");
        return 0;
    }
    err = confObj.fromJson(confStr);
    if (err != MojErrNone)
    {
        MojString errStr;
        MojErrToString(err, errStr);
        LOG_ERROR(MSGID_DB_ERROR, 0, errStr.data());
        return 0;
    }

    err = doMigrate(confObj);
    if (err != MojErrNone)
    {
        LOG_ERROR(MSGID_DB_ERROR, 0, "Can't convert database");
        return 0;
    }

    return 0;
}

