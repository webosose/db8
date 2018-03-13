// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include <array>
#include <thread>

#include <leveldb/memory_db.hpp>
#include <leveldb/txn_db.hpp>

#include "db/MojDb.h"
#include "db/MojDbStorageEngine.h"
#include "db/MojDbMediaHandler.h"

#include "Runner.h"

#include "engine/sandwich/pool.hpp"
#include "engine/sandwich/MojDbSandwichItem.h"

using ::testing::PrintToString;

namespace {
    MojObject obj(const leveldb::Slice &orig)
    {
        MojString s;
        MojExpectNoErr( s.assign(orig.data(), orig.size()) );
        return {s};
    }

    MojObject obj(std::initializer_list<std::pair<leveldb::Slice, MojObject>> props)
    {
        MojObject o(MojObject::TypeObject);
        for (auto prop : props)
        {
            MojString key;
            MojExpectNoErr( key.assign(prop.first.data(), prop.first.size()) );
            MojExpectNoErr( o.put(key, prop.second) );
        }
        return o;
    }

    MojObject arr(std::initializer_list<MojObject> items)
    {
        MojObject a(MojObject::TypeArray);
        for (auto item : items) a.push(item);
        return a;
    }

    MojObject json(const leveldb::Slice &text)
    {
        MojObject obj;
        MojExpectNoErr( obj.fromJson(text.data(), text.size()) );
        return obj;
    }
} // anonymous

/* from leveldb-tl/test/util.hpp
 * XXX: expose it in leveldb-tl? */
namespace leveldb {
    inline void PrintTo(const Slice &s, ::std::ostream *os)
    { *os << ::testing::PrintToString(s.ToString()); }
}

inline ::testing::AssertionResult check(
    const leveldb::Status &s,
    std::function<bool(const leveldb::Status &)> pred = [](const leveldb::Status &s) { return s.ok(); }
)
{
    if (pred(s)) return ::testing::AssertionSuccess() << s.ToString();
    else return ::testing::AssertionFailure() << s.ToString();
}

#define ASSERT_OK(E) ASSERT_TRUE(check(E))
#define EXPECT_OK(E) EXPECT_TRUE(check(E))

#define ASSERT_FAIL(E) ASSERT_FALSE(check(E))
#define EXPECT_FAIL(E) EXPECT_FALSE(check(E))

#define ASSERT_STATUS(M, E) ASSERT_TRUE(check(E, [](const leveldb::Status &s) { return s.Is ## M(); }))
#define EXPECT_STATUS(M, E) EXPECT_TRUE(check(E, [](const leveldb::Status &s) { return s.Is ## M(); }))
/* end */

struct TestPath : public ::testing::Test
{
    std::string test_path;

    void SetUp() override
    {
        const ::testing::TestInfo* const test_info =
          ::testing::UnitTest::GetInstance()->current_test_info();

        test_path = std::string(tempFolder) + '/'
             + test_info->test_case_name() + '-' + test_info->name();
    }
};

TEST(PoolTest, pool1)
{
    Pool<int, leveldb::MemoryDB> pool;
    pool.Mount(0, leveldb::MemoryDB {
        { "a", "0" },
        { "b", "0" },
        { "d", "0" },
    });

    auto it = pool.NewIterator();
    ASSERT_TRUE( bool(it) );
    it->SeekToFirst();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "a", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "b", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "d", it->key() );
    it->Next();
    EXPECT_FALSE( it->Valid() );
}

TEST(PoolTest, pool2)
{
    Pool<int, leveldb::MemoryDB> pool;
    pool.Mount(0, leveldb::MemoryDB {
        { "a", "0" },
        { "b", "0" },
        { "d", "0" },
    });
    pool.Mount(1, leveldb::MemoryDB {
        { "c", "1" },
        // { "d", "1" }, -- no intersection between shards
        { "e", "1" },
    });

    auto it = pool.NewIterator();
    ASSERT_TRUE( bool(it) );
    it->SeekToFirst();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "a", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "b", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "c", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "d", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "e", it->key() );
    it->Next();
    EXPECT_FALSE( it->Valid() );
}

TEST(PoolTest, pool3)
{
    Pool<int, leveldb::MemoryDB> pool;
    pool.Mount(0, leveldb::MemoryDB {
        { "a", "0" },
        { "b", "0" },
        { "d", "0" },
    });

    pool.Mount(1, leveldb::MemoryDB {
        { "c", "1" },
        // { "d", "1" }, -- no intersection between shards
        { "e", "1" },
    });

    pool.Mount(2, leveldb::MemoryDB {
        { "f", "2" },
        { "0", "2" },
    });

    auto it = pool.NewIterator();
    ASSERT_TRUE( bool(it) );
    it->SeekToFirst();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "0", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "a", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "b", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "c", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "d", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "e", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "f", it->key() );
    it->Next();
    EXPECT_FALSE( it->Valid() );
}

TEST(PoolTest, txn_pool2)
{
    Pool<int, leveldb::MemoryDB> pool;
    pool.Mount(0, leveldb::MemoryDB {
        { "b", "0" },
        { "c", "0" },
    });
    pool.Mount(1, leveldb::MemoryDB {
        { "a", "1" },
        { "d", "1" },
    });
    auto txn = pool.ref<leveldb::TxnDB>();
    std::string v;

    ASSERT_OK( txn.Select(1).Get("a", v) );
    EXPECT_EQ( "1", v );
    ASSERT_OK( txn.Select(0).Put("e", "0") );
    ASSERT_OK( txn.Select(1).Put("f", "1") );

    EXPECT_OK( txn.Select(0).Get("e", v) );
    EXPECT_OK( txn.Select(1).Get("f", v) );

    auto it = txn.NewIterator();
    ASSERT_TRUE( bool(it) );
    it->SeekToFirst();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "a", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "b", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "c", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "d", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "e", it->key() );
    it->Next();
    ASSERT_TRUE( it->Valid() );
    EXPECT_EQ( "f", it->key() );
    it->Next();
    EXPECT_FALSE( it->Valid() );

    EXPECT_STATUS( NotFound, pool.Select(0).Get("e", v) );
    EXPECT_STATUS( NotFound, pool.Select(1).Get("f", v) );
}

struct UnmountTest : TestPath {};

TEST_F(UnmountTest, unmount_shard_in_txn)
{
    const MojDbShardId magicShard = 42;

    MojString magicShardPath;
    MojAssertNoErr( magicShardPath.format("%s/magic", test_path.c_str()) );

    // we'll work with this objects
    MojRefCountedPtr<MojDbStorageEngine> engine;
    MojRefCountedPtr<MojDbStorageTxn> txn;
    MojRefCountedPtr<MojDbStorageExtDatabase> db;

    // create and open engine
    MojAssertNoErr( MojDbStorageEngine::createEngine("sandwich", engine) );
    MojAssertNoErr( engine->configure({}) );
    MojAssertNoErr( engine->open(test_path.c_str()) );

    // mount our magic shard
    MojAssertNoErr( engine->mountShard(magicShard, magicShardPath) );

    // open database within some transaction
    MojAssertNoErr( engine->beginTxn(txn) );
    MojAssertNoErr( engine->openExtDatabase("foo", txn.get(), db) );
    MojExpectNoErr( txn->commit() );

    MojBuffer buf;

    // write in some data
    MojAssertNoErr( buf.write("41", 2) );
    MojExpectNoErr( db->insert(obj("a"), buf, txn.get()) );

    buf.clear();
    MojAssertNoErr( buf.write("43", 2) );
    MojExpectNoErr( db->insert(magicShard, obj("b"), buf, txn.get()) );

    MojExpectNoErr( txn->commit() );

    MojRefCountedPtr<MojDbStorageItem> item;
    auto itemData = [&] {
        MojDbSandwichItem &sandwichItem = dynamic_cast<MojDbSandwichItem &>(*item);
        return std::string { (const char *)sandwichItem.data(), sandwichItem.size() };
    };

    // verify that we were able to write it

    txn.reset(); // re-create transaction
    MojAssertNoErr( engine->beginTxn(txn) );

    MojAssertNoErr( db->get(obj("a"), txn.get(), false, item) );
    EXPECT_EQ( "41", itemData() );
    MojAssertNoErr( db->get(magicShard, obj("b"), txn.get(), false, item) );
    EXPECT_EQ( "43", itemData() );
    MojAssertNoErr( db->get(obj("b"), txn.get(), false, item) );
    EXPECT_FALSE( bool(item.get()) ) << "Expect no item \"b\" in main shard";
    MojAssertNoErr( db->get(magicShard, obj("a"), txn.get(), false, item) );
    EXPECT_FALSE( bool(item.get()) ) << "Expect no item \"a\" in magic shard";

    // check unmount
    MojExpectNoErr( engine->unMountShard(magicShard) );
    MojAssertNoErr( db->get(obj("a"), txn.get(), false, item) );
    EXPECT_EQ( "41", itemData() );

    // next access to unmounted shard shouldn't cause segfault etc
    MojAssertNoErr( db->get(magicShard, obj("b"), txn.get(), false, item) );
    EXPECT_EQ( "43", itemData() );
}

struct SandwichTest : TestPath
{
    MojDb db;

    void SetUp() override
    {
        TestPath::SetUp();

        MojDbStorageEngine::setEngineFactory("sandwich");

        auto cfg = obj({
            { "db", obj({
                { "enable_sharding", MojObject(true) },
                //{ "shard_db_prefix", obj("shards") },
                { "shard_db_prefix", obj(test_path) },
                { "device_links_path", obj(test_path) },
                { "fallback_path", obj(test_path) },
                { "device_minimum_free_bytes", 100000000 }
            }) },
        });

        MojAssertNoErr( db.configure(cfg) );

        // open
        MojAssertNoErr( db.open(test_path.c_str()) );

        // setup some useful constants
        devicesStick = obj({{ "devices", arr({
            obj({
                { "deviceType", obj("usb") },
                { "deviceId", obj("stick") },
                { "subDevices", arr({
                    obj({
                        { "deviceId", obj("stick") },
                        { "deviceName", obj("stick") },
                        { "deviceUri", obj(test_path) },
                        }),
                    }) },
                }),
        })}});

        ASSERT_NO_FATAL_FAILURE( refreshActiveShards() );
    }

    void TearDown() override
    {
        MojAssertNoErr( db.close() );
    }

    void refreshActiveShards()
    {
        MojUInt32 activeShardsCount;
        MojAssertNoErr( db.shardEngine()->getAllActive(activeShards, activeShardsCount) );
        EXPECT_EQ( activeShardsCount, activeShards.size() );

        for (const auto &shardInfo : activeShards)
        {
            EXPECT_TRUE( shardInfo.active ) << "We should see only active shards";
        }
    }

    void plugIn(MojDbMediaHandler &media)
    {
        MojAssertNoErr( media.handleDeviceListResponse(devicesStick, MojErrNone) );
        ASSERT_NO_FATAL_FAILURE( refreshActiveShards() );
        EXPECT_EQ( 1u, activeShards.size() );
        ASSERT_FALSE( activeShards.empty() );
        EXPECT_STREQ( "stick", activeShards.front().deviceId );
        EXPECT_STREQ( "stick", activeShards.front().deviceName );
    }

    void plugOut(MojDbMediaHandler &media)
    {
        MojAssertNoErr( media.handleDeviceListResponse(devicesEmpty, MojErrNone) );
        ASSERT_NO_FATAL_FAILURE( refreshActiveShards() );
        EXPECT_EQ( 0u, activeShards.size() );
        ASSERT_TRUE( activeShards.empty() );
    }

    MojObject devicesEmpty = obj({{ "devices", arr({}) }});
    MojObject devicesStick; // filled in SetUp() and unique for each test
    std::list<MojDbShardInfo> activeShards;
};

TEST_F(SandwichTest, mount_unmount)
{
    MojDbMediaHandler media(db);

    EXPECT_TRUE( activeShards.empty() );

    ASSERT_NO_FATAL_FAILURE( plugIn(media) );
    ASSERT_NO_FATAL_FAILURE( plugIn(media) );
    ASSERT_NO_FATAL_FAILURE( plugOut(media) );
    ASSERT_NO_FATAL_FAILURE( plugIn(media) );
    ASSERT_NO_FATAL_FAILURE( plugOut(media) );
    ASSERT_NO_FATAL_FAILURE( plugOut(media) );
    ASSERT_NO_FATAL_FAILURE( plugIn(media) );
}

TEST_F(SandwichTest, mount_unmount_visibility)
{
    const char *kindId = "Test:1";
    //const char *kindDecl = "{\"id\":\"Test:1\", \"owner\": \"com.foo.bar\", \"indexes\":[{\"name\":\"foo\", \"props\":[{\"name\":\"bar\"}]}]}";
    const char *kindDecl = "{\"id\":\"Test:1\", \"owner\": \"com.foo.bar\"}";

    const auto put = [&](MojObject &obj, MojString shardIdStr = MojString()) {
        MojAssertNoErr( db.put(obj, MojDbFlagNone, MojDbReq(), shardIdStr) );
    };

    MojDbQuery query;
    MojAssertNoErr( query.from(kindId) );
    //MojAssertNoErr( query.where("foo", MojDbQuery::OpEq, 1) );

    bool watchFired = false;
    bool shardChanged = false;

    MojEasySlot<> watcher ([&] { watchFired = true; std::cout << "Watcher hit" << std::endl; return MojErrNone; });

    const auto shardStatusHandler = [&](const MojDbShardInfo &shardInfo) {
        shardChanged = true;
        std::cout << "Shard change " << shardInfo.id << ": active = " << shardInfo.active << std::endl;
        return MojErrNone;
    };
    MojEasySlot<const MojDbShardInfo &> shardStatusWatcher (shardStatusHandler);

    db.shardStatusChanged.connect(shardStatusWatcher.slot());

    const auto armWatch = [&] {
        watchFired = false;
        shardChanged = false;

        bool fired;
        MojDbQuery query;
        MojAssertNoErr( query.from(kindId) );
        MojDbCursor cursor;
        MojExpectNoErr( db.watch(query, cursor, watcher.slot(), fired) );
        MojAssertNoErr( cursor.close() );
    };

    MojDbMediaHandler media(db);

    // declare some kind
    auto kindObj = json(kindDecl);
    MojAssertNoErr( db.putKind(kindObj) );

    // setup watch
    MojDbCursor cursor;

    ASSERT_NO_FATAL_FAILURE( plugIn(media) );

    // put into first shard
    auto bar = obj({
                   { "_kind", obj(kindId) },
                   { "bar", 42 }
                   });

    ASSERT_NO_FATAL_FAILURE( armWatch() );
    std::cout << "Watch for put" << std::endl;
    ASSERT_NO_FATAL_FAILURE( put(bar, activeShards.front().id_base64) );
    EXPECT_TRUE( watchFired )
        << "Object insertion should notify watchers";
    EXPECT_FALSE( shardChanged )
        << "Object insertion doesn't change shards status";
    std::cout << "Watched" << std::endl;

    MojObject barId;
    EXPECT_TRUE( bar.get(MojDb::IdKey, barId) );

    MojObject barCopy;
    bool found = false;
    MojAssertNoErr( db.get(barId, barCopy, found) );
    ASSERT_TRUE( found );
    EXPECT_EQ( bar, barCopy );

    found = false;
    MojAssertNoErr( db.find(query, cursor) );
    MojAssertNoErr( cursor.get(barCopy, found) );
    EXPECT_TRUE( found );
    EXPECT_EQ( bar, barCopy );
    MojAssertNoErr( cursor.close() );

    // unplug shard
    ASSERT_NO_FATAL_FAILURE( plugOut(media) );

    MojExpectErr( MojErrDbInvalidShardId, db.get(barId, barCopy, found) )
        << "Direct request results in an error (different from search)";

    found = true;
    MojAssertNoErr( db.find(query, cursor) );
    MojAssertNoErr( cursor.get(barCopy, found) );
    EXPECT_FALSE( found );
    MojAssertNoErr( cursor.close() );

    // plug-in shard back
    ASSERT_NO_FATAL_FAILURE( armWatch() );
    EXPECT_FALSE( shardChanged );
    EXPECT_FALSE( watchFired ) << "No premature watch fire";
    ASSERT_NO_FATAL_FAILURE( plugIn(media) );
    EXPECT_TRUE( watchFired )
        << "Plugging in shard with matching object should notify watchers";
    EXPECT_TRUE( shardChanged )
        << "Shard mount should be visible";

    found = false;
    MojAssertNoErr( db.get(barId, barCopy, found) );
    ASSERT_TRUE( found );
    EXPECT_EQ( bar, barCopy );

    found = false;
    MojAssertNoErr( db.find(query, cursor) );
    MojAssertNoErr( cursor.get(barCopy, found) );
    EXPECT_TRUE( found );
    EXPECT_EQ( bar, barCopy );
    MojAssertNoErr( cursor.close() );
}

TEST_F(SandwichTest, parallel_mount_unmount)
{
    MojDbMediaHandler media(db);

    auto flip_devs =
        arr({
            obj({
                { "deviceType", obj("usb") },
                { "deviceId", obj("flip") },
                { "subDevices",
                    arr({
                        obj({
                            { "deviceId", obj("flip") },
                            { "deviceName", obj("flip") },
                            { "deviceUri", obj(test_path) },
                            })
                        }) },
                })
            });
    auto flip = obj({ { "devices", flip_devs } });
    auto flop_devs =
        arr({
            obj({
                { "deviceType", obj("usb") },
                { "deviceId", obj("flop") },
                { "subDevices",
                    arr({
                        obj({
                            { "deviceId", obj("flop") },
                            { "deviceName", obj("flop") },
                            { "deviceUri", obj(test_path) },
                            })
                        }) },
                })
            });
    auto flop = obj({ { "devices", flop_devs } });
    auto none = obj({ { "devices", arr({}) } });

    for (size_t i = 0; i < 100; ++i)
    {
        MojExpectNoErr( media.handleDeviceListResponse(flip, MojErrNone) );
        MojExpectNoErr( media.handleDeviceListResponse(flop, MojErrNone) );
        MojExpectNoErr( media.handleDeviceListResponse(none, MojErrNone) );
        MojExpectNoErr( media.handleDeviceListResponse(flip, MojErrNone) );
        MojExpectNoErr( media.handleDeviceListResponse(none, MojErrNone) );
        MojExpectNoErr( media.handleDeviceListResponse(flop, MojErrNone) );
    }

    const size_t nthreads = 4, nsteps = 10000;
    auto do_flip = [&] {
        for (size_t i = 0; i < nsteps; ++i)
        {
            MojExpectNoErr( media.handleDeviceListResponse(flip, MojErrNone) );
        }
    };
    auto do_flop = [&] {
        for (size_t i = 0; i < nsteps; ++i)
        {
            MojExpectNoErr( media.handleDeviceListResponse(flop, MojErrNone) );
        }
    };

    std::array<std::thread, nthreads> threads;
    bool is_flip = true;
    for(auto &thread : threads)
    {
        if (is_flip) thread = std::thread(do_flip);
        else thread = std::thread(do_flop);
        is_flip = !is_flip;
    }

    for(auto &thread : threads) thread.join();
}

TEST_F(SandwichTest, quota_balance)
{
    const char *kindId = "Test:1";
    const char *kindDecl = "{\"id\":\"Test:1\", \"owner\": \"com.foo.bar\"}";

    const auto kindUsage = [&] {
        MojInt64 usage = -1;
        MojRefCountedPtr<MojDbStorageTxn> txn;
        MojExpectNoErr( db.storageEngine()->beginTxn(txn) );
        MojExpectNoErr( db.quotaEngine()->kindUsage(kindId, usage, txn.get()) );
        return usage;
    };

    const auto put = [&](MojObject &obj, MojString shardIdStr = MojString()) {
        MojAssertNoErr( db.put(obj, MojDbFlagNone, MojDbReq(), shardIdStr) );
    };

    // we'll need it to mount/unmount shards
    MojDbMediaHandler media(db);

    // declare some kind
    auto kindObj = json(kindDecl);
    MojAssertNoErr( db.putKind(kindObj) );

    // ensure all kinds born with zero usage
    ASSERT_EQ( 0, kindUsage() );

    // check that kind mount brings no additional kind usage
    MojAssertNoErr( media.handleDeviceListResponse(devicesStick, MojErrNone) );
    ASSERT_EQ( 0, kindUsage() );

    // get some shardInfo
    std::list<MojDbShardInfo> shards;
    MojUInt32 count;
    MojAssertNoErr( db.shardEngine()->getAllActive(shards, count) );
    ASSERT_EQ( count, shards.size() );
    EXPECT_EQ( 1u, shards.size() );

    // put something into main shard
    auto foo = obj({
                   { "_kind", obj(kindId) },
                   { "foo", 42 }
                   });
    ASSERT_NO_FATAL_FAILURE( put(foo) );

    auto baseUsage = kindUsage();
    EXPECT_GT( baseUsage, 0 );

    // put into first shard
    auto bar = obj({
                   { "_kind", obj(kindId) },
                   { "bar", 42 }
                   });
    ASSERT_NO_FATAL_FAILURE( put(bar, shards.front().id_base64) );

    // unplug stick (unmount shard)
    MojAssertNoErr( media.handleDeviceListResponse(devicesEmpty, MojErrNone) );

    EXPECT_EQ( baseUsage, kindUsage() ) << "Quota free space shouldn't leak away with shard unmount";;
}
