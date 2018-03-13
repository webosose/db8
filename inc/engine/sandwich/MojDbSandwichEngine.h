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

#ifndef MOJDBLEVELENGINE_H_
#define MOJDBLEVELENGINE_H_

#include <leveldb/db.h>
#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"
#include "core/MojLogDb8.h"

#include "leveldb/sandwich_db.hpp"
#include "leveldb/bottom_db.hpp"
#include "leveldb/memory_db.hpp"
#include "leveldb/txn_db.hpp"

#include "pool.hpp"

class MojDbSandwichDatabase;
class MojDbSandwichEnv;
class MojDbSandwichSeq;
class MojDbSandwichLazyUpdater;

namespace mojo {
    // XXX: unfortunately C++ type-system quite weak to make some deduction of
    //      these types automatically

    // single shard
    typedef leveldb::BottomDB Bottom;
    typedef leveldb::SandwichDB<Bottom> Sandwich;
    typedef std::shared_ptr<Sandwich> SharedSandwich;
    typedef leveldb::SandwichDB<leveldb::TxnDB<Bottom>> SandwichTxn;

    // pool of shards
    typedef std::unordered_map<MojDbShardId, SharedSandwich> Sandwiches;
    typedef std::unordered_map<MojDbShardId, SandwichTxn> SandwichesTxn;
    typedef Pool<MojDbShardId, Sandwich::Part> PoolPart;
    typedef Pool<MojDbShardId, SandwichTxn::Part> PoolTxnPart;

    // create transaction (single/multi shard)
    inline SandwichTxn forkTxn(Sandwich &s) { return s.ref<leveldb::TxnDB>(); }
    inline SandwichesTxn forkTxn(Sandwiches &p)
    {
        SandwichesTxn result(p.bucket_count());
        for (auto &kv : p) result.emplace(kv.first, forkTxn(*kv.second));
        return result;
    }

    // use of specific part of sandwich
    inline Sandwich::Part use(Sandwich &s, const Sandwich::Cookie &c)
    { return s.use(c); }
    inline Sandwich::Part use(SharedSandwich &s, const Sandwich::Cookie &c)
    { return s->use(c); }
    inline SandwichTxn::Part use(SandwichTxn &s, const Sandwich::Cookie &c)
    { return s.use(c); }
    inline PoolPart use(Sandwiches &p, const Sandwich::Cookie &c)
    { return {p, [&c](SharedSandwich &s) { return use(s, c); }}; }
    inline PoolTxnPart use(SandwichesTxn &p, const Sandwich::Cookie &c)
    { return {p, [&c](SandwichTxn &s) { return use(s, c); }}; }
} // namespace mojo

class MojDbSandwichEngine final : public MojDbStorageEngine
{
public:
    typedef mojo::Sandwich BackendDb;

    MojDbSandwichEngine();
    ~MojDbSandwichEngine();

    // prepare sandwich cookie for specific database name
    MojErr cook(const leveldb::Slice &name, mojo::Sandwich::Cookie &cookie);

    // MojDbStorageEngine
    virtual MojErr configure(const MojObject& config);
    virtual MojErr drop(const MojChar* path, MojDbStorageTxn* txn);
    virtual MojErr open(const MojChar* path);
    virtual MojErr open(const MojChar* path, MojDbEnv* env);
    virtual MojErr close();
    virtual MojErr compact();
#ifdef LMDB_ENGINE_SUPPORT
    virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
#else
    virtual MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut);
#endif
    virtual MojErr openDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageDatabase>& dbOut) ;
    virtual MojErr openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut) ;
    MojErr openExtDatabase(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageExtDatabase>& dbOut) override;

    MojErr mountShard(MojDbShardId shardId, const MojString& databasePath) override;
    MojErr unMountShard(MojDbShardId shardId) override;

    const MojString& path() const { return m_path; }
    MojDbSandwichEnv* env() { return m_env.get(); }
    MojErr addSeq(MojDbSandwichSeq* seq);
    MojErr removeSeq(MojDbSandwichSeq* seq);

    MojDbSandwichDatabase* indexDb() { return m_indexDb.get(); }
    BackendDb& impl() {return m_db;}
    mojo::Sandwiches &sandwiches() { return m_sandwiches; }

    MojErr useShard(const mojo::Sandwich::Cookie &cookie, MojDbShardId shardId, mojo::Sandwich::Part &part)
    {
        auto it = m_sandwiches.find(shardId);
        if (it == m_sandwiches.end())
        {
            MojErrThrowMsg(MojErrDbInvalidShardId, "Shard %s not found", std::to_string(shardId).c_str());
        }
        part = mojo::use(*(it->second), cookie);
        return MojErrNone;
    }

    static const leveldb::WriteOptions& getWriteOptions() { return WriteOptions; }
    static const leveldb::ReadOptions& getReadOptions() { return ReadOptions; }
    static const leveldb::Options& getOpenOptions() { return OpenOptions; }

    MojDbSandwichLazyUpdater* getUpdater() const { return m_updater; }
    bool lazySync() const { return m_lazySync; }

private:
    typedef MojVector<MojRefCountedPtr<MojDbSandwichDatabase> > DatabaseVec;
    typedef MojVector<MojRefCountedPtr<MojDbSandwichSeq> > SequenceVec;

    mojo::Sandwiches m_sandwiches = {
        // pre-defined main shard
        { MojDbIdGenerator::MainShardId, std::make_shared<mojo::Sandwich>() }
    };
    mojo::Sandwich &m_db = *m_sandwiches[MojDbIdGenerator::MainShardId];  // reference main shard
    mojo::Bottom &m_bottom = *m_db; // refer to main shard bottom

    MojRefCountedPtr<MojDbSandwichEnv> m_env;
    MojThreadMutex m_dbMutex;
    MojRefCountedPtr<MojDbSandwichDatabase> m_indexDb;
    MojRefCountedPtr<MojDbSandwichDatabase> m_seqDb;
    MojString m_path;
    SequenceVec m_seqs;
    bool m_isOpen;

    static leveldb::ReadOptions ReadOptions;
    static leveldb::WriteOptions WriteOptions;
    static leveldb::Options OpenOptions;

    bool m_lazySync;
    MojDbSandwichLazyUpdater* m_updater;
};

#endif /* MOJDBLEVELENGINE_H_ */
