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


#include "MojDbTestStorageEngine.h"

#define MojTestTxn(TXN) ((TXN) ? (static_cast<MojDbTestStorageTxn*>((TXN)))->txn() : NULL)

MojDbTestStorageEngine::MojDbTestStorageEngine(MojDbStorageEngine* engine)
: m_engine(engine)
{
}

MojDbTestStorageEngine::~MojDbTestStorageEngine()
{
}

MojErr MojDbTestStorageEngine::configure(const MojObject& conf)
{
	MojErr err = checkErrMap(_T("engine.configure"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	return m_engine->configure(conf);
}

MojErr MojDbTestStorageEngine::drop(const MojChar* path, MojDbStorageTxn* txn)
{
	MojErr err = checkErrMap(_T("engine.drop"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	return m_engine->drop(path, txn);
}

MojErr MojDbTestStorageEngine::open(const MojChar* path)
{
	MojErr err = checkErrMap(_T("engine.open"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	return m_engine->open(path);
}

MojErr MojDbTestStorageEngine::open(const MojChar* path, MojDbEnv* env)
{
    return open(path);
}

MojErr MojDbTestStorageEngine::close()
{
	MojErr err = checkErrMap(_T("engine.close"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	return m_engine->close();
}
#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbTestStorageEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
#else
MojErr MojDbTestStorageEngine::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut)
#endif
{
	MojErr err = checkErrMap(_T("engine.beginTxn"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	MojAssert(!txnOut.get());

	//create the real transaction
	MojRefCountedPtr<MojDbStorageTxn> realTxn;
#ifdef LMDB_ENGINE_SUPPORT
	err = m_engine->beginTxn(realTxn, isWriteOp);
#else
	err = m_engine->beginTxn(realTxn);
#endif
	MojErrCheck(err);

	//create a test txn wrapper around the real txn
	MojRefCountedPtr<MojDbTestStorageTxn> txn(new MojDbTestStorageTxn(realTxn.get(), this));
	MojAllocCheck(txn.get());
	txnOut = txn;

	return MojErrNone;
}

MojErr MojDbTestStorageEngine::openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut)
{
	MojErr err = checkErrMap(_T("engine.openDatabase"));
	MojErrCheck(err);

	MojAssert(name && !dbOut.get());
	//open the actual storage database
	MojAssert(m_engine.get());
	MojRefCountedPtr<MojDbStorageExtDatabase> realDb;
	err = m_engine->openExtDatabase(name, MojTestTxn(txn), realDb);
	MojErrCheck(err);

	//create the test storage db as a wrapper
	MojRefCountedPtr<MojDbTestStorageDatabase> db(new MojDbTestStorageDatabase(realDb.get(), this));
	MojAllocCheck(db.get());
	dbOut = db;

	return MojErrNone;
}

MojErr MojDbTestStorageEngine::openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut)
{
	MojErr err = checkErrMap(_T("engine.openSequence"));
	MojErrCheck(err);

	MojAssert(name && !seqOut.get());
	//open the actual storage sequence
	MojAssert(m_engine.get());
	MojRefCountedPtr<MojDbStorageSeq> realSeq;
	err = m_engine->openSequence(name, MojTestTxn(txn), realSeq);
	MojErrCheck(err);

	//create the test storage seq as a wrapper
	MojRefCountedPtr<MojDbTestStorageSeq> seq(new MojDbTestStorageSeq(realSeq.get(), this));
	MojAllocCheck(seq.get());
	seqOut = seq;

	return MojErrNone;
}

MojErr MojDbTestStorageEngine::compact()
{
	MojErr err = checkErrMap(_T("engine.compact"));
	MojErrCheck(err);

	MojAssert(m_engine.get());
	return m_engine->compact();
}

MojErr MojDbTestStorageEngine::setNextError(const MojChar* methodName, MojErr err)
{
	if (err == MojErrNone) {
		bool found;
		MojErr errDel = m_errMap.del(methodName, found);
		MojErrCheck(errDel);
	} else {
		MojString str;
		str.assign(methodName);
		MojErr errPut = m_errMap.put(str, err);
		MojErrCheck(errPut);
	}

	return MojErrNone;
}

MojErr MojDbTestStorageEngine::checkErrMap(const MojChar* methodName)
{
	ErrorMap::ConstIterator i = m_errMap.find(methodName);
	if (i != m_errMap.end()) {
		MojErr methodErr = *i;
		bool found;
		MojErr err = m_errMap.del(methodName, found);
		MojErrCheck(err);
		return methodErr;
	}

	return MojErrNone;
}


MojDbTestStorageDatabase::MojDbTestStorageDatabase(MojDbStorageExtDatabase* database, MojDbTestStorageEngine* engine)
: m_db(database),
  m_testEngine(engine)
{
}

MojDbTestStorageDatabase::~MojDbTestStorageDatabase()
{
}

MojErr MojDbTestStorageDatabase::close()
{
	MojErr err = m_testEngine->checkErrMap(_T("db.close"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->close();
}

MojErr MojDbTestStorageDatabase::drop(MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.drop"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->drop(MojTestTxn(txn));
}

MojErr MojDbTestStorageDatabase::stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.size"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->stats(MojTestTxn(txn), countOut, sizeOut);
}

MojErr MojDbTestStorageDatabase::insert(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.insert"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->insert(shardId, id, val, MojTestTxn(txn));
}

MojErr MojDbTestStorageDatabase::update(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.update"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->update(shardId, id, val, oldVal, MojTestTxn(txn));
}

MojErr MojDbTestStorageDatabase::del(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool& foundOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.del"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->del(shardId, id, MojTestTxn(txn), foundOut);
}

MojErr MojDbTestStorageDatabase::get(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.get"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->get(shardId, id, MojTestTxn(txn), forUpdate, itemOut);
}

MojErr MojDbTestStorageDatabase::find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.find"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	return m_db->find(plan, MojTestTxn(txn), queryOut);
}

#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbTestStorageDatabase::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
#else
MojErr MojDbTestStorageDatabase::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut)
#endif
{
	MojErr err = m_testEngine->checkErrMap(_T("db.beginTxn"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	MojAssert(!txnOut.get());

	//create the real transaction
	MojRefCountedPtr<MojDbStorageTxn> realTxn;
	err = m_db->beginTxn(realTxn);
	MojErrCheck(err);

	//create a test txn wrapper around the real txn
	MojRefCountedPtr<MojDbTestStorageTxn> txn(new MojDbTestStorageTxn(realTxn.get(), m_testEngine));
	MojAllocCheck(txn.get());
	txnOut = txn;

	return MojErrNone;
}

MojErr MojDbTestStorageDatabase::openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtIndex>& indexOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("db.openIndex"));
	MojErrCheck(err);

	MojAssert(m_db.get());
	MojAssert(!indexOut.get());

	//open the actual storage index
	MojRefCountedPtr<MojDbStorageExtIndex> realIdx;
	err = m_db->openIndex(id, MojTestTxn(txn), realIdx);
	MojErrCheck(err);

	//create the test storage index as a wrapper
	MojRefCountedPtr<MojDbTestStorageIndex> index(new MojDbTestStorageIndex(realIdx.get(), m_testEngine));
	MojAllocCheck(index.get());
	indexOut = index;

	return MojErrNone;
}


MojDbTestStorageIndex::MojDbTestStorageIndex(MojDbStorageExtIndex* index, MojDbTestStorageEngine* engine)
: m_idx(index),
  m_testEngine(engine)
{
}

MojDbTestStorageIndex::~MojDbTestStorageIndex()
{
}

MojErr MojDbTestStorageIndex::close()
{
	MojErr err = m_testEngine->checkErrMap(_T("index.close"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	return m_idx->close();
}

MojErr MojDbTestStorageIndex::drop(MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("index.drop"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	return m_idx->drop(MojTestTxn(txn));
}

MojErr MojDbTestStorageIndex::stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("index.size"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	return m_idx->stats(MojTestTxn(txn), countOut, sizeOut);
}

MojErr MojDbTestStorageIndex::insert(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("index.insert"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	return m_idx->insert(shardId, key, MojTestTxn(txn));
}

MojErr MojDbTestStorageIndex::del(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn)
{
	MojErr err = m_testEngine->checkErrMap(_T("index.del"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	return m_idx->del(shardId, key, MojTestTxn(txn));
}

MojErr MojDbTestStorageIndex::find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("index.find"));
	MojErrCheck(err);

	MojAssert(m_idx.get());
	MojAssert(plan.get());

	//create the real query
	MojRefCountedPtr<MojDbStorageQuery> realQuery;
	err = m_idx->find(plan, MojTestTxn(txn), realQuery);
	MojErrCheck(err);

	MojRefCountedPtr<MojDbTestStorageQuery> storageQuery(new MojDbTestStorageQuery(realQuery.get(), m_testEngine));
	MojAllocCheck(storageQuery.get());
	queryOut = storageQuery;

	return MojErrNone;
}

#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbTestStorageIndex::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
#else
MojErr MojDbTestStorageIndex::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut)
#endif
{
	MojErr err = m_testEngine->checkErrMap(_T("index.beginTxn"));
	MojErrCheck(err);

	MojAssert(m_idx.get());

	//create the real transaction
	MojRefCountedPtr<MojDbStorageTxn> realTxn;
	err = m_idx->beginTxn(realTxn);
	MojErrCheck(err);

	//create a test txn wrapper around the real txn
	MojRefCountedPtr<MojDbTestStorageTxn> txn(new MojDbTestStorageTxn(realTxn.get(), m_testEngine));
	MojAllocCheck(txn.get());
	txnOut = txn;

	return MojErrNone;
}

MojDbTestStorageTxn::MojDbTestStorageTxn(MojDbStorageTxn* txn, MojDbTestStorageEngine* engine)
: m_txn(txn),
  m_testEngine(engine)
{
}

MojDbTestStorageTxn::~MojDbTestStorageTxn()
{
}

MojErr MojDbTestStorageTxn::commitImpl()
{
	MojErr err = m_testEngine->checkErrMap(_T("txn.commit"));
	if (err != MojErrNone) {
		MojErr errAbort = m_txn->abort();
		MojErrCheck(errAbort);
		return err;
	}

	MojAssert(m_txn.get());
	err = m_txn->commit();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbTestStorageTxn::abort()
{
	MojErr err = m_testEngine->checkErrMap(_T("txn.abort"));
	MojErrCheck(err);

	MojAssert(m_txn.get());
	return m_txn->abort();
}

MojDbTestStorageQuery::MojDbTestStorageQuery(MojDbStorageQuery* query, MojDbTestStorageEngine* engine)
: m_query(query),
  m_testEngine(engine)
{
}

MojDbTestStorageQuery::~MojDbTestStorageQuery()
{

}

MojErr MojDbTestStorageQuery::close()
{
	MojErr err = m_testEngine->checkErrMap(_T("query.close"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->close();
}

MojErr MojDbTestStorageQuery::get(MojDbStorageItem*& itemOut, bool& foundOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.get"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->get(itemOut, foundOut);
}

MojErr MojDbTestStorageQuery::getId(MojObject& objOut, MojUInt32& groupOut, bool& foundOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.getId"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->getId(objOut, groupOut, foundOut);
}

MojErr MojDbTestStorageQuery::getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.getById"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->getById(id, itemOut, foundOut);
}

MojErr MojDbTestStorageQuery::getById(const MojObject& id, MojObject& itemOut, bool& foundOut, MojDbKindEngine* kindEngine)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.getById"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->getById(id, itemOut, foundOut, kindEngine);
}

MojErr MojDbTestStorageQuery::count(MojUInt32& countOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.count"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->count(countOut);
}

MojErr MojDbTestStorageQuery::nextPage(MojDbQuery::Page& pageOut)
{
	MojErr err = m_testEngine->checkErrMap(_T("query.nextPage"));
	MojErrCheck(err);

	MojAssert(m_query.get());
	return m_query->nextPage(pageOut);
}

MojDbTestStorageSeq::MojDbTestStorageSeq(MojDbStorageSeq* seq, MojDbTestStorageEngine* engine)
: m_seq(seq),
  m_testEngine(engine)
{
}

MojDbTestStorageSeq::~MojDbTestStorageSeq()
{
}

#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbTestStorageSeq::close(MojDbStorageTxn *txn)
#else
MojErr MojDbTestStorageSeq::close()
#endif
{
	MojErr err = m_testEngine->checkErrMap(_T("seq.close"));
	MojErrCheck(err);

	MojAssert(m_seq.get());
	return m_seq->close();
}

#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbTestStorageSeq::get(MojInt64& valOut, MojDbStorageTxn* txn)
#else
MojErr MojDbTestStorageSeq::get(MojInt64& valOut)
#endif
{
	MojErr err = m_testEngine->checkErrMap(_T("seq.get"));
	MojErrCheck(err);

	MojAssert(m_seq.get());
#ifdef LMDB_ENGINE_SUPPORT
	return m_seq->get(valOut, MojTestTxn(txn));
#else
	return m_seq->get(valOut);
#endif
}
