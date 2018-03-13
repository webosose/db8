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
#include <thread>

#include "core/MojGmainReactor.h"
#include "core/MojThread.h"
#include "core/MojObjectBuilder.h"

#include "db/MojDbSearchCursor.h"
#include "db/MojDbServiceHandler.h"
#include "db/MojDb.h"
#include "db/MojDbDefs.h"
#include "db/MojDbServiceClient.h"

#include "MojFakeService.h"
#include "MojDbClientTestResultHandler.h"
#include "MojDbClientTestWatcher.h"
#include "Runner.h"
#include "CpuMemStats.h"

#include "gtest/gtest.h"

extern const char *g_temp_folder;
extern const char *g_dump_path;
extern const MojChar * const g_moj_test_kind;
// Number of threads to start in Concurrent tests
extern size_t g_nthreads;
// Number of steps per each thread
extern size_t g_nsteps;
// Number of iterations for Sequential tests
extern size_t g_counter;

#define START_WORKER_THREADS_WITH_ARG(WORKER_FUNC, WORKER_ARG) do {std::vector<std::thread> threads;\
threads.resize(g_nthreads);\
for (size_t worker = 0; worker < threads.size(); ++worker) {\
    threads[worker] = std::thread(WORKER_FUNC, WORKER_ARG);\
}\
for(auto &thread : threads) {\
    thread.join();\
}\
} while (0)

struct DbClientPerfBase : public ::testing::Test
{

    void init();
    void registerType();
    MojErr writeTestObj(const MojChar* json, MojObject* idOut = NULL);
    MojErr writeTestObj(const MojChar* json, bool kind, MojObject* idOut = NULL);
    MojAutoPtr<MojService> m_service;
    MojAutoPtr<MojDbServiceClient> m_dbClient;
    MojAutoPtr<MojReactor> m_reactor;
    MojDb m_db;
    MojRefCountedPtr<MojDbServiceHandler> m_handler;
    MojThreadMutex m_mutex;
};

struct DbClientPerfTest : public DbClientPerfBase
{
    virtual void SetUp()
    {
        init();
        registerType();
    }
    virtual void TearDown()
    {
        MojExpectNoErr(m_db.close());
        MojExpectNoErr(MojRmDirRecursive(g_temp_folder));
    }

    MojErr testPut(size_t ninterations);
};

struct DbClientPerfTestDumpAware : public DbClientPerfBase
{
    virtual void SetUp()
    {
        init();
        if (g_dump_path) {
            MojChar *path = (MojChar *) g_dump_path;
            MojUInt32 count = (MojUInt32) g_counter;
            MojErr err = m_db.load(path, count);
            MojAssertNoErr(err);
        } else {
            registerType();
        }
    }
    virtual void TearDown()
    {
        MojExpectNoErr(m_db.close());
        MojExpectNoErr(MojRmDirRecursive(g_temp_folder));
    }
};

void DbClientPerfBase::registerType()
{
    // register type
    MojErr err = writeTestObj(g_moj_test_kind, true);
    MojAssertNoErr(err);
}

void DbClientPerfBase::init()
{
    if (!m_dbClient.get()) {
        MojAutoPtr<MojGmainReactor> reactor(new MojGmainReactor);
        MojAssert(reactor.get());
        MojErr err = reactor->init();
        MojAssertNoErr(err);

        err = m_db.open(g_temp_folder);
        MojAssertNoErr(err);

        m_handler.reset(new MojDbServiceHandler(m_db, *m_reactor.get()));
        MojAssert(m_handler.get() != NULL);

        MojAutoPtr<MojFakeService> svc(new MojFakeService);

        MojAssert(svc.get());
        err = svc->open(_T("com.palm.mojfakeluna"));
        MojAssertNoErr(err);
        err = svc->attach(reactor->impl());
        MojAssertNoErr(err);

        m_reactor = reactor;
        m_service = svc;

        // open handler
        err = m_handler->open();
        MojAssertNoErr(err);
        err = m_service->addCategory(MojDbServiceDefs::Category, m_handler.get());
        MojAssertNoErr(err);

        m_dbClient.reset(new MojDbServiceClient(m_service.get()));
        MojAssert(m_dbClient.get());
    }
}

MojErr DbClientPerfBase::writeTestObj(const MojChar* json, MojObject* idOut)
{
    return writeTestObj(json, false, idOut);
}

MojErr DbClientPerfBase::writeTestObj(const MojChar* json, bool kind, MojObject* idOut)
{
    // not a test, per say, but a utility to populate the datastore for performing specific tests

    MojObject obj;
    bool retb;
    MojErr err = obj.fromJson(json);
    MojExpectNoErr(err);

    MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
    MojAssert(handler.get() != NULL);

    if (kind) {
        err = m_dbClient->putKind(handler->m_slot, obj);
        MojExpectNoErr(err);
    } else {
        err = m_dbClient->put(handler->m_slot, obj);
        MojExpectNoErr(err);
    }

    // block until repsonse received
    err = handler->wait(m_dbClient->service());
    MojExpectNoErr(err);
    // verify result
    MojExpectErr(handler->m_dbErr, MojErrNone);
    MojObject results;
    if (!kind) {
        retb = handler->m_result.get(_T("results"), results);
        MojAssert(retb);

        MojObject::ConstArrayIterator iter = results.arrayBegin();
        retb = (iter != results.arrayEnd());
        MojAssert(retb);

        MojObject id;
        err = iter->getRequired(_T("id"), id);
        MojExpectNoErr(err);
        MojAssert(!id.undefined());
        if (idOut) {
            *idOut = id;
        }
    }

    return MojErrNone;
}

/* Testing a single put operation performance
*  1. Put an object to database
*  2. Get a response, check for 'id' and 'rev' fields
*/
MojErr DbClientPerfTest::testPut(size_t ninterations)
{
    for (size_t i = 0; i < ninterations; i++){
        MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
        MojAssert(handler.get() != NULL);

        MojObject obj;
        MojErr err;
        bool retb;

        err = obj.putString(_T("_kind"), _T("LunaDbClientTest:1"));
        MojExpectNoErr(err);
        err = obj.putString(_T("foo"), _T("test_foo"));
        MojExpectNoErr(err);
        err = obj.putString(_T("bar"), _T("test_bar"));
        MojExpectNoErr(err);

        err = m_dbClient->put(handler->m_slot, obj);
        MojExpectNoErr(err);

        // block until repsonse received
        err = handler->wait(m_dbClient->service());
        MojExpectNoErr(err);

        // verify result
        MojExpectNoErr(handler->m_dbErr);
        MojObject results;
        MojString  s;
        err = handler->m_result.toJson(s);
        MojExpectNoErr(err);

        retb = handler->m_result.get(_T("results"), results);
        MojAssert(retb);
        MojObject::ConstArrayIterator iter = results.arrayBegin();
        retb = (iter != results.arrayEnd());
        MojAssert(retb);

        retb = (iter->find("id") != iter->end());
        MojAssert(retb);
        retb = (iter->find("rev") != iter->end());
        MojAssert(retb);

        // verify only 1 result
        iter++;
        retb = (iter == results.arrayEnd());
        MojAssert(retb);
    }
    return MojErrNone;
}

TEST_F(DbClientPerfTest, testPut)
{
    CpuStat c("testPut");
    MemStat m("testPut");
    MojAssertErr(testPut(1), MojErrNone);
}

/* Testing a single get operation performance
*  1. Put a test object to database, save its id
*  2. Get an object from database by id, check for correct 'id' field
*/
TEST_F(DbClientPerfTest, testGet)
{
    CpuStat c("testGet");
    MemStat m("testGet");
    // write an object
    MojObject id;
    bool retb;
    MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
    MojAssertNoErr(err);

    MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
    MojAssert(handler.get() != NULL);

    MojObject idObj(id);
    err = m_dbClient->get(handler->m_slot, idObj);
    MojAssertNoErr(err);

    // block until repsonse received
    err = handler->wait(m_dbClient->service());
    MojAssertNoErr(err);

    // verify result
    MojAssertErr(handler->m_dbErr, MojErrNone);
    MojObject results;
    retb = handler->m_result.get(_T("results"), results);
    MojAssert(retb);

    MojObject::ConstArrayIterator iter = results.arrayBegin();
    retb = (iter != results.arrayEnd());
    MojAssert(retb);
    MojObject idReturned = -1;
    err = iter->getRequired("_id", idReturned);
    MojAssertNoErr(err);
    MojAssert(idReturned == id);
    retb = (iter->find("_rev") != iter->end());
    MojAssert(retb);

    MojString foo;
    err = iter->getRequired(_T("foo"), foo);
    MojAssertNoErr(err);
    MojAssert(foo == _T("test_foo"));

    MojString bar;
    err = iter->getRequired(_T("bar"), bar);
    MojAssertNoErr(err);
    MojAssert(bar == _T("test_bar"));

    // verify only 1 result
    iter++;
    MojAssert(iter == results.arrayEnd());
}

/* Testing a single del operation performance
*  1. Put a test object to database, save its id
*  2. Delete an object from database by id, check for correct 'id' and 'rev' fields
*/
TEST_F(DbClientPerfTest, testDel)
{
    CpuStat c("testDel");
    MemStat m("testDel");
    // write an object
    MojObject id;
    bool retb;
    MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
    MojAssertNoErr(err);

    MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
    MojAssert(handler.get() != NULL);

    // delete
    MojObject idObj(id);
    err = m_dbClient->del(handler->m_slot, idObj, MojDbFlagForceNoPurge);
    MojAssertNoErr(err);

    // block until repsonse received
    err = handler->wait(m_dbClient->service());
    MojAssertNoErr(err);

    // verify result
    MojAssertNoErr(handler->m_dbErr);
    MojObject results;
    retb = handler->m_result.get(_T("results"), results);
    MojAssert(retb);

    MojObject::ConstArrayIterator iter = results.arrayBegin();
    MojAssert(iter != results.arrayEnd());
    retb = (iter->find("id") != iter->end());
    MojAssert(retb);
    retb = (iter->find("rev") != iter->end());
    MojAssert(retb);

    // verify only 1 result
    iter++;
    MojAssert(iter == results.arrayEnd());
}

/* Testing multiple sequential put operations performance
*  Run testPut multiple times
*/
TEST_F(DbClientPerfTest, testPutSequential)
{
    CpuStat c("testPutSequential");
    MemStat m("testPutSequential");
    MojErr err = testPut(g_counter);
    MojAssertNoErr(err);
}

/* FIX: test case hangs up */
/* Testing multiple concurrent put operations performance
*  1. Put a test object to database, save its rev as baseRev
*  2. Create threads and put some objects to database
*  3. Put another test object to database, save is rev and
*     calculate the difference of rev - baseRevq
*  4. Check that value of (rev - baseRev) is equal to amount
*     of objects put into database by threads
*/
TEST_F(DbClientPerfTest, DISABLED_testPutConcurrent)
{
    CpuStat c("testPutConcurrent");
    MemStat m("testPutConcurrent");
    MojErr err;
    MojObject obj;
    err = obj.putString(MojDb::KindKey, "LunaDbClientTest:1");
    MojAssertNoErr(err);
    err = obj.putInt("foo", -1);
    MojAssertNoErr(err);
    err = m_db.put(obj);

    MojAssertNoErr(err);
    MojObject baseId, id;
    MojInt64 baseRev, rev;

    // put base object
    err = obj.getRequired(MojDb::IdKey, baseId);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::RevKey, baseRev);
    MojAssertNoErr(err);

    // check that we can read it
    bool found = false;
    err = m_db.get(baseId, obj, found);
    MojAssertNoErr(err);
    MojAssert( found );
    err = obj.getRequired(MojDb::IdKey, id);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::RevKey, rev);
    MojAssertNoErr(err);
    MojAssert( baseId == id );
    MojAssert( baseRev == rev );

    // put it back and notice revision bump
    err = obj.putInt("foo", -2);
    MojAssertNoErr(err);
    err = m_db.put(obj);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::IdKey, baseId);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::RevKey, baseRev);
    MojAssertNoErr(err);
    MojAssert( id == baseId );
    MojAssert( rev+1 == baseRev );

    const auto f = [&](size_t worker) {
        MojObject obj;
        MojErr err;

        err = obj.putString(_T("_kind"), _T("LunaDbClientTest:1"));
        MojAssertNoErr(err);
        err = obj.putInt(_T("worker"), worker);
        MojAssertNoErr(err);

        for (size_t n = 0; n < g_nsteps; ++n)
        {
            MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
            MojAssert(handler.get() != NULL);

            err = obj.putInt("iteration", n);
            MojAssertNoErr(err);
            err = m_dbClient->put(handler->m_slot, obj);
            MojAssertNoErr(err);

            err = handler->wait(m_dbClient->service());
            MojAssertNoErr(err);
        }
    };

    START_WORKER_THREADS_WITH_ARG(f, worker);

    // put our object again to bump revision once more time and verify it
    err = obj.putInt("foo", -3);
    MojAssertNoErr(err);
    err = m_db.put(obj);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::IdKey, id);
    MojAssertNoErr(err);
    err = obj.getRequired(MojDb::RevKey, rev);
    MojAssertNoErr(err);
    MojAssert( baseId == id );
    ASSERT_EQ( baseRev + MojInt64(g_nthreads * g_nsteps) + 1, rev )
        << "Expected " << g_nthreads * g_nsteps << " revision bumps from " << g_nthreads << " threads"
        << " and one extra bump for verification";
}

/* Testing multiple sequential get operations performance
*  1. From dataset written to database, get all the objects in a loop
*/
TEST_F(DbClientPerfTestDumpAware, testGetSequential)
{
    // write an object
    MojErr err;
    bool retb;
    std::vector<MojObject> ids_vec;

    if (g_dump_path) {
        for (size_t i = 0; i < g_counter; i++) {
            MojObject id(MojUInt32(i + 1));
            ids_vec.push_back(id);
        }
    } else {
        for (size_t i = 0; i < g_counter; i++) {
            MojObject id;
            err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
            MojAssertNoErr(err);
            ids_vec.push_back(id);
        }
    }

    CpuStat c("testGetSequential");
    MemStat m("testGetSequential");
    for (size_t i = 0; i < g_counter; i++) {
        MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
        MojAssert(handler.get() != NULL);

        MojObject id = ids_vec[i];

        MojObject idObj(id);
        err = m_dbClient->get(handler->m_slot, idObj);
        MojAssertNoErr(err);

        // block until repsonse received
        err = handler->wait(m_dbClient->service());
        MojAssertNoErr(err);

        // verify result

        MojAssertNoErr(handler->m_dbErr);
        MojObject results;
        retb = handler->m_result.get(_T("results"), results);
        MojAssert(retb);

        MojObject::ConstArrayIterator iter = results.arrayBegin();
        MojAssert(iter != results.arrayEnd());
        MojObject idReturned = -1;
        err = iter->getRequired("_id", idReturned);
        MojAssertNoErr(err);
        MojAssert(idReturned == id);
        retb = (iter->find("_rev") != iter->end());
        MojAssert(retb);

        MojString foo;
        err = iter->getRequired(_T("foo"), foo);
        MojAssertNoErr(err);
        MojAssert(foo == _T("test_foo"));

        MojString bar;
        err = iter->getRequired(_T("bar"), bar);
        MojAssertNoErr(err);
        MojAssert(bar == _T("test_bar"));

        // verify only 1 result
        iter++;
        MojAssert(iter == results.arrayEnd());
    }
}

/* FIX: test case hangs up */
/* Testing multiple concurrent get operations performance
*  1. From dataset written to database, get all the objects in multiple threads
*/
TEST_F(DbClientPerfTestDumpAware, DISABLED_testGetConcurrent)
{
    // write an object
    MojErr err;
    std::vector<MojObject> ids_vec;
    for (size_t i = 0; i < g_counter; i++) {
        MojObject id;
        err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
        MojAssertNoErr(err);
        ids_vec.push_back(id);
    }
    CpuStat c("testGetConcurrent");
    MemStat m("testGetConcurrent");
    const auto f = [&](size_t unused) {
        for (size_t i = 0; i < g_nsteps; i++) {
            MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
            MojAssert(handler.get() != NULL);

            MojObject id = ids_vec.front();

            MojObject idObj(id);
            err = m_dbClient->get(handler->m_slot, idObj);
            MojAssertNoErr(err);

            // block until repsonse received
            err = handler->wait(m_dbClient->service());
            MojAssertNoErr(err);

            // verify result
            MojAssertErr(handler->m_dbErr, MojErrNone);
            MojObject results;
            MojAssert(handler->m_result.get(_T("results"), results));

            MojObject::ConstArrayIterator iter = results.arrayBegin();
            MojAssert(iter != results.arrayEnd());
            MojObject idReturned = -1;
            MojAssertErr(iter->getRequired("_id", idReturned), MojErrNone);
            MojAssert(idReturned == id);
            MojAssert(iter->find("_rev") != iter->end());

            MojString foo;
            MojAssertErr(iter->getRequired(_T("foo"), foo), MojErrNone);
            MojAssert(foo == _T("test_foo"));

            MojString bar;
            MojAssertErr(iter->getRequired(_T("bar"), bar), MojErrNone);
            MojAssert(bar == _T("test_bar"));

            // verify only 1 result
            iter++;
            MojAssert(iter == results.arrayEnd());
        }
    };
    START_WORKER_THREADS_WITH_ARG(f, 0 /*unused*/);
}

/* Testing multiple sequential delete operations performance
*  1. From dataset written to database, delete all the objects in a loop
*/
TEST_F(DbClientPerfTestDumpAware, testDelSequential)
{
    std::vector<MojObject> ids_vec;
    bool retb;

    if (g_dump_path) {
        for (size_t i = 0; i < g_counter; i++) {
            MojObject id(MojUInt32(i + 1));
            ids_vec.push_back(id);
        }
    } else {
        for (size_t i = 0; i < g_counter; i++) {
            MojObject id;
            MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
            MojAssertNoErr(err);
            ids_vec.push_back(id);
        }
    }
    CpuStat c("testDelSequential");
    MemStat m("testDelSequential");
    for (size_t i = 0; i < g_counter; i++) {
        MojObject id = ids_vec[i];
        MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
        MojAssert(handler.get() != NULL);

        // delete
        MojObject idObj(id);
        MojErr err = m_dbClient->del(handler->m_slot, idObj, MojDbFlagForceNoPurge);
        MojAssertNoErr(err);

        // block until repsonse received
        err = handler->wait(m_dbClient->service());
        MojAssertNoErr(err);

        // verify result
        MojAssertErr(handler->m_dbErr, MojErrNone);
        MojObject results;
        retb = handler->m_result.get(_T("results"), results);
        MojAssert(retb);

        MojObject::ConstArrayIterator iter = results.arrayBegin();
        retb = (iter != results.arrayEnd());
        MojAssert(retb);
        retb = (iter->find("id") != iter->end());
        MojAssert(retb);
        retb = (iter->find("rev") != iter->end());
        MojAssert(retb);

        // verify only 1 result
        iter++;
        MojAssert(iter == results.arrayEnd());
    }
}


/* FIX: test case hangs up */
/* Testing multiple concurrent delete operations performance
*  1. From dataset written to database, delete all the objects in multiple threads
*/
TEST_F(DbClientPerfTestDumpAware, DISABLED_testDelConcurrent)
{
    std::vector<MojObject> ids_vec;
    for (size_t i = 0; i < g_counter; i++) {
        MojObject id;
        MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"})"), &id);
        MojAssertNoErr(err);
        ids_vec.push_back(id);
    }
    CpuStat c("testDelConcurrent");
    MemStat m("testDelConcurrent");
    const auto f = [&](size_t unused) {
        for (size_t i = 0; i < g_nsteps; i++) {
            MojObject id = ids_vec.front();
            MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
            MojAssert(handler.get() != NULL);

            // delete
            MojObject idObj(id);
            MojErr err = m_dbClient->del(handler->m_slot, idObj, MojDbFlagForceNoPurge);
            MojAssertNoErr(err);

            // block until repsonse received
            err = handler->wait(m_dbClient->service());
            MojAssertNoErr(err);

            // verify result
            MojAssertNoErr(handler->m_dbErr);
            MojObject results;
            MojAssert(handler->m_result.get(_T("results"), results));

            MojObject::ConstArrayIterator iter = results.arrayBegin();
            MojAssert(iter != results.arrayEnd());
            MojAssert(iter->find("id") != iter->end());
            MojAssert(iter->find("rev") != iter->end());

            // verify only 1 result
            iter++;
            MojAssert(iter == results.arrayEnd());
        }
    };

    START_WORKER_THREADS_WITH_ARG(f, 0 /*unused*/);
}
