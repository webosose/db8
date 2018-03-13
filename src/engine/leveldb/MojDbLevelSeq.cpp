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

#include "engine/leveldb/MojDbLevelSeq.h"
#include "engine/leveldb/MojDbLevelDatabase.h"
#include "engine/leveldb/MojDbLevelEngine.h"

namespace {
    constexpr MojInt64 SequenceAllocationPage = 100;
} // anonymous namespace

// at this point it's just a placeholder
MojDbLevelSeq::~MojDbLevelSeq()
{
    if (m_db) {
        MojErr err = close();
        MojErrCatchAll(err);
    }
}

MojErr MojDbLevelSeq::open(const MojChar* name, MojDbLevelDatabase* db)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(db);

    //MojAssert(!m_seq);
    MojErr err;

    m_db = db;
    err = m_key.fromBytes(reinterpret_cast<const MojByte  *>(name), strlen(name));
    MojErrCheck(err);

    MojDbLevelItem val;
    bool found = false;
    err = m_db->get(m_key, NULL, false, val, found);
    MojErrCheck(err);

    if (found)
    {
        MojObject valObj;
        err = val.toObject(valObj);
        MojErrCheck(err);
        m_next = valObj.intValue();
    }
    else
    {
        m_next = 0;
    }

    m_allocated.store(m_next, std::memory_order_relaxed);

    return MojErrNone;
}
#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbLevelSeq::close(MojDbStorageTxn* txn)
#else
MojErr MojDbLevelSeq::close()
#endif
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (m_db) {
        MojErr err = store(m_next);
        MojErrCheck(err);
        m_db = NULL;
    }
    return MojErrNone;
}
#ifdef LMDB_ENGINE_SUPPORT
MojErr MojDbLevelSeq::get(MojInt64& valOut, MojDbStorageTxn* txn)
#else
MojErr MojDbLevelSeq::get(MojInt64& valOut)
#endif
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

#ifdef MOJ_DEBUG
    valOut = -1; // in case of error trace down those who do not check MojErr
#endif

    auto val = m_next.fetch_add(1, std::memory_order_relaxed); // only attomicity of increment required

    // the only stuff we need here is atomicity of load()
    // we are ok if some concurrent get() will fallback to mutex due due to
    // read of outdated value
    if (val >= m_allocated.load(std::memory_order_relaxed))
    {
        // everyone who received val from unallocated range will come here and
        // will be processed in a queue on this mutex
        std::lock_guard<std::mutex> allocationGuard(m_allocationLock);

        // re-read under lock before allocating more sequence values
        auto allocated = m_allocated.load(std::memory_order_relaxed);
        if (val >= allocated) // no other threads who marked our value allocated
        {
            // mark our allocated val as a start of next pre-allocated page
            MojErr err = store(val + SequenceAllocationPage);
            MojErrCheck(err);
        }
    }
    valOut = (MojInt64)val;

    return MojErrNone;
}

MojErr MojDbLevelSeq::store(MojUInt64 next)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert( next >= m_next );
    MojErr err;
    MojDbLevelItem val;

#ifdef MOJ_DEBUG
    // ensure that our state is consistent with db
    bool found = false;
    err = m_db->get(m_key, NULL, false, val, found);
    MojErrCheck(err);

    if (found)
    {
        MojObject valObj;
        err = val.toObject(valObj);
        MojErrCheck(err);
        if (m_allocated != MojUInt64(valObj.intValue()))
            MojErrThrowMsg(MojErrDbInconsistentIndex, "Stored and cached seq are different");
    }
    else
    {
        if (m_allocated != 0)
            MojErrThrowMsg(MojErrDbInconsistentIndex, "Failed to fetch previously allocated seq");
    }

#endif

    err = val.fromObject(MojInt64(next));
    MojErrCheck(err);
    err = m_db->put(m_key, val, NULL, false);
    MojErrCheck(err);
    m_allocated.store(next, std::memory_order_relaxed);

    return MojErrNone;
}


