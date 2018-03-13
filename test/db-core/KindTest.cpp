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

#include <core/MojObjectBuilder.h>
#include <db/MojDbSearchCursor.h>

#include <db/MojDbKind.h>
#include <db/MojDbKindEngine.h>
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

    const MojChar* const MojTestKind1Str3 =
    _T("{\"id\":\"Test:1\",")
    _T("\"owner\":\"com.foo.bar\",")
    _T("\"indexes\":[")
    _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]},")
    _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]}")
    _T("]}");

    const MojChar* const MojTestKind2Str1 =
    _T("{\"id\":\"Test:2\",")
    _T("\"owner\":\"com.foo.bar\",")
    _T("\"indexes\":[")
    _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\"}]},")
    _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]}")
    _T("]}");


    const MojUInt32 MagicId = 42u;
}

/**
 * Test fixture for tests around multi-shard logic
 */
struct KindTest : public MojDbCoreTest
{
    MojDbIdGenerator idGen;
    MojDbShardEngine *shardEngine;
    MojString shardPath;
    MojString magicShardPath;

    void SetUp()
    {
        MojDbCoreTest::SetUp();

        shardEngine = db.shardEngine();

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
        MojAssertNoErr( shardEngine->put( shardInfo ) );

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


    void getHash(const MojChar* _id, const MojChar* _schema, MojSize* hash)
    {
        MojErr err;
        MojDbReq req;
        MojString locale;

        err = req.begin(&db, true);
        MojAssertNoErr(err);

        MojString id;
        err = id.assign(_id);
        MojAssertNoErr(err);

        MojRefCountedPtr<MojDbKind> kind (new MojDbKind(db.storageDatabase(), db.kindEngine(), true));
        ASSERT_TRUE(kind.get());

        err = kind->init(id);
        MojAssertNoErr(err);

        MojObject schema;
        err = schema.fromJson(_schema);
        MojAssertNoErr(err);

        err = kind->configure(schema, db.kindEngine()->kindMap(), locale, req);
        MojAssertNoErr(err);

        *hash = kind->hash();
    }
};



TEST_F(KindTest, hash_generate_logic)
{
    MojSize hash1;
    MojSize hash2;
    MojSize hash3;

    getHash(_T("Test:1"), MojTestKind1Str1, &hash1);
    getHash(_T("Test:2"), MojTestKind1Str2, &hash2);
    getHash(_T("Test:1"), MojTestKind1Str3, &hash3);

    ASSERT_TRUE(hash1);
    ASSERT_TRUE(hash2);
    ASSERT_TRUE(hash3);

    ASSERT_NE(hash1, hash2);
    ASSERT_EQ(hash1, hash3);
}

TEST_F(KindTest, hash_kind)
{
    MojErr err;
    MojObject obj;

    err = obj.fromJson(MojTestKind2Str1);
    MojAssertNoErr(err);

    err = db.putKind(obj);
    MojAssertNoErr(err);

    MojDbKind* kind;
    err = db.kindEngine()->getKind(_T("Test:2"), kind);
    MojAssertNoErr(err);
    ASSERT_TRUE(kind);

    MojSize hash1 = kind->hash();
    EXPECT_TRUE(hash1);

    err = db.close();
    MojAssertNoErr(err);

    MojDb db1;
    err = db1.open(path.c_str());
    MojAssertNoErr(err);

    MojDbKind* kind2;
    db1.kindEngine()->getKind(_T("Test:2"), kind2);
    ASSERT_TRUE(kind2);

    MojSize hash2 = kind2->hash();
    EXPECT_TRUE(hash2);

    EXPECT_EQ(hash1, hash2);

}
