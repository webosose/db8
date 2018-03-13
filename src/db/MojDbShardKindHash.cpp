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

#include "db/MojDbShardKindHash.h"
#include "db/MojDb.h"
#include <db/MojDbKind.h>

const MojChar* const KindHash::KindJson =
        _T("{")
            _T("\"id\":\"KindHashMap:1\",\"owner\":\"com.palm.admin\",")
            _T("\"indexes\":[")
                _T("{\"name\":\"kindId\",\"props\":[{\"name\":\"kindId\"}]},")
                _T("{\"name\":\"hash\",  \"props\":[{\"name\":\"hash\"  }]}")
            _T("]")
        _T("}");

const MojChar* const KindHash::KindId        = _T("KindHashMap:1");
const MojChar* const KindHash::KindIdKey     = _T("_id");
const MojChar* const KindHash::KindKindIdKey = _T("kindId");
const MojChar* const KindHash::HashKey       = _T("hash");

const MojChar* const DummyObjectId      = _T("FFAAFFAA");
const MojChar* const DummyObjectKindId  = _T("KindHashMap:1");
const MojSize DummyObjectRevision       = 0;
const MojSize DummyObjectHash           = 0xFFAAFFAA;

inline MojInt64 convertHash(MojSize hash)  { return static_cast<MojInt64> (hash); }
inline MojSize  convertHash(MojInt64 hash) { return static_cast<MojSize> (hash); }

MojErr KindHash::toObject(MojObject* result) const
{
    MojAssert(result);
    MojErr err;

    err = result->putString(_T("_kind"), KindId);
    MojErrCheck(err);

    if (!m_id.empty()) {
        err = result->putString(KindIdKey, m_id);
        MojErrCheck(err);
    }

    err = result->putString(KindKindIdKey, m_kindId);
    MojErrCheck(err);

    err = result->putInt(HashKey, convertHash(m_hash));
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::fromObject(const MojObject& obj)
{
    MojErr err;

    err = obj.getRequired(KindIdKey, m_id);
    MojErrCheck(err);

    err = obj.getRequired(KindKindIdKey, m_kindId);
    MojErrCheck(err);

    MojInt64 hash;
    err = obj.getRequired(HashKey, hash);
    MojErrCheck(err);

    m_hash = convertHash(hash);

    return MojErrNone;
}

MojErr KindHash::fromKindObject(const MojObject& obj)
{
    MojErr err;

    err = obj.getRequired(_T("id"), m_kindId);
    MojErrCheck(err);

    MojInt64 hash;
    err = obj.getRequired(_T("hash"), hash);
    MojErrCheck(err);

    m_hash = convertHash(hash);

    return MojErrNone;
}

MojErr KindHash::fromKind(const MojDbKind& kind)
{
    m_kindId.assign(kind.id());
    m_hash = kind.hash();

    return MojErrNone;
}

MojErr KindHash::load(MojDb* db, const MojString& shardId, const MojString& kindId, bool* found, MojObject* result, MojDbReqRef req)
{
    MojAssert(db);
    MojAssert(found);
    MojAssert(result);

    MojErr err;
    MojDbQuery query;

    err = query.from(KindId);
    MojErrCheck(err);

    err = query.where(KindKindIdKey, MojDbQuery::OpEq, kindId);
    MojErrCheck(err);

    MojDbCursor cursor;

    err = db->find(query, cursor, req);
    MojErrCheck(err);

    err = cursor.get(*result, *found);
    MojErrCheck(err);

    err = cursor.close();
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::load(MojDb* db, const MojString& shardId, const MojString& kindId, bool* found, MojDbReqRef req)
{
    MojAssert(db);
    MojAssert(found);

    MojErr err;
    MojObject obj;

    err = load(db, shardId, kindId, found, &obj, req);
    MojErrCheck(err);

    if (!*found)
        return MojErrNone;

    err = fromObject(obj);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::save(MojDb* db, const MojString& shardId, MojDbReqRef req)
{
    MojAssert(db);

    MojErr err;
    MojObject obj;

    bool found;

    err = KindHash::load(db, shardId, m_kindId, &found, &obj, req);
    MojErrCheck(err);

    if (!found) {
        err = toObject(&obj);
        MojErrCheck(err);
    } else {
        // only update hash, no sense to touch any other fields
        err = obj.putInt(HashKey, convertHash (m_hash));
        MojErrCheck(err);
    }

    err = db->put(obj, MojDbFlagNone, req, shardId);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::del(MojDb* db, const MojString& shardId, MojDbReqRef req)
{
    MojAssert(db);
    MojAssert(!m_id.empty());

    MojErr err;
    bool found;

    err = db->del(m_id, found, MojDbFlagPurge, req);
    MojErrCheck(err);

    MojAssert(found);

    return MojErrNone;
}

MojErr KindHash::registerKind(MojDb* db, MojDbReqRef req)
{
    MojAssert(db);

    MojErr err;
    MojObject kindObj;

    err = kindObj.fromJson(KindJson);
    MojErrCheck(err);

    err = db->kindEngine()->putKind(kindObj, req, true);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::generateDummyObject(MojObject* object)
{
    MojAssert(object);

    MojErr err;

    KindHash dummyKindHash;
    err = dummyKindHash.m_id.assign(DummyObjectId);
    MojErrCheck(err);

    err = dummyKindHash.m_kindId.assign(DummyObjectKindId);
    MojErrCheck(err);

    dummyKindHash.m_hash = DummyObjectHash;

    err = dummyKindHash.toObject(object);
    MojErrCheck(err);

    // we always keep same object in database with same revision only for regenerating TokenSet
    err = object->putInt(_T("_rev"), DummyObjectRevision);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::isDummyObject(const MojObject& object, bool* isDummy)
{
    MojAssert(isDummy);

    MojErr err;

    MojString objectId;
    err = object.getRequired(_T("_id"), objectId);
    MojErrCheck(err);

    *isDummy = (objectId.compare(DummyObjectId) == 0) ? true : false;

    return MojErrNone;
}

MojErr KindHash::putTokenSet(MojDb* db, MojDbReqRef req)
{
    MojAssert(db);

    MojErr err;

    bool exists;
    err = dummyObjectExists(db, req, &exists);
    MojErrCheck(err);

    if (exists)
        return MojErrNone;

    MojObject dummyObject;
    err = generateDummyObject(&dummyObject);
    MojErrCheck(err);

    err = db->put(dummyObject, MojDbFlagNone, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::dropDummyObject(MojDb* db, MojDbReqRef req)
{
    MojAssert(db);

    MojErr err;

    MojString id;
    err = id.assign(DummyObjectId);
    MojErrCheck(err);

    bool found;
    err = db->del(id, found, MojDbFlagPurge, req);
    MojErrCheck(err);

    MojAssert(found);

    return MojErrNone;
}

MojErr KindHash::dummyObjectExists(MojDb* db, MojDbReqRef req, bool* found)
{
    MojAssert(found);

    MojErr err;
    MojObject dummyObj;
    MojString id;

    err = id.assign(DummyObjectId);
    MojErrCheck(err);

    err = db->get(id, dummyObj, *found, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr KindHash::loadHashes(MojDb* db, const MojUInt32 shardId, KindHashContainer* hashes, MojDbReqRef req)
{
    MojAssert(hashes);
    MojAssert(db);

    MojErr err;

    hashes->clear();

    err = putTokenSet(db, req);
    MojErrCheck(err);

    MojDbQuery query;
    err = query.from(KindHash::KindId);
    MojErrCheck(err);

    MojDbCursor cursor;
    err = db->find(query, cursor, req);
    MojErrCheck(err);

    while (true) {
        bool found;
        MojObject obj;

        err = cursor.get(obj, found);
        MojErrCheck(err);

        if (!found) break;

        bool isDummy;
        err = isDummyObject(obj, &isDummy);
        MojErrCheck(err);

        if (isDummy) continue;  // ignore dummy object

        KindHash kindHash;
        err = kindHash.fromObject(obj);
        MojErrCheck(err);

        hashes->push_back(kindHash);
    }

    return MojErrNone;
}
