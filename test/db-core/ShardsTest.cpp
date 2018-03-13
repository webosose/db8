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

/**
 *  @file ShardsTest.cpp
 *  Verify multi-shard logic
 */

#include <db/MojDbSearchCursor.h>
#include <db/MojDbKind.h>
#include <core/MojObjectBuilder.h>

#include "MojDbCoreTest.h"

namespace {
    const MojChar* const MojTestKind1Str1 =
            _T("{\"id\":\"Test:1\",")
            _T("\"owner\":\"com.foo.bar\",")
            _T("\"indexes\":[")
                    _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},")
                    _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]}")
            _T("]}");

    const MojChar* const MojTestKind1Str2 =
            _T("{\"id\":\"Test:1\",")
            _T("\"owner\":\"com.foo.bar\",")
            _T("\"indexes\":[")
                   _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]}")
            _T("]}");

    const MojChar* const MojTestKind2Str1 =
            _T("{\"id\":\"OtherTest:1\",")
            _T("\"owner\":\"com.foo.bar\",")
            _T("\"indexes\":[")
                   _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]}")
            _T("]}");
    const MojChar* const MojTestKind2Str2 =
            _T("{\"id\":\"OtherTest:1\",")
            _T("\"owner\":\"com.foo.bar\",")
            _T("\"indexes\":[")
                _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},")
                _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]}")
            _T("]}");

    const MojUInt32 MagicId = 42u;
    const MojChar* ServiceName = _T("com.foo.bar");
}

/**
 * Test fixture for tests around multi-shard logic
 */
struct ShardsTest : public MojDbCoreTest
{
    MojDbIdGenerator idGen;
    MojDbShardEngine *shardEngine;
    MojString shardPath;
    MojString magicShardPath;

    void SetUp()
    {
        MojDbCoreTest::SetUp();

        shardEngine = db.shardEngine();
        MojAssertNoErr( configureShardEngine(&db) );

        // initialize own copy of _id generator
        idGen.init();

        // add kind
        MojObject obj;
        MojAssertNoErr( obj.fromJson(MojTestKind1Str1) );
        MojAssertNoErr( db.putKind(obj) );

        MojAssertNoErr( magicShardPath.format("%s/shard-%s", path.c_str(), std::to_string(MagicId).c_str()) );
    }

    MojErr put(MojObject &obj, MojUInt32 shardId = MojDbIdGenerator::MainShardId)
    {
        MojErr err;
        /* XXX: once shard id will be added to Put interface (GF-11469)
         *  MojAssertNoErr( db.put("_shardId", MagicId) );
         */

        // work-around with pre-generated _id
        MojObject id;
        err = idGen.id(id, shardId);
        MojErrCheck( err );

        err = obj.put("_id", id);
        MojErrCheck( err );

        return db.put(obj);
    }

    void registerShards(MojDb* db)
    {

        // add shards
        MojDbShardInfo shardInfo;
        shardInfo.id = MagicId;
        shardInfo.active = true;
        MojAssertNoErr( db->shardEngine()->put( shardInfo ) );

        MojAssertNoErr( shardPath.format("%s/shard-%s", path.c_str(), std::to_string(shardInfo.id).c_str()) );
        MojAssertNoErr( db->storageEngine()->mountShard(MagicId, shardPath) );

        // verify shard put
        shardInfo = {};
        bool found;
        MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
        ASSERT_TRUE( found );
        ASSERT_EQ( MagicId, shardInfo.id );
        ASSERT_TRUE( shardInfo.active );
    }

    void unregisterShards(MojDb* db)
    {
        MojErr err;

        // add shards
        MojDbShardInfo shardInfo;
        shardInfo.id = MagicId;
        shardInfo.active = false;

        err = db->shardEngine()->put(shardInfo);
        MojAssertNoErr(err);

        err = db->storageEngine()->unMountShard(MagicId);
        MojAssertNoErr(err);

        // verify shard put
        shardInfo = {};
        bool found;
        MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
        ASSERT_TRUE( found );
        ASSERT_EQ( MagicId, shardInfo.id );
        ASSERT_FALSE( shardInfo.active );
    }

    void fillData()
    {
        // add objects
        for(int i = 1; i <= 10; ++i)
        {
            MojObject obj;
            MojAssertNoErr( obj.putString("_kind", "Test:1") );
            MojAssertNoErr( obj.put("foo", i) );
            MojAssertNoErr( obj.put("bar", i/4) );

            if (i % 2) MojExpectNoErr( put(obj, MagicId) );
            else MojExpectNoErr( put(obj) );
        }
    }

    void expect(MojDb* db, const MojChar* expectedJson, bool includeInactive = false)
    {
        MojDbQuery query;
        query.clear();
        MojAssertNoErr( query.from(_T("Test:1")) );
        if (includeInactive) query.setIgnoreInactiveShards( false );

        MojString str;
        MojDbSearchCursor cursor(str);
        MojAssertNoErr( db->find(query, cursor) );

        MojObjectBuilder builder;
        MojAssertNoErr( builder.beginArray() );
        MojAssertNoErr( cursor.visit(builder) );
        MojAssertNoErr( cursor.close() );
        MojAssertNoErr( builder.endArray() );
        MojObject results = builder.object();

        MojString json;
        MojAssertNoErr( results.toJson(json) );

        MojObject expected;
        MojAssertNoErr( expected.fromJson(expectedJson) );

        ASSERT_EQ( expected.size(), results.size() )
            << "Amount of expected records should match amount of records in result";
        MojObject::ConstArrayIterator j = results.arrayBegin();
        size_t n = 0;
        for (MojObject::ConstArrayIterator i = expected.arrayBegin();
             j != results.arrayEnd() && i != expected.arrayEnd();
             ++i, ++j, ++n)
        {
            MojString resultStr;
            MojAssertNoErr( j->toJson(resultStr) );
            fprintf(stderr, "result: %s\n", resultStr.data());

            MojObject foo;
            MojAssertNoErr( j->getRequired("foo", foo) )
                << "\"foo\" should be present in item #" << n;
            ASSERT_EQ( *i, foo );
        }
    }

    MojErr configureShardEngine(MojDb* db)
    {
        MojErr err;

        MojString shardsDir;

        err = shardsDir.assign(path.c_str());
        MojErrCheck(err);
        err = shardsDir.append("/.shards/");
        MojErrCheck(err);
        err = MojMkDir(shardsDir, MOJ_S_IRWXU);
        MojErrCheck(err);

        MojString linksDir;
        err = linksDir.assign(path.c_str());
        MojErrCheck(err);
        err = linksDir.append("/link/");
        MojErrCheck(err);
        err = MojMkDir(linksDir, MOJ_S_IRWXU);
        MojErrCheck(err);

        MojString fallbackDir;
        err = fallbackDir.assign(path.c_str());
        MojErrCheck(err);
        err = fallbackDir.append("/fallback/");
        MojErrCheck(err);
        err = MojMkDir(fallbackDir, MOJ_S_IRWXU);
        MojErrCheck(err);

        MojObject conf;
        err = conf.put("enable_sharding", true);
        MojErrCheck(err);
        err = conf.put("shard_db_prefix", shardsDir);
        MojErrCheck(err);
        err = conf.put("device_links_path", linksDir);
        MojErrCheck(err);
        err = conf.put("fallback_path", fallbackDir);
        MojErrCheck(err);

        err = conf.put("device_minimum_free_bytes", 10000000);
        MojErrCheck(err);

        err = conf.put("device_minimum_free_percentage", MojDecimal(0, 800000));
        MojErrCheck(err);

        err = db->shardEngine()->configure(conf);
        MojErrCheck(err);

        return MojErrNone;
    }
};

/**
 * @test Verify that put shardInfo works
 */
TEST_F(ShardsTest, putShardInfo)
{
    // add shard
    shardEngine = db.shardEngine();
    MojDbShardInfo shardInfo;
    shardInfo.id = MagicId;
    shardInfo.active = true;
    MojAssertNoErr( shardEngine->put( shardInfo ) );

    bool found;
    // verify shard put
    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    EXPECT_TRUE( found );
    EXPECT_EQ( MagicId, shardInfo.id );
    EXPECT_TRUE( shardInfo.active );
}

/**
 * @test Verify that setActivity actually changes state
 */
TEST_F(ShardsTest, setActivity)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojDbShardInfo shardInfo;
    bool found;

    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    EXPECT_TRUE( found );
    EXPECT_EQ( MagicId, shardInfo.id );
    EXPECT_TRUE( shardInfo.active );

    // verify active state change
    shardInfo.active = false;
    MojAssertNoErr( shardEngine->update(shardInfo) );
    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    ASSERT_TRUE( found );
    EXPECT_EQ( MagicId, shardInfo.id );
    EXPECT_FALSE( shardInfo.active );

    shardInfo.active = true;
    MojAssertNoErr( shardEngine->update(shardInfo) );
    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    ASSERT_TRUE( found );
    EXPECT_EQ( MagicId, shardInfo.id );
    EXPECT_TRUE( shardInfo.active );
}

/**
 * @test Verify behavior of different request with non-existing shard
 *   - getting info for non-existing shard
 *   - adding object to a non-existing shard
 */
TEST_F(ShardsTest, nonexistingShard)
{
    MojDbShardInfo shardInfo = {};
    bool found;

    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    EXPECT_FALSE(found);

    /* 'restore from MojObject' is not working
    shardInfo = {};
    shardInfo.id = MagicId;
    shardInfo.active = true;
    EXPECT_EQ( MojErrDbObjectNotFound, shardEngine->update(shardInfo) );

    MojObject obj;
    MojAssertNoErr( obj.putString("_kind", "Test:1") );
    MojAssertNoErr( obj.put("foo", 0) );

    // TODO: EXPECT_EQ( MojErr...,  put(obj, MagicId) );
    EXPECT_NE( MojErrNone, put(obj, MagicId) );
    */
}

/**
 * @test Verify that after MojDb re-open all shards are inactive
 */
TEST_F(ShardsTest, initalInactivity)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojDbShardInfo shardInfo;
    bool found;

    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    ASSERT_TRUE( found );
    ASSERT_TRUE( shardInfo.active );

    MojAssertNoErr( db.close() );
    // re-open
    MojAssertNoErr( db.open(path.c_str()) );

    MojAssertNoErr( shardEngine->get( MagicId, shardInfo, found ) );
    ASSERT_TRUE(found);
    EXPECT_FALSE( shardInfo.active );
}

/**
 * @test Verify that mount/unmount/re-mount shards have effect on visible objects
 */
TEST_F(ShardsTest, remountShard)
{
    // add here engines that support this test
    static std::set<std::string> supportedEngines = {
        "sandwich",
    };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end())
    {
        return; // nothing to test if feature not supported
    }

    // shard is not mounted
    MojExpectErr( MojErrDbInvalidShardId, db.storageEngine()->unMountShard(MagicId) )
        << "Expecting that shard " << MagicId << " isn't mounted";

    ASSERT_NO_FATAL_FAILURE( registerShards(&db) );

    MojString altShardPath;
    MojAssertNoErr( altShardPath.format("%s/alternative", shardPath.data()) );

    MojExpectErr( MojErrDbInvalidShardId, db.storageEngine()->mountShard(MagicId, altShardPath) )
        << "Shouldn't be able to mount shard " << MagicId << " twice";

    // store some object
    MojObject obj;
    MojAssertNoErr( obj.putString("_kind", "Test:1") );
    MojAssertNoErr( obj.put("foo", 20141031) );
    MojAssertNoErr( put(obj, MagicId) );

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[20141031]") ) << "Should see new obj";

    MojAssertNoErr( db.storageEngine()->unMountShard(MagicId) );
    MojExpectErr( MojErrDbInvalidShardId, db.storageEngine()->unMountShard(MagicId) );

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[]") ) << "Shouldn't see unmounted shard";

    MojAssertNoErr( db.storageEngine()->mountShard(MagicId, shardPath) );

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[20141031]") ) << "Should see our obj again";
}

/**
 * @test Verify effect of mounting two different shards to same database
 */
TEST_F(ShardsTest, mountCollision)
{
    // add here engines that support this test
    static std::set<std::string> supportedEngines = {
        "sandwich",
    };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end())
    {
        return; // nothing to test if feature not supported
    }

    MojAssertErr( MojErrDbInvalidShardId, db.storageEngine()->unMountShard(MagicId) )
        << "Shard " << MagicId << " shouldn't be mounted";

    MojAssertNoErr( db.storageEngine()->mountShard(MagicId, magicShardPath) );
    MojExpectErr( MojErrDbIO, db.storageEngine()->mountShard(MagicId+1, magicShardPath) )
        << "Mounting two shards from same database isn't allowed";

    // store some object
    MojObject obj;
    MojAssertNoErr( obj.putString("_kind", "Test:1") );
    MojAssertNoErr( obj.put("foo", 20141031) );
    MojAssertNoErr( put(obj, MagicId) );

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[20141031]") ) << "Should see new obj";

    MojAssertNoErr( db.storageEngine()->unMountShard(MagicId) );
    MojExpectErr( MojErrDbInvalidShardId, db.storageEngine()->unMountShard(MagicId) );
    MojExpectErr( MojErrDbInvalidShardId, db.storageEngine()->unMountShard(MagicId+1) )
        << "Shard " << MagicId+1 << " shouldn't be able to mount together with " << MagicId;

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[]") ) << "Shouldn't see unmounted shard";

    // re-mount same shard under different shard id
    MojAssertNoErr( db.storageEngine()->mountShard(MagicId, magicShardPath) );

    ASSERT_NO_FATAL_FAILURE( expect(&db, "[20141031]") ) << "Should see our obj again";
}

/**
 * @test Verify that all records available (including active shards)
 */
TEST_F(ShardsTest, queryActiveShard)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));
    fillData();

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]"); // re-order by shard prefix vs timestamp
}

/*
 * @test Verify that records for inactive shards are filtered out
 */
TEST_F(ShardsTest, queryInactiveShard)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));
    fillData();

    MojDbShardInfo shardInfo;
    bool found;
    MojAssertNoErr (shardEngine->get(MagicId, shardInfo, found));
    ASSERT_TRUE(found);
    EXPECT_EQ(MagicId, shardInfo.id);

    shardInfo.active = false;
    MojAssertNoErr (shardEngine->update(shardInfo));

    expect(&db, "[2,4,6,8,10]"); // no records from shard MagicId
}

/*
 * @test Verify that query.setIgnoreInactiveShards(false) results in all
 * records returned (even for inactive shards).
 */
TEST_F(ShardsTest, queryInactiveShardWithoutIgnore)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));
    fillData();

    MojDbShardInfo shardInfo;
    bool found;
    MojAssertNoErr (shardEngine->get(MagicId, shardInfo, found));
    ASSERT_TRUE(found);
    EXPECT_EQ(MagicId, shardInfo.id);

    shardInfo.active = false;
    MojAssertNoErr (shardEngine->update(shardInfo));

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]", true); // re-order by shard prefix vs timestamp
}


TEST_F(ShardsTest, putKindHash)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    MojErr err;

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    KindHash::KindHashContainer kinds;
    MojDbReq req;

    MojObject kindObject;

    err = kindObject.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(kindObject, MojDbFlagNone, req);
    MojAssertNoErr(err);

    MojDbKind* kind;
    err = db.kindEngine()->getKind(_T("OtherTest:1"), kind);
    MojAssertNoErr(err);

    ASSERT_TRUE(kind);

    MojSize hash = kind->hash();

    // test kind hash should be registered in hash table
    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());
    EXPECT_EQ(hash, kinds.at(0).hash());

    // check, that put kind again will update the same kind
    MojObject kindObject2;

    err = kindObject.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(kindObject, MojDbFlagNone, req);
    MojAssertNoErr(err);

    kinds.clear();

    // test kind hash should be registered in hash table
    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());
    EXPECT_EQ(hash, kinds.at(0).hash());

    // check update
    MojObject kindObject3;
    err = kindObject3.fromJson(MojTestKind2Str2);
    MojAssertNoErr(err);

    err = db.putKind(kindObject3, MojDbFlagNone, req);
    MojAssertNoErr(err);

    MojDbKind* kind2;
    err = db.kindEngine()->getKind(_T("OtherTest:1"), kind2);
    MojAssertNoErr(err);

    ASSERT_TRUE(kind2);

    MojSize hash2 = kind2->hash();
    EXPECT_NE(hash, hash2);

    kinds.clear();
    // test kind hash should be registered in hash table
    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());
    EXPECT_EQ(hash2, kinds.at(0).hash());
}

TEST_F(ShardsTest, putKindsHash)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojErr err;
    MojDbReq req;

    KindHash::KindHashContainer kinds;

    err = db.shardEngine()->putKindsHashes(MagicId, req);
    MojAssertNoErr(err);

    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());

    // and try again "sync" all kinds
    err = db.shardEngine()->putKindsHashes(MagicId);
    MojAssertNoErr(err);

    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());

    // modify kind
    MojObject obj;
    err = obj.fromJson(MojTestKind1Str2);
    MojAssertNoErr(err);

    err = db.putKind(obj, MojDbFlagNone, req);
    MojAssertNoErr(err);

    err = db.shardEngine()->putKindsHashes(MagicId, req);
    MojAssertNoErr(err);

    KindHash::KindHashContainer kinds2;

    err = KindHash::loadHashes(&db, MagicId, &kinds2, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds2.size());
}


TEST_F(ShardsTest, loadKindsHash)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojDbReq req;

    MojErr err;
    err = db.shardEngine()->putKindsHashes(MagicId, req);
    MojAssertNoErr(err);

    KindHash::KindHashContainer kinds;

    err = KindHash::loadHashes(&db, MagicId, &kinds, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds.size());

    // modify kind structure
    MojObject obj;
    err = obj.fromJson(MojTestKind1Str2);
    MojAssertNoErr(err);

    err = db.putKind(obj, MojDbFlagNone, req);
    MojAssertNoErr(err);

    err = db.shardEngine()->putKindsHashes(MagicId, req);
    MojAssertNoErr(err);

    KindHash::KindHashContainer kinds2;

    err = KindHash::loadHashes(&db, MagicId, &kinds2, req);
    MojAssertNoErr(err);

    EXPECT_EQ(1u, kinds2.size());

    // register additional kind
    err = obj.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(obj);
    MojAssertNoErr(err);

    err = db.shardEngine()->putKindsHashes(MagicId, req);
    MojAssertNoErr(err);

    KindHash::KindHashContainer kinds3;

    err = KindHash::loadHashes(&db, MagicId, &kinds3, req);
    MojAssertNoErr(err);

    EXPECT_EQ(2u, kinds3.size());
}

TEST_F(ShardsTest, delKindData_goodToken)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojErr err;
    fillData();

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]");

    err = db.shardEngine()->delKindData(MagicId, _T("Test:1"));
    MojAssertNoErr(err);

    expect(&db, "[2,4,6,8,10]");
}

TEST_F(ShardsTest, delKindData_badToken)
{
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojErr err;
    fillData();

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]"); // re-order by shard prefix vs timestamp

    MojAssertNoErr(db.close());

    MojDb db1;
    MojAssertNoErr( db1.open((path + "/db1").c_str()) );

    // add kind
    shardEngine = db1.shardEngine();

    // initialize own copy of _id generator
    idGen.init();

    // step8: add kind
    MojObject obj;
    MojAssertNoErr( obj.fromJson(MojTestKind1Str2) );
    MojAssertNoErr( db1.putKind(obj) );

    MojAssertNoErr( magicShardPath.format("%s/shard-%s", path.c_str(), std::to_string(MagicId).c_str()) );

    ASSERT_NO_FATAL_FAILURE(registerShards(&db1));

    // inside
    err = db1.shardEngine()->delKindData(MagicId, _T("Test:1"));
    MojAssertNoErr(err);

    expect(&db1, "[]");
}

TEST_F(ShardsTest, delKindData)
{
    MojErr err;

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    fillData();

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]"); // re-order by shard prefix vs timestamp

    err = db.shardEngine()->delKindData(MagicId, _T("Test:1"));
    MojAssertNoErr(err);
}

TEST_F(ShardsTest, InconsistencyTest)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    MojErr err;

    // 1. Open db1
    // 2. Register Kind in db1
    // 3. Attach shard for db1
    // 4. Put data into shard for Kind:1
    // 5. Detach shard with test data
    // 6. Close db1
    // 7. Open db2
    // 8. Register again Kind:1 with changed schema (different indexes)
    // 9. Attach shard with test data
    // 10. Search for data. Serialization should correctly de-serialize data

    // step 1, 2, 3 are already done in test init
    ASSERT_NO_FATAL_FAILURE(registerShards(&db));
    fillData(); // step 4

    err = db.shardEngine()->putKindsHashes(MagicId);
    MojAssertNoErr(err);

    expect(&db, "[1,3,5,7,9,2,4,6,8,10]"); // re-order by shard prefix vs timestamp

    // step 5: accurate unmount shard
    MojDbShardInfo shardInfo;
    bool found;

    err = shardEngine->get(MagicId, shardInfo, found);
    MojAssertNoErr(err);
    ASSERT_TRUE(found);
    EXPECT_EQ(MagicId, shardInfo.id);

    shardInfo.active = false;

    err = shardEngine->update(shardInfo);
    MojAssertNoErr(err);

    // paranoic check, that shard is inactive
    expect(&db, "[2,4,6,8,10]"); // no records from shard MagicId

    ASSERT_NO_FATAL_FAILURE(unregisterShards(&db));

    // step 6
    err = db.close();
    MojAssertNoErr(err);

    // step 7
    MojDb db1;
    err = db1.open((path + "/db1").c_str());
    MojAssertNoErr(err);

    // initialize own copy of _id generator
    err = idGen.init();
    MojAssertNoErr(err);

    shardEngine = db1.shardEngine();

    // step8: add kind
    MojObject obj;
    err = obj.fromJson(MojTestKind1Str2);
    MojAssertNoErr(err);

    err = db1.putKind(obj);
    MojAssertNoErr(err);

    // paranoic check, that shard is inactive
    expect(&db1, "[]"); // no records from shard MagicId

    // step 9
    ASSERT_NO_FATAL_FAILURE(registerShards(&db1));

    obj.clear();

    err = db1.shardEngine()->dropGarbage(MagicId, MojDbReq());
    MojAssertNoErr(err);

    // step 10
    expect(&db1, "[]"); // no records
}

TEST_F(ShardsTest, PutKindPermissionsTest)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojErr err;
    MojDbReq req;

    err = req.domain(ServiceName);
    MojAssertNoErr(err);

    req.admin(false);

    MojObject kindObject;
    err = kindObject.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(kindObject, MojDbFlagNone, req);
    MojAssertNoErr(err);

}

TEST_F(ShardsTest, PutPermissionsTest)
{
    static std::set<std::string> supportedEngines = { "sandwich" };
    if (supportedEngines.find(MojDbStorageEngine::engineFactory()->name()) == supportedEngines.end()) {
        return; // nothing to test if feature not supported
    }

    ASSERT_NO_FATAL_FAILURE(registerShards(&db));

    MojErr err;
    MojDbReq req;

    err = req.domain(ServiceName);
    MojAssertNoErr(err);

    req.admin(false);

    MojObject kindObject;
    err = kindObject.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(kindObject, MojDbFlagNone, req);
    MojAssertNoErr(err);

    MojObject object;
    err = object.putString(_T("_kind"), _T("OtherTest:1"));
    MojAssertNoErr(err);
    err = object.putString(_T("foo"), _T("foo1"));
    MojAssertNoErr(err);
    err = object.putString(_T("bar"), _T("bar1"));
    MojAssertNoErr(err);

    MojString shardIdStr;
    err = db.shardEngine()->convertId(MagicId, shardIdStr);
    MojAssertNoErr(err);

    err = db.put(object, MojDbFlagNone, req, shardIdStr);
    MojAssertNoErr(err);

}
