// Copyright (c) 2009-2021 LG Electronics, Inc.
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

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/iterator.h>
#include <leveldb/ref_db.hpp>
#include <sys/statvfs.h>
#include <leveldb/cache.h>

#include "engine/sandwich/MojDbSandwichEngine.h"
#include "engine/sandwich/MojDbSandwichFactory.h"
#include "engine/sandwich/MojDbSandwichDatabase.h"
#include "engine/sandwich/MojDbSandwichQuery.h"
#include "engine/sandwich/MojDbSandwichSeq.h"
#include "engine/sandwich/MojDbSandwichTxn.h"
#include "engine/sandwich/MojDbSandwichEnv.h"
#include "engine/sandwich/defs.h"
#include "engine/sandwich/MojDbSandwichLazyUpdater.h"


#include "db/MojDbObjectHeader.h"
#include "db/MojDbQueryPlan.h"
#include "db/MojDb.h"
#include "core/MojObjectBuilder.h"
#include "core/MojObjectSerialization.h"
#include "core/MojString.h"
#include "core/MojTokenSet.h"



//const MojChar* const MojDbSandwichEnv::LockFileName = _T("_lock");
//db.ldb
static const MojChar* const MojEnvIndexDbName = _T("indexes.ldb");
static const MojChar* const MojEnvSeqDbName = _T("seq.ldb");

leveldb::ReadOptions MojDbSandwichEngine::ReadOptions;
leveldb::WriteOptions MojDbSandwichEngine::WriteOptions;
leveldb::Options MojDbSandwichEngine::OpenOptions;


////////////////////MojDbSandwichEngine////////////////////////////////////////////

MojDbSandwichEngine::MojDbSandwichEngine()
: m_isOpen(false), m_lazySync(false), m_updater(NULL)
{
    m_updater = new MojDbSandwichLazyUpdater;
}


MojDbSandwichEngine::~MojDbSandwichEngine()
{
    MojErr err =  close();
    MojErrCatchAll(err);

    if (m_updater)
        delete m_updater;
}

MojErr MojDbSandwichEngine::cook(const leveldb::Slice &name, mojo::Sandwich::Cookie &cookie)
{
    leveldb::Status status = m_db.cook(name, cookie);
    MojLdbErrCheck(status, _T("failed to find/allocate cookie"));
    return MojErrNone;
}

MojErr MojDbSandwichEngine::configure(const MojObject& config)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    bool found=false;
    MojInt32 syncOption=0;
    MojErr err = config.get("sync", syncOption, found);
    MojErrCheck(err);
    if (false == found) {
        WriteOptions.sync = true;
    } else {
        switch(syncOption) {
            case 2:
                WriteOptions.sync = false;
                m_lazySync = true;
                break;

            case 1:
            case 0:
                WriteOptions.sync = !!syncOption;
                break;
        }
    }

    if (!config.get("fill_cache", ReadOptions.fill_cache)) {
        ReadOptions.fill_cache = true;
    }

    if (!config.get("verify_checksums", ReadOptions.verify_checksums)) {
        ReadOptions.verify_checksums = false;
    }

    if (!config.get("paranoid_checks", OpenOptions.paranoid_checks)) {
        OpenOptions.paranoid_checks = true;
    }

#ifdef WITH_SNAPPY_COMPRESSION
    MojInt64 compression;
    if (!config.get("compression", compression)) {
        OpenOptions.compression = leveldb::CompressionType::kSnappyCompression;
    }
    else {
        switch (compression) {
            case 0:
                OpenOptions.compression = leveldb::CompressionType::kNoCompression;
                break;
            case 1:
                OpenOptions.compression = leveldb::CompressionType::kSnappyCompression;
                break;
            default:
                LOG_ERROR (MSGID_DB_ERROR, 0, "compression parameter is not valid");
                return MojErrInvalidArg;
        }
    }
#endif

    OpenOptions.create_if_missing = true;

    // cache option
    MojInt64 cacheSize = 0L;
    if (config.get("cacheSize", cacheSize)) {
        OpenOptions.block_cache = leveldb::NewLRUCache(cacheSize);
    }

    return MojErrNone;
}

// although we might have only 1 real Level DB - just to be safe and consistent
// provide interface to add/drop multiple DB
MojErr MojDbSandwichEngine::drop(const MojChar* path, MojDbStorageTxn* txn)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(m_isOpen);

    MojThreadGuard guard(m_dbMutex);

    // close sequences
    for (SequenceVec::ConstIterator i = m_seqs.begin(); i != m_seqs.end(); ++i) {
        MojErr err = (*i)->close();
        MojErrCheck(err);
    }
    m_seqs.clear();

    // TODO: drop transaction

    return MojErrNone;
}
MojErr MojDbSandwichEngine::open(const MojChar* path)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(path);
    MojAssert(!m_env.get() && !m_isOpen);

    // this is more like a placeholder
    MojRefCountedPtr<MojDbSandwichEnv> env(new MojDbSandwichEnv);
    MojAllocCheck(env.get());
    MojErr err = env->open(path);
    MojErrCheck(err);
    err = open(path, env.get());
    MojErrCheck(err);
    err = m_path.assign(path);
    MojErrCheck(err);

    if (lazySync())
        m_updater->start();

    return MojErrNone;
}

MojErr MojDbSandwichEngine::open(const MojChar* path, MojDbEnv* env)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojDbSandwichEnv* bEnv = static_cast<MojDbSandwichEnv *> (env);
    MojAssert(bEnv);
    MojAssert(!m_env.get() && !m_isOpen);

    m_env.reset(bEnv);
    if (path) {
        MojErr err = m_path.assign(path);
        MojErrCheck(err);
        // create dir
        err = MojCreateDirIfNotPresent(path);
        MojErrCheck(err);
    }

    // TODO: consider moving to configure
    m_bottom.options = MojDbSandwichEngine::getOpenOptions();
    m_bottom.writeOptions = MojDbSandwichEngine::getWriteOptions();
    m_bottom.readOptions = MojDbSandwichEngine::getReadOptions();

    if (path)
    {
        leveldb::Status status = m_bottom.Open(path);

        if (status.IsCorruption()) {    // database corrupted
            // try restore database
            // AHTUNG! After restore database can lost some data!
            status = leveldb::RepairDB(path, MojDbSandwichEngine::getOpenOptions());
            MojLdbErrCheck(status, _T("db corrupted"));
            status = m_bottom.Open(path);  // database restored, re-open
        }

        MojLdbErrCheck(status, _T("db_create/db_open"));
    }

    // open seqence db
    m_seqDb.reset(new MojDbSandwichDatabase(m_db.use(MojEnvSeqDbName)));
    MojAllocCheck(m_seqDb.get());
    MojErr err = m_seqDb->open(MojEnvSeqDbName, this);
    MojErrCheck(err);

    // open index db
    m_indexDb.reset(new MojDbSandwichDatabase(m_db.use(MojEnvIndexDbName)));
    MojAllocCheck(m_indexDb.get());
    err = m_indexDb->open(MojEnvIndexDbName, this);
    MojErrCheck(err);
    m_isOpen = true;

    if (lazySync())
        m_updater->start();

    return MojErrNone;
}

MojErr MojDbSandwichEngine::close()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojErr err = MojErrNone;
    MojErr errClose = MojErrNone;

    // close seqs before closing their databases
    m_seqs.clear();

    // close dbs
    if (m_seqDb.get()) {
        errClose = m_seqDb->close();
        MojErrAccumulate(err, errClose);
        m_seqDb.reset();
    }
    if (m_indexDb.get()) {
        errClose = m_indexDb->close();
        MojErrAccumulate(err, errClose);
        m_indexDb.reset();
    }
    m_env.reset();
    m_isOpen = false;

    if (lazySync())
        m_updater->stop();

    return err;
}
#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbSandwichEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
#else
MojErr MojDbSandwichEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut)
#endif
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(!txnOut.get());

    MojRefCountedPtr<MojDbSandwichEnvTxn> txn(new MojDbSandwichEnvTxn(*this));
    MojAllocCheck(txn.get());
    txnOut = txn;

    return MojErrNone;
}

MojErr MojDbSandwichEngine::openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut)
{
    MojRefCountedPtr<MojDbStorageExtDatabase> db;
    MojErr err = openExtDatabase(name, txn, db);
    MojErrCheck(err);
    dbOut = db;
    return MojErrNone;
}

MojErr MojDbSandwichEngine::openExtDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtDatabase>& dbOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(name && !dbOut.get());

    BackendDb::Cookie cookie;

    leveldb::Status status = m_db.cook(name, cookie);
    MojLdbErrCheck(status, "openDatabase");

    MojRefCountedPtr<MojDbSandwichDatabase> db(new MojDbSandwichDatabase(m_db.use(cookie)));
    MojAllocCheck(db.get());

    MojErr err = db->open(name, this);
    MojErrCheck(err);

    dbOut = db;

    return MojErrNone;
}

MojErr MojDbSandwichEngine::mountShard(MojDbShardId shardId, const MojString& databasePath)
{
    mojo::SharedSandwich sandwich;
    sandwich.reset(new mojo::Sandwich {});
    auto &bottom = **sandwich;
    bottom.options = MojDbSandwichEngine::getOpenOptions();
    bottom.writeOptions = MojDbSandwichEngine::getWriteOptions();
    bottom.readOptions = MojDbSandwichEngine::getReadOptions();
    leveldb::Status status = bottom.Open(databasePath.data());

    if (status.IsCorruption()) {    // database corrupted
        // try to repair database (expect data loss)
        status = leveldb::RepairDB(databasePath.data(), MojDbSandwichEngine::getOpenOptions());
        MojLdbErrCheck(status, _T("db corrupted"));
        status = bottom.Open(databasePath.data());  // database restored, re-open
    }

    MojLdbErrCheck(status, _T("db_create/db_open"));

    auto emplaceInfo = m_sandwiches.emplace(shardId, std::move(sandwich));
    if (!emplaceInfo.second)
    {
        MojErrThrowMsg(MojErrDbInvalidShardId, "Shard %s already mounted",
                       std::to_string(shardId).c_str());
    }

    return MojErrNone;
}

MojErr MojDbSandwichEngine::unMountShard(MojDbShardId shardId)
{
    if (shardId == MojDbIdGenerator::MainShardId)
    {
        MojErrThrowMsg(MojErrDbInvalidShardId, "Can't unmount main shard %s",
                       std::to_string(shardId).c_str());
    }
    if (m_sandwiches.erase(shardId) == 0)
    {
        MojErrThrowMsg(MojErrDbInvalidShardId, "Shard %s wasn't mounted",
                       std::to_string(shardId).c_str());
    }
    return MojErrNone;
}


MojErr MojDbSandwichEngine::openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(name && !seqOut.get());
    MojAssert(m_seqDb.get());

    MojRefCountedPtr<MojDbSandwichSeq> seq(new MojDbSandwichSeq());
    MojAllocCheck(seq.get());


    MojErr err = seq->open(name, m_seqDb.get());
    MojErrCheck(err);
    seqOut = seq;
    m_seqs.push(seq);

    return MojErrNone;
}


// placeholder
MojErr MojDbSandwichEngine::compact()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojThreadGuard guard(m_dbMutex);

    m_bottom->CompactRange(nullptr, nullptr);

    return MojErrNone;
}

MojErr MojDbSandwichEngine::addSeq(MojDbSandwichSeq* seq)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(seq);

    MojThreadGuard guard(m_dbMutex);

    return m_seqs.push(seq);
}

MojErr MojDbSandwichEngine::removeSeq(MojDbSandwichSeq* seq)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(seq);
    MojThreadGuard guard(m_dbMutex);

    MojSize idx;
    MojSize size = m_seqs.size();
    for (idx = 0; idx < size; ++idx) {
        if (m_seqs.at(idx).get() == seq) {
            MojErr err = m_seqs.erase(idx);
            MojErrCheck(err);
            break;
        }
    }
    return MojErrNone;
}
