// Copyright (c) 2013-2021 LG Electronics, Inc.
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

#ifndef MOJDBLEVELDATABASE_H
#define MOJDBLEVELDATABASE_H

#include <leveldb/db.h>
#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"
#include "MojDbSandwichEngine.h"

class MojDbSandwichEngine;
class MojDbSandwichItem;
class MojDbSandwichEnvTxn;

class MojDbSandwichDatabase final : public MojDbStorageExtDatabase
{
public:
    MojDbSandwichDatabase(const MojDbSandwichEngine::BackendDb::Part& part) :
        m_db(part), m_engine(nullptr)
    {}
    ~MojDbSandwichDatabase();

    MojErr open(const MojChar* dbName, MojDbSandwichEngine* env);
    MojErr close() override;
    MojErr drop(MojDbStorageTxn* txn) override;
    MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut) override;
    MojErr insert(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn) override;
    MojErr update(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn) override;
    MojErr del(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool& foundOut) override;
    MojErr get(MojDbShardId shardId, const MojObject& id, MojDbStorageTxn* txn, bool forUpdate, MojRefCountedPtr<MojDbStorageItem>& itemOut) override;
    MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut) override;
#ifdef LMDB_ENGINE_SUPPORT
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false) override;
#else
    MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut) override;
#endif
    MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtIndex>& indexOut) override;

//hack:
    MojErr mutexStats(int* total_mutexes, int* mutexes_free, int* mutexes_used, int* mutexes_used_highwater, int* mutex_regionsize) override;

    MojErr put(MojDbShardId shardId, const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn, bool updateIdQuota);
    MojErr put(MojDbShardId shardId, MojDbSandwichItem& key, MojDbSandwichItem& val, MojDbStorageTxn* txn, bool updateIdQuota);
    MojErr del(MojDbShardId shardId, MojDbSandwichItem& key, bool& foundOut, MojDbStorageTxn* txn);
    MojErr get(MojDbShardId shardId, MojDbSandwichItem& key, MojDbStorageTxn* txn, bool forUpdate, MojDbSandwichItem& valOut, bool& foundOut);
    void   compact();

    MojErr delPrefix(MojDbSandwichEnvTxn &txn, leveldb::Slice prefix = {});
    MojErr stats(MojDbSandwichEnvTxn* txn, MojSize &countOut, MojSize &sizeOut, leveldb::Slice prefix);

    bool valid() const { return m_name; }
    MojDbSandwichEngine::BackendDb::Part& impl() { return m_db; }
    MojDbSandwichEngine* engine() { return m_engine; }

    leveldb::DB* getDb();

private:
    friend class MojDbSandwichEngine;
    friend class MojDbSandwichIndex;

    //MojErr verify();
    MojErr closeImpl();
    void postUpdate(MojDbStorageTxn* txn, MojSize updateSize);

    mojo::Sandwich::Part m_db;
    mojo::Sandwich::Cookie m_cookie;
    MojDbSandwichEngine* m_engine;
    MojString m_name;
};

#endif
