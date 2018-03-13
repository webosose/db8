// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#pragma once

#include "core/MojString.h"
#include "core/MojVector.h"
#include "core/MojObject.h"
#include "db/MojDbDefs.h"
#include "db/MojDbReq.h"
#include <vector>

class KindHash
{
    static const MojChar* const KindJson;
    static const MojChar* const KindId;
    static const MojChar* const KindIdKey;
    static const MojChar* const KindKindIdKey;
    static const MojChar* const HashKey;

public:
    typedef std::vector<KindHash> KindHashContainer;

    const MojString& id() const { return m_id; }
    const MojString& kindId() const { return m_kindId; }
    MojSize hash() const { return m_hash; }

    MojErr toObject(MojObject* result) const;
    MojErr fromObject(const MojObject& obj);

    MojErr fromKindObject(const MojObject& obj);
    MojErr fromKind(const MojDbKind& kind);

    MojErr load(MojDb* db, const MojString& shardId, const MojString& kindId, bool* found, MojDbReqRef req);
    MojErr save(MojDb* db, const MojString& shardId, MojDbReqRef req);
    MojErr del(MojDb* db, const MojString& shardId, MojDbReqRef req);

    static MojErr registerKind(MojDb* db, MojDbReqRef req);
    static MojErr loadHashes(MojDb* db, const MojUInt32 shardId, KindHashContainer* hashes, MojDbReqRef req);
private:
    static MojErr putTokenSet(MojDb* db, MojDbReqRef req);
    static MojErr load(MojDb* db, const MojString& shardId, const MojString& kindId, bool* found, MojObject* result, MojDbReqRef req);
    static MojErr generateDummyObject(MojObject* object);
    static MojErr isDummyObject(const MojObject& object, bool* isDummy);
    static MojErr dummyObjectExists(MojDb* db, MojDbReqRef req, bool* isExist);
    static MojErr dropDummyObject(MojDb* db, MojDbReqRef req);

    MojString m_id;
    MojString m_kindId;
    MojSize m_hash;
};
