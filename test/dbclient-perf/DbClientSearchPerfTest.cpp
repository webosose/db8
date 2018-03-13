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
#include <sstream>

#include "core/MojGmainReactor.h"
#include "core/MojThread.h"
#include "core/MojObjectBuilder.h"
#include "core/MojSignal.h"

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
// Number of threads to start in Concurrent tests
extern size_t g_nthreads;
// Number of steps per each thread
extern size_t g_nsteps;
// Number of iterations for Sequential tests
extern size_t g_counter;

std::vector<MojString> MojSearchTestObjects;

enum type { ODD = 0, EVEN = 1 };

#define START_WORKER_THREADS_WITH_ARG(WORKER_FUNC, WORKER_ARG) do {std::vector<std::thread> threads;\
threads.resize(g_nthreads);\
for (size_t worker = 0; worker < threads.size(); ++worker) {\
    threads[worker] = std::thread(WORKER_FUNC, WORKER_ARG);\
}\
for(auto &thread : threads) {\
    thread.join();\
}\
} while (0)

struct SearchTest : public ::testing::Test
{
    virtual void SetUp()
    {
        unsigned int seed = static_cast<unsigned int> (time(NULL) );
        srand(seed);

        init();
        registerType();
        fillTestData();
    }

    virtual void TearDown()
    {
        MojExpectNoErr(m_db.close());
        MojExpectNoErr(MojRmDirRecursive(g_temp_folder));
    }

    typedef MojErr (MojDbServiceClient::* CallFuncPtr)(MojDbServiceClient::Signal::SlotRef, const MojDbQuery&, bool, bool);

    void registerType()
    {
        // register type
        const MojChar* const MojSearchKindStr =
                        _T("{\"id\":\"SearchTest:1\",")
                        _T("\"owner\":\"com.palm.mojfakeluna\",")
                        _T("\"indexes\":[")
                        _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\",\"collate\":\"primary\"}]},")
                        _T("]}");
        MojErr err = writeTestObj(MojSearchKindStr, true);
        MojAssertNoErr(err);
    }

    void fillTestData()
    {
        MojSearchTestObjects.resize(g_counter);

        for (MojSize i = 0; i < g_counter; ++i) {
            std::stringstream s;
            MojString tmp;
                        s << "{\"_id\":" << i + 1 << ",\"_kind\":\"SearchTest:1\",\"foo\":\"" << ((i + 1) % 2 ? "odd" : "even") << "_sample_" << rand() << "\"}";
            tmp.assign(s.str().c_str());
            MojSearchTestObjects[i] = tmp;
        }

        // put test objects
        for (MojSize i = 0; i < g_counter; ++i) {
            MojObject id;
            MojErr err = writeTestObj(MojSearchTestObjects[i], &id);
            MojAssertNoErr(err) << i;
        }
    }

    void initQuery(MojDbQuery &query, const MojChar *queryStr)
    {
        MojErr err;
        query.clear();
        err = query.from(_T("SearchTest:1"));
        MojAssertNoErr(err);

        MojString val;
        err = val.assign(queryStr);
        MojAssertNoErr(err);
        err = query.where(_T("foo"), MojDbQuery::OpPrefix, val, MojDbCollationPrimary);
        MojAssertNoErr(err);
    }

    void check(MojDbServiceClient *dbClient, CallFuncPtr func, const MojChar* queryStr, const MojChar* expectedIdsJson)
    {
        MojDbQuery query;
        ASSERT_NO_FATAL_FAILURE( initQuery(query, queryStr) );
        ASSERT_NO_FATAL_FAILURE( check(dbClient, func, query, expectedIdsJson) );
    }

    void check(MojDbServiceClient *dbClient, CallFuncPtr func, MojDbQuery &query, const MojChar *expectedIdsJson)
    {
        MojErr err;
        bool retb;
        MojObject results;
        MojObject particleResults;
        MojObject nextPage;

        MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
        err = (dbClient->*func)(handler->m_slot, query, false, true);
        MojAssertNoErr(err);

        err = handler->wait(m_dbClient->service());
        MojAssertNoErr(err);
        retb = handler->m_result.get(_T("results"), results);
        MojAssert(retb);

        do {
            MojDbQuery::Page p;

            retb = handler->m_result.get("next", nextPage);
            if (!retb) {
                break;
            }
            err = p.fromObject(nextPage);
            MojAssertNoErr(err);
            query.page(p);
            handler.reset(new MojDbClientTestResultHandler);
            err = (dbClient->*func)(handler->m_slot, query, false, true);
            MojAssertNoErr(err);
            retb = handler->m_result.get(_T("results"), particleResults);
            MojAssert(retb);

            for (MojObject::ConstArrayIterator iter = particleResults.arrayBegin();
                 iter != particleResults.arrayEnd();
                 iter++)
            {
                err = results.push(*iter);
                MojAssertNoErr(err);
            }
        } while (retb);

        MojString json;
        err = results.toJson(json);
        MojAssertNoErr(err);

        MojObject expected;
        err = expected.fromJson(expectedIdsJson);
        MojAssertNoErr(err);

        ASSERT_EQ(expected.size(), results.size())
                << "Amount of expected ids should match amount of ids in result";
        MojObject::ConstArrayIterator j = results.arrayBegin();
        size_t n = 0;
        for (MojObject::ConstArrayIterator i = expected.arrayBegin();
             j != results.arrayEnd() && i != expected.arrayEnd();
             ++i, ++j, ++n)
        {
            MojObject id;
            err = j->getRequired(MojDb::IdKey, id);
            MojAssertNoErr(err) << "MojDb::IdKey(\"" << MojDb::IdKey << "\") should present in item #" << n;
            MojAssert( *i == id );
        }
    }

    void init()
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

    MojErr unquote(const MojString& in, MojString &out)
    {
        MojErr err;

        out.clear();
        for(size_t i = 0; i < in.length(); i++){
            if (in[i] != '\"') {
                err = out.append(in[i]);
                MojErrCheck(err);
            }
        }
        return MojErrNone;
    }

    MojErr get_foo(const MojString& json, MojString &out)
    {
        MojObject o;
        MojObject r;
        MojString s;
        MojErr err;
        bool retb;

        err = o.fromJson(json);
        MojErrCheck(err);
        retb = o.get("foo", r);
        MojAssert(retb);
        err = r.toJson(out);
        MojErrCheck(err);
        return MojErrNone;
    }

    MojErr writeTestObj(const MojChar* json, MojObject* idOut = NULL)
    {
        return writeTestObj(json, false, idOut);
    }

    MojErr writeTestObj(const MojChar* json, bool kind, MojObject* idOut = NULL)
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
            MojString s;
            retb = handler->m_result.get(_T("results"), results);
            results.toJson(s);
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

    MojAutoPtr<MojService> m_service;
    MojAutoPtr<MojDbServiceClient> m_dbClient;
    MojAutoPtr<MojReactor> m_reactor;

    MojDb m_db;
    MojRefCountedPtr<MojDbServiceHandler> m_handler;
};

/* Find 10 different values from test dataset one by one
*/
TEST_F(SearchTest, testBasicFindSequential)
{
    MojString foo;
    MojString pattern;
    MojErr err;

    const size_t ntest_values = 10;
    MojString cmp_pattern;
    size_t x;

    CpuStat c("testBasicFindSequential");
    MemStat m("testBasicFindSequential");

    for (size_t i = ntest_values; i >= 1; i--) {
        x = (g_counter / ntest_values) * i;
        cmp_pattern.format("[%zu]", x);
        err = get_foo(MojSearchTestObjects[x - 1], foo);
        MojAssertNoErr(err);
        err = unquote(foo, pattern);
        MojAssertNoErr(err);
        check(m_dbClient.get(), &MojDbServiceClient::find, pattern, cmp_pattern);
    }
}
