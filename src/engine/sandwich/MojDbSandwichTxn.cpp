// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#include "engine/sandwich/MojDbSandwichEngine.h"
#include "engine/sandwich/MojDbSandwichTxn.h"
#include "engine/sandwich/defs.h"
#include "engine/sandwich/MojDbSandwichLazyUpdater.h"
#include <vector>
// class MojDbSandwichEnvTxn
MojErr MojDbSandwichEnvTxn::abort()
{
    // Note creation of databases will not be rolled back

    // first we rollback our main shard and then rest of them
    m_txnMain->reset();
    for (auto &shard : m_txn) shard.second->reset();
    return MojErrNone;
}

MojErr MojDbSandwichEnvTxn::commitImpl()
{
    std::vector<leveldb::Status> statuses;

    // to ensure consistency of main shard we'll commit it in the last turn
    for (auto &shard : m_txn)
    {
        leveldb::Status s;
        if (shard.first == MojDbIdGenerator::MainShardId) continue; // delay main shard commit
        s = shard.second->commit();
        if (!s.ok()) statuses.push_back(s);
    }
    // at this moment all shards except of main are committed
    leveldb::Status s = m_txnMain->commit();
    MojLdbErrCheck(s, _T("m_txnMain->commit"));

    for (auto &status : statuses)
        MojLdbErrCheck(status, _T("shard.second->commit"));

    if (m_engine.lazySync())
        m_engine.getUpdater()->sendEvent( (*m_engine.impl()).get() );

    return MojErrNone;
}
