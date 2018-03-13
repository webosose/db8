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


#ifndef MOJDBTESTSTORAGEENGINE_H_
#define MOJDBTESTSTORAGEENGINE_H_

#include "db/MojDbStorageEngine.h"
#include "core/MojHashMap.h"

class MojDbTestStorageEngine : public MojDbStorageEngine
{
public:
	typedef MojHashMap<MojString, MojErr, const MojChar*> ErrorMap;
	MojDbTestStorageEngine(MojDbStorageEngine* engine);
	virtual ~MojDbTestStorageEngine();

	virtual MojErr configure(const MojObject& conf);
	virtual MojErr drop(const MojChar* path, MojDbStorageTxn* txn);
	virtual MojErr open(const MojChar* path);
    virtual MojErr open(const MojChar* path, MojDbEnv* env);
	virtual MojErr close();
	virtual MojErr compact();
	virtual MojErr openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut);
	virtual MojErr openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut);
#ifdef LMDB_ENGINE_SUPPORT
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
#else
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut);
#endif

	ErrorMap& errMap() { return m_errMap; }
	MojDbStorageEngine* engine() { return m_engine.get(); }
	MojErr setNextError(const MojChar* methodName, MojErr err);
	MojErr checkErrMap(const MojChar* methodName);

private:
	MojRefCountedPtr<MojDbStorageEngine> m_engine;
	ErrorMap m_errMap;
};

class MojDbTestStorageIndex : public MojDbStorageExtIndex
{
public:
	MojDbTestStorageIndex(MojDbStorageExtIndex *index, MojDbTestStorageEngine* engine);
	virtual ~MojDbTestStorageIndex();

	virtual MojErr close();
	virtual MojErr drop(MojDbStorageTxn* txn);
	virtual MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut);
	virtual MojErr insert(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn);
	virtual MojErr del(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn);
	virtual MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut);
#ifdef LMDB_ENGINE_SUPPORT
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
#else
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut);
#endif
private:
	MojRefCountedPtr<MojDbStorageExtIndex> m_idx;
	MojDbTestStorageEngine* m_testEngine;
};

class MojDbTestStorageSeq : public MojDbStorageSeq
{
public:
	MojDbTestStorageSeq(MojDbStorageSeq* seq, MojDbTestStorageEngine* engine);
	~MojDbTestStorageSeq();
#ifdef LMDB_ENGINE_SUPPORT
	virtual MojErr close(MojDbStorageTxn *txn = nullptr);
	virtual MojErr get(MojInt64& valOut, MojDbStorageTxn* txn = nullptr);
#else
	virtual MojErr close();
	virtual MojErr get(MojInt64& valOut);
#endif

private:
	MojRefCountedPtr<MojDbStorageSeq> m_seq;
	MojDbTestStorageEngine* m_testEngine;
};

class MojDbTestStorageDatabase : public MojDbStorageExtDatabase
{
public:
	MojDbTestStorageDatabase(MojDbStorageExtDatabase *database, MojDbTestStorageEngine* engine);
	~MojDbTestStorageDatabase();
	virtual MojErr close();
	virtual MojErr drop(MojDbStorageTxn* txn) ;
	virtual MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut);
	virtual MojErr insert(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn);
	virtual MojErr update(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn);
	virtual MojErr del(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool& foundOut);
	virtual MojErr get(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut);
	virtual MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut);
	virtual MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtIndex>& indexOut);
#ifdef LMDB_ENGINE_SUPPORT
        virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
#else
        virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut);
#endif

private:
	MojRefCountedPtr<MojDbStorageExtDatabase> m_db;
	MojDbTestStorageEngine* m_testEngine;
};

class MojDbTestStorageTxn : public MojDbStorageTxn
{
public:
	MojDbTestStorageTxn(MojDbStorageTxn* txn, MojDbTestStorageEngine* engine);
	~MojDbTestStorageTxn();

	virtual MojErr abort();

	MojDbStorageTxn* txn() { return m_txn.get(); }
    bool isValid() { return true; }

private:
	virtual MojErr commitImpl();

	MojRefCountedPtr<MojDbStorageTxn> m_txn;
	MojDbTestStorageEngine* m_testEngine;
};

class MojDbTestStorageQuery : public MojDbStorageQuery
{
public:
	MojDbTestStorageQuery(MojDbStorageQuery* query, MojDbTestStorageEngine* engine);
	~MojDbTestStorageQuery();

	virtual MojErr close();
	virtual MojErr get(MojDbStorageItem*& itemOut, bool& foundOut);
	virtual MojErr getId(MojObject& objOut, MojUInt32& groupOut, bool& foundOut);
	virtual MojErr getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut);
	virtual MojErr getById(const MojObject& id, MojObject& itemOut, bool& foundOut, MojDbKindEngine* kindEngine);
	virtual MojErr count(MojUInt32& countOut);
	virtual MojErr nextPage(MojDbQuery::Page& pageOut);
	virtual MojUInt32 groupCount() const { return m_query->groupCount(); }
	virtual void excludeKinds(const StringSet& toExclude) { m_query->excludeKinds(toExclude); }

private:
	MojRefCountedPtr<MojDbStorageQuery> m_query;
	MojDbTestStorageEngine* m_testEngine;
};

#endif /* MOJDBTESTSTORAGEENGINE_H_ */
