// Copyright (c) 2009-2018 LG Electronics, Inc.
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
#include <sys/statvfs.h>


#include "engine/leveldb/MojDbLevelEngine.h"
#include "engine/leveldb/MojDbLevelFactory.h"
#include "engine/leveldb/MojDbLevelDatabase.h"
#include "engine/leveldb/MojDbLevelQuery.h"
#include "engine/leveldb/MojDbLevelSeq.h"
#include "engine/leveldb/MojDbLevelTxn.h"
#include "engine/leveldb/MojDbLevelEnv.h"

#include "db/MojDbObjectHeader.h"
#include "db/MojDbQueryPlan.h"
#include "db/MojDb.h"
#include "core/MojObjectBuilder.h"
#include "core/MojObjectSerialization.h"
#include "core/MojString.h"
#include "core/MojTokenSet.h"



//const MojChar* const MojDbLevelEnv::LockFileName = _T("_lock");
//db.ldb
static const MojChar* const MojEnvIndexDbName = _T("indexes.ldb");
static const MojChar* const MojEnvSeqDbName = _T("seq.ldb");

leveldb::ReadOptions MojDbLevelEngine::ReadOptions;
leveldb::WriteOptions MojDbLevelEngine::WriteOptions;
leveldb::Options MojDbLevelEngine::OpenOptions;


////////////////////MojDbLevelEngine////////////////////////////////////////////

MojDbLevelEngine::MojDbLevelEngine()
: m_isOpen(false)
{
}


MojDbLevelEngine::~MojDbLevelEngine()
{
    MojErr err =  close();
    MojErrCatchAll(err);
}

MojErr MojDbLevelEngine::configure(const MojObject& config)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (!config.get("sync", WriteOptions.sync)) {
        WriteOptions.sync = true;
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

    return MojErrNone;
}

// although we might have only 1 real Level DB - just to be safe and consistent
// provide interface to add/drop multiple DB
MojErr MojDbLevelEngine::drop(const MojChar* path, MojDbStorageTxn* txn)
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

    // drop databases
    for (DatabaseVec::ConstIterator i = m_dbs.begin(); i != m_dbs.end(); ++i) {
        MojErr err = (*i)->closeImpl();
        MojErrCheck(err);
        err = (*i)->drop(txn);
        MojErrCheck(err);
    }
    m_dbs.clear();

    return MojErrNone;
}
MojErr MojDbLevelEngine::open(const MojChar* path)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(path);
    MojAssert(!m_env.get() && !m_isOpen);

    // this is more like a placeholder
    MojRefCountedPtr<MojDbLevelEnv> env(new MojDbLevelEnv);
    MojAllocCheck(env.get());
    MojErr err = env->open(path);
    MojErrCheck(err);
    err = open(path, env.get());
    MojErrCheck(err);
    err = m_path.assign(path);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbLevelEngine::open(const MojChar* path, MojDbEnv* env)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojDbLevelEnv* bEnv = static_cast<MojDbLevelEnv *> (env);
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

    // open seqence db
    bool created = false;
    m_seqDb.reset(new MojDbLevelDatabase);
    MojAllocCheck(m_seqDb.get());
    MojErr err = m_seqDb->open(MojEnvSeqDbName, this, created, NULL);
    MojErrCheck(err);

    // open index db
    m_indexDb.reset(new MojDbLevelDatabase);
    MojAllocCheck(m_indexDb.get());
    err = m_indexDb->open(MojEnvIndexDbName, this, created, NULL);
    MojErrCheck(err);
    m_isOpen = true;

    return MojErrNone;
}

MojErr MojDbLevelEngine::close()
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

    return err;
}

#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbLevelEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
#else
MojErr MojDbLevelEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut)
#endif
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(!txnOut.get());

    MojRefCountedPtr<MojDbLevelEnvTxn> txn(new MojDbLevelEnvTxn());
    MojAllocCheck(txn.get());
    MojErr err = txn->begin(this);
    MojErrCheck(err);
    txnOut = txn;

    return MojErrNone;
}

MojErr MojDbLevelEngine::openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(name && !dbOut.get());

    MojRefCountedPtr<MojDbLevelDatabase> db(new MojDbLevelDatabase());
    MojAllocCheck(db.get());
    bool created = false;

    MojErr err = db->open(name, this, created, txn);
    MojErrCheck(err);

    if (m_dbs.find(db) == MojInvalidIndex) {
        m_dbs.push(db);

        return MojErrDbFatal;
    }

    dbOut = db;

    return MojErrNone;
}

MojErr MojDbLevelEngine::openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(name && !seqOut.get());
    MojAssert(m_seqDb.get());

    MojRefCountedPtr<MojDbLevelSeq> seq(new MojDbLevelSeq());
    MojAllocCheck(seq.get());


    MojErr err = seq->open(name, m_seqDb.get());
    MojErrCheck(err);
    seqOut = seq;
    m_seqs.push(seq);

    return MojErrNone;
}


// placeholder
MojErr MojDbLevelEngine::compact()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojThreadGuard guard(m_dbMutex);

    MojDbLevelDatabase* db;
    MojSize idx;
    MojSize size = m_dbs.size();
    for (idx = 0; idx < size; ++idx) {
        db = m_dbs[idx].get();
        MojAssert(db);
        db->compact();
    }

    return MojErrNone;
}
MojErr MojDbLevelEngine::addDatabase(MojDbLevelDatabase* db)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(db);
    MojThreadGuard guard(m_dbMutex);

    if (m_dbs.find(db) != MojInvalidIndex) {
        LOG_ERROR(MSGID_LEVEL_DB_ENGINE_ERROR, 0, "Database already in database pool");
        return MojErrDbFatal;
    }

    return m_dbs.push(db);
}

MojErr MojDbLevelEngine::removeDatabase(MojDbLevelDatabase* db)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(db);
    MojThreadGuard guard(m_dbMutex);

    MojSize idx;
    MojSize size = m_dbs.size();
    for (idx = 0; idx < size; ++idx) {
        if (m_dbs.at(idx).get() == db) {
            MojErr err = m_dbs.erase(idx);
            MojErrCheck(err);
            break;
        }
    }
    return MojErrNone;
}

MojErr MojDbLevelEngine::addSeq(MojDbLevelSeq* seq)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(seq);

    MojThreadGuard guard(m_dbMutex);

    return m_seqs.push(seq);
}

MojErr MojDbLevelEngine::removeSeq(MojDbLevelSeq* seq)
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
