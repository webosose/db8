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


#include "MojDbShardManagerTest.h"
#include "db/MojDb.h"
#include "db/MojDbKind.h"
#include "db/MojDbSearchCursor.h"
#include "core/MojObjectBuilder.h"

#include <iostream>
#include <set>

#define SHARDID_CACHE_SIZE  100
#define SHARD_ITEMS_NUMBER  10

namespace {
/**
 * Test description
 *
 * This test is cover functionality of two classes: MojDbShardIdCache and MojDbShardEngine.
 *
 * test scenario for MojDbShardIdCache:
 * ------------------------------------
 * - generate a new id's (=SHARDID_CASH_SIZE);
 * - put all new generated id's to cache;
 * - read id's from the cache and compaire them with local copies;
 * - check for existance of never been stored id.
 *
 */

static const MojChar* const TestShardKind1Str =
    _T("{\"id\":\"TestShard1:1\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[ {\"name\":\"RecId\",  \"props\":[ {\"name\":\"recId\"} ]}, \
                    ]}");

static const MojChar* const TestShardKind2Str =
    _T("{\"id\":\"TestShard2:1\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[ {\"name\":\"RecId\",  \"props\":[ {\"name\":\"recId\"} ]}, \
                    ]}");

static const MojChar* const TestShardKind3Str =
    _T("{\"id\":\"TestShard3:1\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[ {\"name\":\"RecId\",  \"props\":[ {\"name\":\"recId\"} ]}, \
                    ]}");

const MojUInt32 MagicId = 42u;

}


MojDbShardManagerTest::MojDbShardManagerTest()
    : MojTestCase(_T("MojDbShardManager"))
{
    // initialize own copy of _id generator
    idGen.init();
}

/**
 * run
 */
MojErr MojDbShardManagerTest::run()
{
    MojErr err = MojErrNone;
    MojDb db;

    // open
    err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    MojDbShardEngine* p_eng = db.shardEngine();
    MojDbShardIdCache cache;
    err = testShardIdCacheIndexes(&cache);
    MojTestErrCheck(err);
    err = testShardIdCacheOperations(&cache);
    MojTestErrCheck(err);
    err = testShardEngine(p_eng);
    MojTestErrCheck(err);

    err = db.close();
    MojTestErrCheck(err);

    cleanup();
    err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    p_eng = db.shardEngine();

    err = testShardCreateAndRemoveWithRecords(db);
    MojTestErrCheck(err);

    err = db.close();
    MojTestErrCheck(err);

    cleanup();
    err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    p_eng = db.shardEngine();

    err = testShardProcessing(db);
    MojTestErrCheck(err);

    err = db.close();
    MojTestErrCheck(err);

    cleanup();
    err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    p_eng = db.shardEngine();

    err = testShardTransient(db);
    MojTestErrCheck(err);

    err = db.close();
    MojTestErrCheck(err);

    return err;
}

MojErr MojDbShardManagerTest::fillData(MojDb& db, MojUInt32 shardId)
{
    // add objects
    for(int i = 1; i <= 10; ++i)
    {
        MojObject obj;
        MojTestErrCheck( obj.putString("_kind", "TestShard1:1") );
        MojTestErrCheck( obj.put("foo", i) );
        MojTestErrCheck( obj.put("bar", i/4) );

        if (i % 2) MojTestErrCheck( put(db, obj, shardId) );
        else MojTestErrCheck( put(db, obj) );
    }
    return MojErrNone;
}

MojErr MojDbShardManagerTest::put(MojDb& db, MojObject &obj, MojUInt32 shardId)
{
    MojErr err;
    /* XXX: once shard id will be added to Put interface (GF-11469)
     *  MojAssertNoErr( db.put("_shardId", MagicId) );
     */

    // work-around with pre-generated _id
    MojObject id;
    err = idGen.id(id, shardId);
    MojTestErrCheck( err );

    err = obj.put("_id", id);
    MojTestErrCheck( err );

    return db.put(obj);
}

MojErr MojDbShardManagerTest::expect(MojDb& db, const MojChar* expectedJson, bool includeInactive)
{
    MojDbQuery query;
    query.clear();
    MojTestErrCheck( query.from(_T("TestShard1:1")) );
    if (includeInactive) query.setIgnoreInactiveShards( false );

    MojString str;
    MojDbSearchCursor cursor(str);
    MojTestErrCheck( db.find(query, cursor) );

    MojObjectBuilder builder;
    MojTestErrCheck( builder.beginArray() );
    MojTestErrCheck( cursor.visit(builder) );
    MojTestErrCheck( cursor.close() );
    MojTestErrCheck( builder.endArray() );
    MojObject results = builder.object();

    MojString json;
    MojTestErrCheck( results.toJson(json) );

    MojObject expected;
    MojTestErrCheck( expected.fromJson(expectedJson) );

    MojAssert( expected.size() == results.size() ); // "Amount of expected records should match amount of records in result";
    MojObject::ConstArrayIterator j = results.arrayBegin();
    size_t n = 0;
    for (MojObject::ConstArrayIterator i = expected.arrayBegin(); j != results.arrayEnd() && i != expected.arrayEnd(); ++i, ++j, ++n)
    {
        MojString resultStr;
        MojTestErrCheck( j->toJson(resultStr) );
        fprintf(stderr, "result: %.*s\n", int(resultStr.length()), resultStr.begin());

        MojObject foo;
        MojTestErrCheck( j->getRequired("foo", foo) );
        if (*i != foo ) {
            fprintf(stderr, "\"foo\" should be present in item %zu #\n", n);
            MojAssert(false);
        }
    }
    return MojErrNone;
}

/**
 * testShardIdCacheIndexes
 *
 * compute a number of new id's (=SHARD_ITEMS_NUMBER);
 */
MojErr MojDbShardManagerTest::testShardIdCacheIndexes (MojDbShardIdCache* ip_cache)
{
    MojAssert(ip_cache);

    MojObject obj;
    MojErr err = MojErrNone;
    MojUInt32 arr[SHARDID_CACHE_SIZE];

    //ShardIdCache: Put id's..
    for (MojInt16 i = 0; i < SHARDID_CACHE_SIZE; i++)
    {
        arr[i] = i * 100; //generate id and keep it
        ip_cache->put(arr[i], obj);
    }

    //compare..
    for (MojInt16 i = 0; i < SHARDID_CACHE_SIZE; i++)
    {
        if (!ip_cache->isExist(arr[i]))
        {
            err = MojErrDbVerificationFailed; //id %d is wrong ", i
        }
    }

    if (ip_cache->isExist(0xFFFFFFFF))
        err = MojErrDbVerificationFailed; //compare id6 is wrong

    //remove all id's
    ip_cache->clear();

    //done

    return err;
}

/**
 * testShardIdCacheOperations
 *
 * - verify id for root path;
 * - store sample shard info;
 * - try to get previously stored info;
 * - get id for path, verify;
 * - set activity, read and verify;
 * - check existance of id, even for wrong id
 */
MojErr MojDbShardManagerTest::testShardIdCacheOperations (MojDbShardIdCache* ip_cache)
{
    MojAssert(ip_cache);

    MojErr err = MojErrNone;
    MojUInt32 id1 = 0xFF, id2 = 0xFFFF;
    MojObject obj1, obj2;
    MojObject v_obj1, v_obj2;
    //generate new object1
    MojObject d1_obj1(static_cast<MojInt32>(id1));
    err = obj1.put(_T("shardId"), d1_obj1);
    MojObject d1_obj2(true);
    err = obj1.put(_T("active"), d1_obj2);
    MojErrCheck(err);

    //generate new object2
    MojObject d2_obj1(static_cast<MojInt32>(id2));
    err = obj1.put(_T("shardId"), d2_obj1);
    MojObject d2_obj2(true);
    err = obj1.put(_T("active"), d2_obj2);
    MojErrCheck(err);

    //store
    ip_cache->put(id1, obj1);
    ip_cache->put(id2, obj2);

    //get
    if(!ip_cache->get(id1, v_obj1))
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    if(!ip_cache->get(id2, v_obj2))
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //verify
    if(v_obj1.compare(obj1) != 0)
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    if(v_obj2.compare(obj1) == 0)
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //update
    ip_cache->update(id1, obj2);

    if(!ip_cache->get(id1, v_obj1))
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //verify
    if(v_obj1.compare(obj2) != 0)
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //del
    ip_cache->del(id2);

    if(ip_cache->get(id2, v_obj2))
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //clear
    ip_cache->clear();

    return err;
}

/**
 * testShardEngine
 */
MojErr MojDbShardManagerTest::testShardEngine (MojDbShardEngine* ip_eng)
{
    MojAssert(ip_eng);

    MojErr err;
    MojUInt32 id;
    MojString str;
    bool found;

    //get shard by wrong id
    MojDbShardInfo info;
    err = ip_eng->get(0xFFFFFFFE, info, found);
    MojTestErrCheck(err);
    MojTestAssert(!found);

    //store sample shard info
    MojDbShardInfo shardInfo;
    generateItem(shardInfo);
    shardInfo.mountPath.assign("/tmp/db8-test/media01");
    id = shardInfo.id;
    err = ip_eng->put(shardInfo);
    MojTestErrCheck(err);

    //get info
    err = ip_eng->get(id, shardInfo, found);
    MojTestErrCheck(err);
    MojTestAssert(found);

    if (shardInfo.mountPath.compare("/tmp/db8-test/media01") != 0)
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //set activity flag, read and verify it
    shardInfo.active = true;
    err = ip_eng->update(shardInfo);
    MojTestErrCheck(err);

    err = ip_eng->get(id, shardInfo, found);
    MojTestErrCheck(err);
    MojTestAssert(found);

    if (!shardInfo.active)
        err = MojErrDbVerificationFailed;

    MojTestErrCheck(err);

    //check existance of id, even for wrong id
    err = ip_eng->isIdExist(id, found);
    MojTestErrCheck(err);

    if (!found)
        err = MojErrDbVerificationFailed;
    MojTestErrCheck(err);

    err = ip_eng->isIdExist(0xFFFFFFFF, found);
    MojTestErrCheck(err);

    if (found)
        err = MojErrDbVerificationFailed;
    MojTestErrCheck(err);

    return err;
}

/**
 * testShardCreateAndRemoveWithRecords
 *
 * - create 3 new shards
 * - add records to 1st and 2nd shard
 * - remove shard 1
 * - verify existance of shard 1
 * - verify records of shard 2
 * - remove shard 2
 * - verify existance of shard 2
 * - verify records of shard 3 (== 0)
 * - remove shard 3
 * - verify existance of shard 3
 */
MojErr MojDbShardManagerTest::testShardCreateAndRemoveWithRecords (MojDb& db)
{
    MojDbShardEngine* p_eng = db.shardEngine();
    MojTestAssert(p_eng);

    MojDbShardInfo shard1, shard2;
    MojVector<MojUInt32> arrShardIds;
    MojUInt32 count;

    //---------------------------------------------------------------
    // test shard 1
    //---------------------------------------------------------------

    MojErr err = createShardObjects1(db, shard1);
    MojErrCheck(err);

    //verify number of records for shard 1 (shard is active)
    err = verifyRecords(_T("TestShard1:1"), db, shard1, count);
    MojTestErrCheck(err);
    MojTestAssert(count == 1);

    //err = db.shardEngine()->purgeShardObjects(1);
    //MojTestErrCheck(err);

    //remove shard 1
    arrShardIds.push(shard1.id);
    err = p_eng->removeShardObjects(arrShardIds);
    MojErrCheck(err);

    //verify number of records for shard 1
    err = verifyRecords(_T("TestShard1:1"), db, shard1, count);
    MojErrCheck(err);
    MojTestAssert(count == 0);

    //verify existance of shard 1
    err = verifyShardExistance(db, shard1);
    MojErrCheck(err);

    //---------------------------------------------------------------
    // test shard 2
    //---------------------------------------------------------------

    err = createShardObjects2(db, shard2);
    MojErrCheck(err);

    //verify number of records for shard 1 (shard is active)
    err = verifyRecords(_T("TestShard1:1"), db, shard2, count);
    MojTestErrCheck(err);
    MojTestAssert(count == 1);

    err = verifyRecords(_T("TestShard2:1"), db, shard2, count);
    MojTestErrCheck(err);
    MojTestAssert(count == 1);

    err = verifyRecords(_T("TestShard3:1"), db, shard2, count);
    MojTestErrCheck(err);
    MojTestAssert(count == 1);

    //remove shard 1
    arrShardIds.push(shard2.id);
    err = p_eng->removeShardObjects(arrShardIds);
    MojErrCheck(err);

    //verify number of records for shard 1
    err = verifyRecords(_T("TestShard1:1"), db, shard2, count);
    MojErrCheck(err);
    MojTestAssert(count == 0);

    err = verifyRecords(_T("TestShard2:1"), db, shard2, count);
    MojErrCheck(err);
    MojTestAssert(count == 0);

    err = verifyRecords(_T("TestShard3:1"), db, shard2, count);
    MojErrCheck(err);
    MojTestAssert(count == 0);

    //verify existance of shard 1
    err = verifyShardExistance(db, shard2);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * Add records to first shard for a single Kind
 */
MojErr MojDbShardManagerTest::createShardObjects1 (MojDb& db, MojDbShardInfo& shard)
{
    MojObject objKind;
    MojString kindId;

    MojErr err = kindId.assign(_T("TestShard1:1"));
    MojErrCheck(err);
    err = objKind.putString(_T("_kind"), kindId.data());
    MojErrCheck(err);

    //generate
    err = generateItem(shard);
    MojErrCheck(err);

    err = addKind(TestShardKind1Str, db);
    MojErrCheck(err);
    err = verifyKindExistance(kindId, db);
    MojErrCheck(err);

    // mount shard
    err = db.storageEngine()->mountShard(shard.id, shard.databasePath);
    MojErrCheck(err);

    //store shard info
    err = db.shardEngine()->put(shard);
    MojErrCheck(err);

    //add record
    MojObject record;
    err = record.putString(_T("_kind"), kindId.data());
    MojErrCheck(err);

    //add value
    MojObject objId(static_cast<MojInt32>(shard.id));
    err = record.put(_T("recId"), objId);
    MojErrCheck(err);

    //put
    MojString strShardId;
    MojDbShardEngine::convertId(shard.id, strShardId);
    err = db.put(record, MojDbFlagNone, MojDbReq(), strShardId);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * Add records for second shard for three different Kinds
 */
MojErr MojDbShardManagerTest::createShardObjects2 (MojDb& db, MojDbShardInfo& shard)
{
    MojObject objKind1;
    MojString kindId1;

    MojErr err = kindId1.assign(_T("TestShard1:1"));
    MojErrCheck(err);
    err = objKind1.putString(_T("_kind"), kindId1.data());
    MojErrCheck(err);

    MojObject objKind2;
    MojString kindId2;

    err = kindId2.assign(_T("TestShard2:1"));
    MojErrCheck(err);
    err = objKind2.putString(_T("_kind"), kindId2.data());
    MojErrCheck(err);

    MojObject objKind3;
    MojString kindId3;

    err = kindId3.assign(_T("TestShard3:1"));
    MojErrCheck(err);
    err = objKind3.putString(_T("_kind"), kindId3.data());
    MojErrCheck(err);

    //generate
    err = generateItem(shard);
    MojErrCheck(err);

    err = addKind(TestShardKind1Str, db);
    MojErrCheck(err);
    err = verifyKindExistance(kindId1, db);
    MojErrCheck(err);

    err = addKind(TestShardKind2Str, db);
    MojErrCheck(err);
    err = verifyKindExistance(kindId2, db);
    MojErrCheck(err);

    err = addKind(TestShardKind3Str, db);
    MojErrCheck(err);
    err = verifyKindExistance(kindId3, db);
    MojErrCheck(err);

    //store shard info
    err = db.shardEngine()->put(shard);
    MojErrCheck(err);

    // mount shard
    err = db.storageEngine()->mountShard(shard.id, shard.databasePath);
    MojErrCheck(err);

    //add record [1]
    MojObject record1;
    err = record1.putString(_T("_kind"), kindId1.data());
    MojErrCheck(err);

    //add value
    MojObject objId(static_cast<MojInt32>(shard.id));
    err = record1.put(_T("recId"), objId);
    MojErrCheck(err);

    //put [1]
    MojString strShardId;
    MojDbShardEngine::convertId(shard.id, strShardId);
    err = db.put(record1, MojDbFlagNone, MojDbReq(), strShardId);
    MojErrCheck(err);

    //add record [2]
    MojObject record2;
    err = record2.putString(_T("_kind"), kindId2.data());
    MojErrCheck(err);

    //put [2]
    err = db.put(record2, MojDbFlagNone, MojDbReq(), strShardId);
    MojErrCheck(err);

    //add record [3]
    MojObject record3;
    err = record3.putString(_T("_kind"), kindId3.data());
    MojErrCheck(err);

    //put [3]
    err = db.put(record3, MojDbFlagNone, MojDbReq(), strShardId);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * addKind
 */
MojErr MojDbShardManagerTest::addKind (const MojChar* strKind, MojDb& db)
{
    MojObject kind;
    MojErr err = kind.fromJson(strKind);
    MojErrCheck(err);
    err = db.putKind(kind);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * is kind exist?
 */
MojErr MojDbShardManagerTest::verifyKindExistance (MojString kindId, MojDb& db)
{
    bool foundOurKind = false;
    MojString str; //for debug

    //kinds map
    MojDbKindEngine::KindMap& map = db.kindEngine()->kindMap();

    for (MojDbKindEngine::KindMap::ConstIterator it = map.begin();
         it != map.end();
         ++it)
    {
        str = it.key();
        if(kindId == str)
        {
            foundOurKind = true;
            break;
        }
    }

    if (!foundOurKind)
        MojErrThrowMsg(MojErrDbKindNotRegistered, "Kind %s not found in kindMap", kindId.data());

    return MojErrNone;
}

/**
 * verifyRecords
 */
MojErr MojDbShardManagerTest::verifyRecords (const MojChar* strKind, MojDb& db, const MojDbShardInfo&, MojUInt32& count)
{
    MojDbQuery query;
    MojDbCursor cursor;
    count = 0;

    MojErr err = query.from(strKind);
    MojErrCheck(err);

    err = db.find(query, cursor);
    MojErrCheck(err);

    while (true)
    {
        bool found;
        MojObject dbObj;

        err = cursor.get(dbObj, found);
        MojErrCheck(err);
        if (!found)
            break;

        ++count;
    }

    return MojErrNone;
}

/**
 * verifyExistance1
 */
MojErr MojDbShardManagerTest::verifyShardExistance (MojDb& db, const MojDbShardInfo& shard)
{
    bool found;

    MojErr err = db.shardEngine()->isIdExist(shard.id, found);
    MojTestErrCheck(err);

    if (!found)
    {
        err = MojErrDbVerificationFailed;
        MojErrCheck(err);
    }

    return MojErrNone;
}

/**
 * generateItem
 */
MojErr MojDbShardManagerTest::generateItem (MojDbShardInfo& o_shardInfo, MojUInt32 shardId)
{
    if (shardId == 0) {
        static MojUInt32 id = 0xFF;
        o_shardInfo.id = ++id;
    } else {
        o_shardInfo.id = shardId;
    }
    MojDbShardEngine::convertId(o_shardInfo.id, o_shardInfo.id_base64);
    o_shardInfo.active = true;
    o_shardInfo.transient = false;
    o_shardInfo.deviceId.format("ID%x", o_shardInfo.id);
    o_shardInfo.deviceUri.format("URI%x", o_shardInfo.id);
    o_shardInfo.mountPath.format("/tmp/db8-test/media%x", o_shardInfo.id);
    o_shardInfo.deviceName.format("TEST-%x", o_shardInfo.id);
    o_shardInfo.databasePath.format("%s/shard-%x", MojDbTestDir, o_shardInfo.id);

    MojErr err;
    err = o_shardInfo.deviceUri.assign(MojDbTestDir);
    MojTestErrCheck(err);
    err = o_shardInfo.deviceUri.append("/drive/");
    MojTestErrCheck(err);

    if (access(o_shardInfo.deviceUri, F_OK) != 0) {
        err = MojMkDir(o_shardInfo.deviceUri, MOJ_S_IRWXU);
        MojTestErrCheck(err);
    }

    return MojErrNone;
}

/**
 * displayMessage
 */
MojErr MojDbShardManagerTest::displayMessage(const MojChar* format, ...)
{
    va_list args;
    va_start (args, format);
    MojErr err = MojVPrintF(format, args);
    va_end(args);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardManagerTest::configureShardEngine(MojDb& db)
{
    MojErr err;

    MojString shardsDir;
    err = shardsDir.assign(".shards/");
    MojTestErrCheck(err);
    /*err = MojMkDir(shardsDir, MOJ_S_IRWXU);
     MojErrCheck(err);**/

    MojString linksDir;
    err = linksDir.assign(MojDbTestDir);
    MojTestErrCheck(err);
    err = linksDir.append("/link/");
    MojTestErrCheck(err);
    err = MojMkDir(linksDir, MOJ_S_IRWXU);
    MojTestErrCheck(err);

    MojString fallbackDir;
    err = fallbackDir.assign(MojDbTestDir);
    MojTestErrCheck(err);
    err = fallbackDir.append("/fallback/");
    MojTestErrCheck(err);
    err = MojMkDir(fallbackDir, MOJ_S_IRWXU);
    MojTestErrCheck(err);

    MojObject conf;
    err = conf.put("enable_sharding", true);
    MojTestErrCheck(err);
    err = conf.put("shard_db_prefix", shardsDir);
    MojTestErrCheck(err);
    err = conf.put("device_links_path", linksDir);
    MojTestErrCheck(err);
    err = conf.put("fallback_path", fallbackDir);
    MojTestErrCheck(err);

    err = conf.put("device_minimum_free_bytes", 10000000);
    MojTestErrCheck(err);

    err = conf.put("device_minimum_free_percentage", MojDecimal(0, 800000));
    MojTestErrCheck(err);

    err = db.shardEngine()->configure(conf);
    MojTestErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardManagerTest::testShardProcessing(MojDb& db)
{
    MojErr err;

    MojDbShardInfo shardInfo;
    err = generateItem(shardInfo, MagicId);
    MojTestErrCheck(err);

    err = configureShardEngine(db);
    MojTestErrCheck(err);

    MojString kindId;
    err = kindId.assign(_T("TestShard1:1"));
    MojTestErrCheck(err);

    err = addKind(TestShardKind1Str, db);
    MojTestErrCheck(err);

    MojDbShardInfo databaseShard;
    // mount shard
    err = db.shardEngine()->processShardInfo(shardInfo, &databaseShard);
    MojTestErrCheck(err);

    err = fillData(db, databaseShard.id);
    MojTestErrCheck(err);

    err = expect(db, "[1,3,5,7,9,2,4,6,8,10]", true); // re-order by shard prefix vs timestamp
    MojTestErrCheck(err);

    shardInfo.active = false;
    err = db.shardEngine()->processShardInfo(shardInfo);
    MojTestErrCheck(err);

    err = expect(db, "[2,4,6,8,10]", false); // re-order by shard prefix vs timestamp
    MojTestErrCheck(err);

    // mount again shard
    shardInfo.active = true;
    err = db.shardEngine()->processShardInfo(shardInfo);
    MojTestErrCheck(err);

    err = expect(db, "[1,3,5,7,9,2,4,6,8,10]", true); // re-order by shard prefix vs timestamp
    MojTestErrCheck(err);

    return MojErrNone;
}


MojErr MojDbShardManagerTest::testShardTransient(MojDb& db)
{
    // add here engines that support this test
    static std::set<std::string> supportedEngines = {
        "sandwich",
    };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end())
    {
        return MojErrNone; // nothing to test if feature not supported
    }

    MojErr err;

    MojDbShardInfo shardInfo;
    err = generateItem(shardInfo, MagicId);
    MojTestErrCheck(err);

    err = configureShardEngine(db);
    MojTestErrCheck(err);

    MojString kindId;
    err = kindId.assign(_T("TestShard1:1"));
    MojTestErrCheck(err);

    err = addKind(TestShardKind1Str, db);
    MojTestErrCheck(err);

    MojDbShardInfo databaseShard;
    // mount shard
    shardInfo.active = true;
    err = db.shardEngine()->processShardInfo(shardInfo, &databaseShard);
    MojTestErrCheck(err);

    err = fillData(db, databaseShard.id);
    MojTestErrCheck(err);

    err = expect(db, "[1,3,5,7,9,2,4,6,8,10]", true); // re-order by shard prefix vs timestamp
    MojTestErrCheck(err);

    shardInfo.active = false;
    shardInfo.transient = true;
    err = db.shardEngine()->processShardInfo(shardInfo);
    MojTestErrCheck(err);

    err = expect(db, "[2,4,6,8,10]", true); // re-order by shard prefix vs timestamp
    MojTestAssert(access(databaseShard.databasePath.data(), F_OK) != 0);

    return MojErrNone;
}


/**
 * cleanup
 */
void MojDbShardManagerTest::cleanup()
{
    (void) MojRmDirRecursive(MojDbTestDir);
}

