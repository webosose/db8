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

#include "core/MojLogDb8.h"
#include "engine/lmdb/MojDbLmdbEngine.h"
#include "engine/lmdb/MojDbLmdbFactory.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"
#include "engine/lmdb/MojDbLmdbEnv.h"
#include "engine/lmdb/MojDbLmdbSeq.h"
static const MojChar* const MojEnvIndexDbName = _T("indexes.mdb");
static const MojChar* const MojEnvSeqDbName = _T("seq.mdb");

MojDbLmdbEngine::MojDbLmdbEngine()
: m_isOpen(false)
{

}

MojDbLmdbEngine::~MojDbLmdbEngine()
{
	MojErr err = close();
	MojErrCatchAll(err);
}

MojErr MojDbLmdbEngine::configure(const MojObject& conf)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	return MojErrNone;
}

MojErr MojDbLmdbEngine::drop(const MojChar* path, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_isOpen);

	MojErr err;
	// close sequences
	for (SequenceVec::ConstIterator i = m_seqs.begin(); i != m_seqs.end(); ++i) {
		err = (*i)->close(txn);
		MojErrCheck(err);
	}
	m_seqs.clear();

	return MojErrNone;

}

MojErr MojDbLmdbEngine::open(const MojChar* path)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(path);
	MojAssert(!m_env.get() && !m_isOpen);
	MojRefCountedPtr<MojDbLmdbEnv> env(new MojDbLmdbEnv);
	MojAllocCheck(env.get());
	MojErr err = env->open(path);
	MojErrCheck(err);
	err = open(nullptr, env.get());
	MojErrCheck(err);
	err = m_path.assign(path);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbLmdbEngine::open(const MojChar* path, MojDbEnv* env)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbLmdbEnv* bEnv = static_cast<MojDbLmdbEnv *>(env);
	MojAssert(bEnv);
	MojAssert(!m_env.get() && !m_isOpen);
        MojErr err;
	m_env.reset(bEnv);
	if (path) {
		err = m_path.assign(path);
		MojErrCheck(err);
		// create dir
		err = MojCreateDirIfNotPresent(path);
		MojErrCheck(err);
	}

	MojRefCountedPtr<MojDbStorageTxn> txn;
	err = beginTxn(txn, true);
	MojErrCheck(err);
	// open seqence db
	m_seqDb.reset(new MojDbLmdbDatabase);
	MojAllocCheck(m_seqDb.get());
	err = m_seqDb->open(MojEnvSeqDbName, this, txn.get());
	MojErrCheck(err);
	// open index db
	m_indexDb.reset(new MojDbLmdbDatabase);
	MojAllocCheck(m_indexDb.get());
	err = m_indexDb->open(MojEnvIndexDbName, this, txn.get());
	MojErrCheck(err);
	m_isOpen = true;

	err = txn->commit();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbEngine::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = MojErrNone;
	MojErr errClose = MojErrNone;
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
MojErr MojDbLmdbEngine::openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(name && !dbOut.get());

	MojRefCountedPtr<MojDbLmdbDatabase> db(new MojDbLmdbDatabase());
	MojAllocCheck(db.get());
	MojErr err = db->open(name, this, txn);
	MojErrCheck(err);
	dbOut = db;
	return MojErrNone;
}
MojErr MojDbLmdbEngine::openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(name && !seqOut.get());
	MojRefCountedPtr<MojDbLmdbSeq> seq(new MojDbLmdbSeq());
	MojAllocCheck(seq.get());
	MojErr err = seq->open(name, txn, m_seqDb.get());
	MojErrCheck(err);
	seqOut = seq;
	return MojErrNone;
}
MojErr MojDbLmdbEngine::compact()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
        return MojErrNone;
}

MojErr MojDbLmdbEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!txnOut.get());

	MojRefCountedPtr<MojDbLmdbTxn> txn(new MojDbLmdbTxn());
	MojAllocCheck(txn.get());
	MojErr err = txn->begin(this, isWriteOp);
	MojErrCheck(err);
	txnOut = txn;
	return MojErrNone;
}

MojErr MojDbLmdbEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, bool isWriteOp,MojRefCountedPtr<MojDbStorageTxn>& txnOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!txnOut.get());
	MojRefCountedPtr<MojDbLmdbTxn> txn(new MojDbLmdbTxn());
	MojAllocCheck(txn.get());
	MojErr err = txn->begin(this, isWriteOp, txnParent.get());
	MojErrCheck(err);
	txnOut = txn;
	return MojErrNone;
}
MojErr MojDbLmdbEngine::addDatabase(MojDbLmdbDatabase* db)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(db);
	MojThreadGuard guard(m_dbMutex);
	return m_dbs.push(db);
}

MojErr MojDbLmdbEngine::removeDatabase(MojDbLmdbDatabase* db)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(db);
	MojThreadGuard guard(m_dbMutex);

	MojSize idx;
        MojErr err;
	MojSize size = m_dbs.size();
	for (idx = 0; idx < size; ++idx) {
		if (m_dbs.at(idx).get() == db) {
			err = m_dbs.erase(idx);
			MojErrCheck(err);
			break;
		}
	}
	return MojErrNone;
}

MojErr MojDbLmdbEngine::addSeq(MojDbLmdbSeq* db)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(db);
	MojThreadGuard guard(m_dbMutex);
	return m_seqs.push(db);
}

MojErr MojDbLmdbEngine::removeSeq(MojDbLmdbSeq* seq)
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

