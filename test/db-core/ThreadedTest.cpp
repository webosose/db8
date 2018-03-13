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

#include "MojDbCoreTest.h"

#include <array>
#include <thread>
#include <mutex>

struct ThreadedTest : MojDbCoreTest
{
    static constexpr const char * const kindId = "Test:1";
    static constexpr const char * const kindDef =
        "{\"id\":\"Test:1\", \"owner\":\"mojodb.admin\","
        "\"indexes\":[{\"name\":\"x\",\"props\":[{\"name\":\"x\"}]}]"
        "}";

    void SetUp()
    {
        MojDbCoreTest::SetUp();

        // add kind
        MojObject kindObj;
        MojAssertNoErr( kindObj.fromJson(kindDef) );
        MojAssertNoErr( db.putKind(kindObj) );
    }
};

TEST_F(ThreadedTest, revision)
{
    const size_t nthreads = 8, nsteps = 1000;

    MojObject obj;
    MojAssertNoErr( obj.putString(MojDb::KindKey, "Test:1") );
    MojAssertNoErr( obj.putInt("foo", -1) );
    MojAssertNoErr( db.put(obj) );

    MojObject baseId, id;
    MojInt64 baseRev, rev;

    // put base object
    MojAssertNoErr( obj.getRequired(MojDb::IdKey, baseId) );
    MojAssertNoErr( obj.getRequired(MojDb::RevKey, baseRev) );

    // check that we can read it
    bool found = false;
    MojAssertNoErr( db.get(baseId, obj, found) );
    ASSERT_TRUE( found );
    MojAssertNoErr( obj.getRequired(MojDb::IdKey, id) );
    MojAssertNoErr( obj.getRequired(MojDb::RevKey, rev) );
    ASSERT_EQ( baseId, id );
    ASSERT_EQ( baseRev, rev );

    // put it back and notice revision bump
    MojAssertNoErr( obj.putInt("foo", -2) );
    MojAssertNoErr( db.put(obj) );
    MojAssertNoErr( obj.getRequired(MojDb::IdKey, baseId) );
    MojAssertNoErr( obj.getRequired(MojDb::RevKey, baseRev) );
    ASSERT_EQ( id, baseId );
    ASSERT_EQ( rev+1, baseRev ) << "Expected revision bump";

    // now lets check that rule in threads
    const auto f = [&](size_t worker) {
        MojObject obj;
        MojAssertNoErr( obj.putString(MojDb::KindKey, "Test:1") );
        MojAssertNoErr( obj.putInt("worker", worker) );
        for (size_t n = 0; n < nsteps; ++n)
        {
            MojAssertNoErr( obj.putInt("iteration", n) );
            MojAssertNoErr( db.put(obj) );
        }
    };

    std::array<std::thread, nthreads> threads;
    for (size_t worker = 0; worker < threads.size(); ++worker)
    {
        threads[worker] = std::thread(f, worker);
    }

    for(auto &thread : threads) thread.join();

    // put our object again to bump revision once more time and verify it
    MojAssertNoErr( obj.putInt("foo", -3) );
    MojAssertNoErr( db.put(obj) );
    MojAssertNoErr( obj.getRequired(MojDb::IdKey, id) );
    MojAssertNoErr( obj.getRequired(MojDb::RevKey, rev) );

    ASSERT_EQ( baseId, id );
    EXPECT_EQ( baseRev + MojInt64(nthreads * nsteps) + 1, rev )
        << "Expected " << nthreads * nsteps << " revision bumps from " << nthreads << " threads"
        << " and one extra bump for verification";
}

TEST_F(ThreadedTest, watch_cancel_vs_release)
{
    const size_t nthreads = 64, ntries = 1000;

    pthread_barrier_t barrier;

    typedef MojVector<MojRefCountedPtr<MojDbWatcher> > WatcherVec;
    WatcherVec watchers;
    std::mutex watchers_mutex;

    const auto f = [&](const size_t worker) {
        bool signaled = false;
        MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

        MojRefCountedPtr<MojDbWatcher> watcher(new MojDbWatcher(easySlot.slot()));

        watchers_mutex.lock();
        MojExpectNoErr( watchers.push(watcher.get()) );
        watcher.reset();
        watchers_mutex.unlock();

        (void) pthread_barrier_wait(&barrier);

        easySlot.cancel();
    };

    for (size_t ntry = 0; ntry < ntries; ++ntry)
    {
        ASSERT_EQ( 0, pthread_barrier_init(&barrier, nullptr, nthreads+1) );

        std::array<std::thread, nthreads> threads;
        for (size_t worker = 0; worker < threads.size(); ++worker)
        {
            threads[worker] = std::thread(f, worker);
        }

        (void) pthread_barrier_wait(&barrier);

        watchers.clear();

        for(auto &thread : threads) thread.join();

        EXPECT_EQ( 0, pthread_barrier_destroy(&barrier) );
    }
}

TEST_F(ThreadedTest, watch_cancel_vs_delKind)
{
    const size_t nthreads = 64, ntries = 100;

    pthread_barrier_t barrier;

    const auto f = [&](const size_t worker) {
        bool signaled = false;
        MojEasySlot<> easySlot([&]() { signaled = true; return MojErrNone; });

        MojDbQuery query;
        MojAssertNoErr( query.from("Test:1") );
        MojAssertNoErr( query.where("x", MojDbQuery::OpLessThan, 50) );

        MojDbCursor cursor;

        MojAssertNoErr( db.watch(query, cursor, easySlot.slot(), signaled) );

        (void) pthread_barrier_wait(&barrier);

        easySlot.cancel();
    };

    for (size_t ntry = 0; ntry < ntries; ++ntry)
    {
        ASSERT_EQ( 0, pthread_barrier_init(&barrier, nullptr, nthreads+1) );

        std::array<std::thread, nthreads> threads;
        for (size_t worker = 0; worker < threads.size(); ++worker)
        {
            threads[worker] = std::thread(f, worker);
        }

        bool foundKind;
        MojString kindStr;
        MojAssertNoErr( kindStr.assign(kindId) );

        (void) pthread_barrier_wait(&barrier);

        MojAssertNoErr( db.delKind(kindStr, foundKind) );
        EXPECT_TRUE( foundKind );

        for(auto &thread : threads) thread.join();

        EXPECT_EQ( 0, pthread_barrier_destroy(&barrier) );

        // restore kind
        MojObject kindObj;
        MojAssertNoErr( kindObj.fromJson(kindDef) );
        MojAssertNoErr( db.putKind(kindObj) );
    }
}
