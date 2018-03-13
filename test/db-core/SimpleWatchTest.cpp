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

#include "db/MojDbEasySignal.h"
#include "db/MojDb.h"

#include "MojDbCoreTest.h"

namespace {

const char* const MojKindId = "Test:1";
const char* const MojKindStr =
    "{\"id\":\"Test:1\","
    "\"owner\":\"mojodb.admin\","
    "\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},{\"name\":\"barfoo\",\"props\":[{\"name\":\"bar\"},{\"name\":\"foo\"}]}]}";

const char* const objMatch =
    "{\"_kind\":\"Test:1\","
    "\"foo\": 30}";

} // anonymous


struct SimpleWatchTest : public MojDbCoreTest
{
    void SetUp() override
    {
        MojDbCoreTest::SetUp();

        // add kind
        MojObject obj;
        MojAssertNoErr( obj.fromJson(MojKindStr) );
        MojAssertNoErr( db.putKind(obj) );
    }

    void putJson(const char *json, MojDbReqRef req = MojDbReq())
    {
        MojObject obj;
        MojAssertNoErr( obj.fromJson(json) );
        MojAssertNoErr( db.put(obj, MojDbFlagNone, req) ) << "Should successfully put object: " << json;
    }

    void watchQuery(MojDb::WatchSignal::SlotRef slot, bool &fired, MojDbReqRef req = MojDbReq())
    {
        MojDbQuery query;
        MojAssertNoErr( query.from(_T("Test:1")) );
        MojAssertNoErr( query.where(_T("foo"), MojDbQuery::OpLessThan, 50) );

        MojDbCursor cursor;

        MojAssertNoErr( db.watch(query, cursor, slot, fired, req) );
    }
};

TEST_F(SimpleWatchTest, watch_nodata)
{
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    EXPECT_FALSE( signaled );
}

TEST_F(SimpleWatchTest, watch_cancel)
{
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    easySlot.cancel();

    EXPECT_FALSE( signaled );
}

TEST_F(SimpleWatchTest, put_commit_watch)
{
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch) );

    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    EXPECT_TRUE( signaled );
}

TEST_F(SimpleWatchTest, watch_put_commit)
{
    MojDbReq req;
#ifdef LMDB_ENGINE_SUPPORT
    MojAssertNoErr( req.begin(&db, true) );

    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled, req) );
#else
    MojAssertNoErr( req.begin(&db, false) );

    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );
#endif

    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );

    MojAssertNoErr( req.end(true) );

    EXPECT_TRUE( signaled );
}

TEST_F(SimpleWatchTest, watch_put_abort)
{
#ifdef LMDB_ENGINE_SUPPORT
    MojDbReq req;
    MojAssertNoErr( req.begin(&db, true) );
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled, req) );

    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );
    MojAssertNoErr( req.end(false) );

    EXPECT_FALSE( signaled );
#else
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    MojDbReq req;
    MojAssertNoErr( req.begin(&db, false) );
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );
    MojAssertNoErr( req.end(false) );

    EXPECT_FALSE( signaled );
#endif
}

#ifdef LMDB_ENGINE_SUPPORT
TEST_F(SimpleWatchTest, put_watch_commit)
{
    MojDbReq req;
    MojAssertNoErr( req.begin(&db, true) );
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );

    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });
    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled, req) );

    MojAssertNoErr( req.end(true) );

    EXPECT_TRUE( signaled );
}
#else
TEST_F(SimpleWatchTest, put_watch_commit)
{
    MojDbReq req;
    MojAssertNoErr( req.begin(&db, false) );
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });
    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );
    MojAssertNoErr( req.end(true) );
    EXPECT_TRUE( signaled );
}
#endif

TEST_F(SimpleWatchTest, put_watch_abort)
{
    MojDbReq req;
#ifdef LMDB_ENGINE_SUPPORT
    MojAssertNoErr( req.begin(&db, true) );
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );
    MojAssertNoErr( req.abort() );
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );
#else
    MojAssertNoErr( req.begin(&db, false) );
    ASSERT_NO_FATAL_FAILURE( putJson(objMatch, req) );

    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    MojAssertNoErr( req.end(false) );
#endif
    EXPECT_FALSE( signaled )
        << "We are pretty smart to detect that aborted transaction have no effect on query results";
}

TEST_F(SimpleWatchTest, watch_delKind)
{
    bool signaled = false;
    MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

    ASSERT_NO_FATAL_FAILURE( watchQuery(easySlot.slot(), signaled) );

    bool foundKind;
    MojString kindStr;
    MojAssertNoErr( kindStr.assign(MojKindId) );
    MojAssertNoErr( db.delKind(kindStr, foundKind) );
    ASSERT_TRUE( foundKind );

    EXPECT_TRUE( signaled ) << "Kind/Index removal usually have effect on query results";
}
