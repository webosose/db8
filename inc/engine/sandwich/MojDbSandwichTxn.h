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

#ifndef __MOJDBLEVELTXN_H
#define __MOJDBLEVELTXN_H

#include <map>
#include <set>
#include <list>
#include <string>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/txn_db.hpp>
#include <leveldb/bottom_db.hpp>

#include <core/MojString.h>
#include <core/MojErr.h>
#include <db/MojDbStorageEngine.h>
#include "MojDbSandwichEngine.h"

class MojDbSandwichEnvTxn final : public MojDbStorageTxn
{
public:
    typedef mojo::SandwichTxn BackendDb;

    MojDbSandwichEnvTxn(MojDbSandwichEngine& engine) :
        m_sandwiches(engine.sandwiches()), // copy and hold
        m_txn(mojo::forkTxn(m_sandwiches)), // ref local copy
        // m_txn(mojo::forkTxn(engine.sandwiches())),
        m_txnMain(m_txn.find(MojDbIdGenerator::MainShardId)->second),
        m_engine(engine)
    { }

    ~MojDbSandwichEnvTxn()
    { abort(); }

    MojErr abort() override;

    bool isValid() override
    { return true; }

    MojErr useShard(const mojo::Sandwich::Cookie &cookie, MojDbShardId shardId, mojo::SandwichTxn::Part &part)
    {
        auto it = m_txn.find(shardId);
        if (it == m_txn.end())
        {
            MojErrThrowMsg(MojErrDbInvalidShardId, "Shard %s not found", std::to_string(shardId).c_str());
        }
        part = mojo::use(it->second, cookie);
        return MojErrNone;
    }

    mojo::PoolTxnPart use(const mojo::Sandwich::Cookie &cookie)
    { return mojo::use(m_txn, cookie); }

private:
    MojErr commitImpl() override;

    mojo::Sandwiches m_sandwiches; // XXX: preserve shared pointers
    // FIXME: there is no TxnDB with strong refs in leveldb-tl 0.1.x
    //        but once we'll get them we can drop m_sandwiches
    mojo::SandwichesTxn m_txn;
    mojo::SandwichTxn &m_txnMain;
    MojDbSandwichEngine& m_engine;
};

#endif
