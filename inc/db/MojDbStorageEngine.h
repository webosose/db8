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


#ifndef MOJDBSTORAGEENGINE_H_
#define MOJDBSTORAGEENGINE_H_

#include <utility>

#include "db/MojDbDefs.h"
#include "db/MojDbWatcher.h"
#include "db/MojDbQuotaEngine.h"
#include "db/MojDbIdGenerator.h"
#include "core/MojAutoPtr.h"
#include "core/MojObject.h"
#include "core/MojVector.h"
#include "core/MojHashMap.h"
#include "core/MojSet.h"
#include "core/MojSignal.h"

class MojDbEnv : public MojRefCounted
{
  public:
    virtual ~MojDbEnv() {}
    virtual MojErr configure(const MojObject& conf) = 0;
    virtual MojErr open(const MojChar* path) = 0;
};

class MojDbStorageItem : public MojRefCounted
{
public:
	virtual ~MojDbStorageItem() {}
	virtual MojErr close() = 0;
	virtual MojErr kindId(MojString& kindIdOut, MojDbKindEngine& kindEngine) = 0;
	virtual MojErr visit(MojObjectVisitor& visitor, MojDbKindEngine& kindEngine, bool headerExpected = true) const = 0;
	virtual const MojObject& id() const = 0;
	virtual MojSize size() const = 0;

	MojErr toObject(MojObject& objOut, MojDbKindEngine& kindEngine, bool headerExpected = true) const;
	MojErr toJson(MojString& strOut, MojDbKindEngine& kindEngine) const;

protected:
	MojObject m_id;
};

class MojDbKindEngine;
class MojDbStorageQuery : public MojRefCounted
{
public:
	typedef MojVector<MojByte> ByteVec;
	typedef MojSet<MojString> StringSet;

	MojDbStorageQuery() {}
	virtual ~MojDbStorageQuery() {}
	virtual MojErr close() = 0;
	virtual MojErr get(MojDbStorageItem*& itemOut, bool& foundOut) = 0;
	virtual MojErr getId(MojObject& idOut, MojUInt32& groupOut, bool& foundOut) = 0;
	virtual MojErr getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut) = 0;
	virtual MojErr getById(const MojObject& id, MojObject& itemOut, bool& foundOut, MojDbKindEngine* kindEngine) { return MojErrNotImplemented; }
	virtual MojErr count(MojUInt32& countOut) = 0;
	virtual MojErr nextPage(MojDbQuery::Page& pageOut) = 0;
	virtual void excludeKinds(const StringSet& toExclude) { m_excludeKinds = toExclude; }
	virtual MojUInt32 groupCount() const = 0;
	const MojDbKey& endKey() const { return m_endKey; }
	StringSet& excludeKinds() { return m_excludeKinds; }
	bool verify() { return m_verify; }
	void verify(bool bVal) { m_verify = bVal; }

protected:
	MojDbKey m_endKey;
	StringSet m_excludeKinds;
	bool m_verify = false;

};

class MojDbStorageSeq : public MojRefCounted
{
public:
	virtual ~MojDbStorageSeq() {}
#ifdef LMDB_ENGINE_SUPPORT
	virtual MojErr close(MojDbStorageTxn* txn = nullptr) = 0;
	virtual MojErr get(MojInt64& valOut, MojDbStorageTxn* txn = nullptr) = 0;
#else
	virtual MojErr close() = 0;
	virtual MojErr get(MojInt64& valOut) = 0;
#endif
};

class MojDbStorageTxn : public MojSignalHandler
{
public:
	typedef MojVector<MojByte> ByteVec;
	typedef MojSignal<MojDbStorageTxn*> CommitSignal;

	// XXX: MojSignal doesn't allow multiple connections per slot
	class Monitor
	{
	public:
		virtual MojErr committed(MojDbStorageTxn&); //!< After transaction were committed
		virtual MojErr destroy(MojDbStorageTxn&); //!< About to destroy transaction
		// to monitor for abort we should remember if committed were called before destroy
	protected:
		virtual ~Monitor();
	};

	virtual ~MojDbStorageTxn();
	virtual MojErr abort() = 0;
    virtual bool isValid() = 0;
	MojErr commit();

	MojErr subscribe(Monitor&); // Monitor should outlive transaction subscription
	MojErr unsubscribe(Monitor&);

	MojErr offsetQuota(MojInt64 amount);
	void quotaEnabled(bool val) { m_quotaEnabled = val; }
	void refreshQuotas() { m_refreshQuotas = true; }

	void notifyPreCommit(CommitSignal::SlotRef slot);
	void notifyPostCommit(CommitSignal::SlotRef slot);

protected:
	MojDbStorageTxn();
	virtual MojErr commitImpl() = 0;

private:
	friend class MojDbQuotaEngine;

	bool m_quotaEnabled;
	bool m_refreshQuotas;
	MojDbQuotaEngine* m_quotaEngine;
	MojDbQuotaEngine::OffsetMap m_offsetMap;
	MojRefCountedPtr<MojDbQuotaEngine::Offset> m_curQuotaOffset;
	MojSet<Monitor*> m_monitors;
	CommitSignal m_preCommit;
	CommitSignal m_postCommit;
};

class MojDbStorageCollection : public MojRefCounted
{
public:
	virtual ~MojDbStorageCollection() {}
	virtual MojErr close() = 0;
	virtual MojErr drop(MojDbStorageTxn* txn) = 0;
	virtual MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut) = 0;
	virtual MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut) = 0;
#ifdef LMDB_ENGINE_SUPPORT
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false) = 0;
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, bool isWriteOp, MojRefCountedPtr<MojDbStorageTxn>& txnOut){
		return MojErrNone;
	}
#else
	virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut) = 0;
#endif
};

class MojDbStorageIndex : public MojDbStorageCollection
{
public:
	typedef MojVector<MojByte> ByteVec;

	virtual ~MojDbStorageIndex() {}
	virtual MojErr insert(const MojDbKey& key, MojDbStorageTxn* txn) = 0;
	virtual MojErr del(const MojDbKey& key, MojDbStorageTxn* txn) = 0;
};

class MojDbStorageDatabase : public MojDbStorageCollection
{
public:
	virtual ~MojDbStorageDatabase() {}
	virtual MojErr insert(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn) = 0;
	virtual MojErr update(const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn) = 0;
	virtual MojErr del(const MojObject& id, MojDbStorageTxn* txn, bool& foundOut) = 0;
	virtual MojErr get(const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut) = 0;
	virtual MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageIndex>& indexOut) = 0;
//hack:
	virtual MojErr mutexStats(int* total_mutexes, int* mutexes_free, int* mutexes_used, int* mutexes_used_highwater, int* mutexes_regionsize)
		{ if (total_mutexes) *total_mutexes = 0;
		  if (mutexes_free) *mutexes_free = 0;
		  if (mutexes_used) *mutexes_used = 0;
		  if (mutexes_used_highwater) *mutexes_used_highwater = 0;
		  if (mutexes_regionsize) *mutexes_regionsize = 0;
		  return MojErrNone;
		 }
};

class MojDbStorageExtIndex : public MojDbStorageIndex
{
public:
    virtual MojErr insert(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn) = 0;
    virtual MojErr del(MojDbShardId shardId, const MojDbKey& key, MojDbStorageTxn* txn) = 0;

    MojErr insert(const MojDbKey& key, MojDbStorageTxn* txn) final override
    { return insert(MojDbIdGenerator::MainShardId, key, txn); }
    MojErr del(const MojDbKey& key, MojDbStorageTxn* txn) final override
    { return del(MojDbIdGenerator::MainShardId, key, txn); }
};

class MojDbDummyExtIndex final : public MojDbStorageExtIndex
{
    MojRefCountedPtr<MojDbStorageIndex> m_index;
public:
    MojDbDummyExtIndex(const MojRefCountedPtr<MojDbStorageIndex> &index) : m_index(index)
    {}

    // MojDbStorageCollection
    MojErr close() override { return m_index->close(); }
    MojErr drop(MojDbStorageTxn* txn) override { return m_index->drop(txn); }
    MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut) override
    { return m_index->stats(txn, countOut, sizeOut); }
    MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut) override
    { return m_index->find(plan, txn, queryOut); }
#ifdef LMDB_ENGINE_SUPPORT
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false) override { return m_index->beginTxn(txnOut, isWriteOp); }
#else
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut) override { return m_index->beginTxn(txnOut); }
#endif
    // MojDbStorageExtIndex
    MojErr insert(MojDbShardId /* shardId */, const MojDbKey& key, MojDbStorageTxn* txn) override
    { return m_index->insert(key, txn); }
    MojErr del(MojDbShardId /* shardId */, const MojDbKey& key, MojDbStorageTxn* txn) override
    { return m_index->del(key, txn); }
};

class MojDbStorageExtDatabase : public MojDbStorageDatabase
{
public:
    virtual MojErr insert(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn) = 0;
    virtual MojErr update(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn) = 0;
    virtual MojErr del(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool& foundOut) = 0;
    virtual MojErr get(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut) = 0;
    virtual MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtIndex>& indexOut) = 0;

    MojErr insert(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn) final override
    { return insert(MojDbIdGenerator::MainShardId, id, val, txn); }
    MojErr update(const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn) final override
    { return update(MojDbIdGenerator::MainShardId, id, val, oldVal, txn); }
    MojErr del(const MojObject& id, MojDbStorageTxn* txn, bool& foundOut) final override
    { return del(MojDbIdGenerator::MainShardId, id, txn, foundOut); }
    MojErr get(const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut) final override
    { return get(MojDbIdGenerator::MainShardId, id, txn, forUpdate, itemOut); }
    MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageIndex>& indexOut) final override
    {
        MojRefCountedPtr<MojDbStorageExtIndex> index;
        MojErr err = openIndex(id, txn, index);
        MojErrCheck(err);
        indexOut = index;
        return MojErrNone;
    }
};

class MojDbDummyExtDatabase final : public MojDbStorageExtDatabase
{
    MojRefCountedPtr<MojDbStorageDatabase> m_db;
public:
    MojDbDummyExtDatabase(const MojRefCountedPtr<MojDbStorageDatabase> &db) : m_db(db)
    {}

    // MojDbStorageCollection
    MojErr close() override { return m_db->close(); }
    MojErr drop(MojDbStorageTxn* txn) override { return m_db->drop(txn); }
    MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut) override { return m_db->stats(txn, countOut, sizeOut); }
    MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut) override
    { return m_db->find(plan, txn, queryOut); }
#ifdef LMDB_ENGINE_SUPPORT
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false) override { return m_db->beginTxn(txnOut, isWriteOp); }
#else
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut) override { return m_db->beginTxn(txnOut); }
#endif
    // MojDbStorageExtDatabase
    MojErr insert(MojDbShardId /* shardId */, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn) override
    { return m_db->insert(id, val, txn); }
    MojErr update(MojDbShardId /* shardId */, const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn) override
    { return m_db->update(id, val, oldVal, txn); }
    MojErr del(MojDbShardId /* shardId */, const MojObject& id, MojDbStorageTxn* txn, bool& foundOut) override
    { return m_db->del(id, txn, foundOut); }
    MojErr get(MojDbShardId /* shardId */, const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut) override
    { return m_db->get(id, txn, forUpdate, itemOut); }
    MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtIndex>& indexOut) override
    {
        MojRefCountedPtr<MojDbStorageIndex> index;
        MojErr err = m_db->openIndex(id, txn, index);
        MojErrCheck(err);
        indexOut.reset(new MojDbDummyExtIndex(index));
        return MojErrNone;
    }
};

class MojDbStorageEngineFactory : public MojRefCounted
{
public:
    virtual ~MojDbStorageEngineFactory() {}
    virtual MojErr create(MojRefCountedPtr<MojDbStorageEngine>& engineOut) const = 0;
    virtual MojErr createEnv(MojRefCountedPtr<MojDbEnv>& envOut) const = 0;
    virtual const MojChar* name() const = 0;
};

class MojDbStorageEngine : public MojRefCounted
{
public:
    typedef MojRefCountedPtr<MojDbStorageEngineFactory> Factory;

    static MojErr createDefaultEngine(MojRefCountedPtr<MojDbStorageEngine>& engineOut);
    static MojErr createEngine(const MojChar* name, MojRefCountedPtr<MojDbStorageEngine>& engineOut);
    static MojErr createEnv(MojRefCountedPtr<MojDbEnv>& envOut);
    static MojErr setEngineFactory(MojDbStorageEngineFactory* factory);
    static MojErr setEngineFactory(const MojChar *name);
    static const MojDbStorageEngineFactory* engineFactory() {return m_factory.get();}

    virtual ~MojDbStorageEngine() {}
    virtual MojErr configure(const MojObject& config) = 0;
    virtual MojErr drop(const MojChar* path, MojDbStorageTxn* txn) = 0;
    virtual MojErr open(const MojChar* path) = 0;
    virtual MojErr open(const MojChar* path, MojDbEnv * env) = 0;
    virtual MojErr close() = 0;
    virtual MojErr compact() = 0;
#ifdef LMDB_ENGINE_SUPPORT
    virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false) = 0;
    virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, MojRefCountedPtr<MojDbStorageTxn>& txnOut) {
        return MojErrNone;
    }
#else
    virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut) = 0;
#endif
    virtual MojErr openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut) = 0;
    virtual MojErr openSequence(const MojChar* name, MojDbStorageTxn* txn,  MojRefCountedPtr<MojDbStorageSeq>& seqOut) = 0;
    virtual MojErr openExtDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtDatabase>& dbOut)
    {
        MojRefCountedPtr<MojDbStorageDatabase> db;
        MojErr err = openDatabase(name, txn, db);
        MojErrCheck(err);
        dbOut.reset(new MojDbDummyExtDatabase(db));
        return MojErrNone;
    }

    virtual MojErr mountShard(MojDbShardId shardId, const MojString& databasePath);
    virtual MojErr unMountShard(MojDbShardId shardId);

protected:
	MojDbStorageEngine();

	typedef MojHashMap<MojString, Factory> Factories;

private:
	static Factory m_factory;
	static Factories m_factories;

public:
	template <typename T>
	struct Registrator
	{
		Registrator()
		{
			Factory factory { new T() };
			MojAssert(factory.get());
			MojString key;
			if (key.assign(factory->name()) != MojErrNone) return; // ignore invalid name
			m_factories.put(key, factory);
		}
	};
};


#endif /* MOJDBSTORAGEENGINE_H_ */
