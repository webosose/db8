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


#include "MojDbPerfCacheReadTest.h"
#include "db/MojDb.h"
#include "core/MojJson.h"
#include <vector>

#ifndef WITH_SEARCH_QUERY_CACHE
#include "db/MojDbSearchCursor.h"
#else
#include "db/MojDbSearchCacheCursor.h"
#endif

static const MojUInt32 numThread = 6;
static const MojUInt32 numWriteThread = 1;
static const MojUInt32 numRetryTest = 2;

static const MojUInt64 numInsertForGet = 100;
static const MojUInt64 numInsertForFind = 500;
static const MojUInt64 numRepetitionsForGet = 100;
static const MojUInt64 numRepetitionsForFind = 20;

extern MojUInt64 allTestsTime;

MojDbPerfCacheReadTest::MojDbPerfCacheReadTest()
: MojDbPerfTest(_T("MojDbPerfCacheRead"))
{
    m_readWriteTest = false;
    m_readThreadDone = false;
}

MojErr MojDbPerfCacheReadTest::run()
{
    MojErr err;

    totalTestTime = 0;

    if (lazySync())
    {
        err=m_db.configure(lazySyncConfig());
        MojTestErrCheck(err);
    }

    err = m_db.open(MojDbTestDir);
    MojTestErrCheck(err);

    err = testConcurrentRead();
    MojTestErrCheck(err);

    err = testConcurrentReadWrite();
    MojTestErrCheck(err);

    allTestsTime += totalTestTime;

    err = MojPrintF("\n\n TOTAL TEST TIME: %llu nanoseconds. | %10.3f seconds.\n\n", totalTestTime, double(totalTestTime) / 1000000000.0);
    MojTestErrCheck(err);
    err = MojPrintF("\n-------\n");
    MojTestErrCheck(err);

    err = m_db.close();
    MojTestErrCheck(err);

    return MojErrNone;
}

struct SearchThreadInfo {
    MojDbPerfCacheReadTest* thiz;
    int argument;
};

MojErr MojDbPerfCacheReadTest::searchThread(void* arg)
{
    SearchThreadInfo* info = reinterpret_cast<SearchThreadInfo*>(arg);
    MojDbPerfCacheReadTest* thiz = info->thiz;
    unsigned threadNumber = info->argument;

    MojErr err;

    err = thiz->testFindPaged(threadNumber);
    MojTestErrCheck(err);

    if (info)
        delete info;

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::testConcurrentRead()
{
    MojErr err;
    std::vector<MojThreadT> threadVector;

    // register all the kinds
    MojUInt64 time = 0;
    err = putKinds(m_db, time);
    MojTestErrCheck(err);

    for (unsigned i=0; i<numThread; ++i)
    {
        MojThreadT threadId = MojInvalidThread;
        SearchThreadInfo* info = new SearchThreadInfo;

        info->thiz = this;
        info->argument = i;

        err = MojThreadCreate(threadId, searchThread, info);
        MojTestErrCheck(err);

        threadVector.push_back(threadId);
     }

    for (unsigned i=0; i<numThread; ++i)
    {
        MojErr threadErr = MojErrNone;
        err = MojThreadJoin(threadVector[i], threadErr);
        MojTestErrCheck(err);
        MojTestAssert(threadErr == MojErrNone);
    }

    delKinds(m_db);

    return err;
}

MojErr MojDbPerfCacheReadTest::readThread(void* arg)
{
    SearchThreadInfo* info = reinterpret_cast<SearchThreadInfo*>(arg);
    MojDbPerfCacheReadTest* thiz = info->thiz;
    unsigned threadNumber = info->argument;

    MojErr err;

    for (unsigned i=0; i<10000; ++i)
    {
        MojDbQuery q;
        err = q.from(MojPerfSmKindId);
        MojTestErrCheck(err);
        err = thiz->findObjsPaged2(threadNumber, MojPerfSmKindId, q);
        MojTestAssert(err == MojErrDbKindNotRegistered || err == MojErrNone );
        if ( err != MojErrDbKindNotRegistered && err != MojErrNone )
        {
            MojTestErrCheck(err);
        }
    }

    if (info)
        delete info;

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::writeThread(void* arg)
{
    SearchThreadInfo* info = reinterpret_cast<SearchThreadInfo*>(arg);
    MojDbPerfCacheReadTest* thiz = info->thiz;
    unsigned threadNumber = info->argument;

    MojErr err;

    while (thiz->m_readThreadDone)
    {
        MojUInt64 time;
        thiz->putKinds(thiz->m_db, time);
        MojSleep( 10 * 1000 );

        // put objects using createFn and get them individually and in groups
        MojObject ids;
        err = thiz->putObjs(MojPerfSmKindId, numInsertForFind, &MojDbPerfTest::createSmallObj, ids);
        MojTestErrCheck(err);
        MojSleep( 10 * 1000 );

        thiz->delKinds(thiz->m_db);
        MojSleep( 10 * 1000 );
    }

    if (info)
        delete info;

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::testConcurrentReadWrite()
{
    MojErr err;
    std::vector<MojThreadT> readThreadVector;
    std::vector<MojThreadT> writeThreadVector;

    m_readWriteTest = true;

    for (unsigned i=0; i<numWriteThread; ++i)
    {
        MojThreadT threadId = MojInvalidThread;
        SearchThreadInfo* info = new SearchThreadInfo;

        info->thiz = this;
        info->argument = i;

        err = MojThreadCreate(threadId, writeThread, info);
        MojTestErrCheck(err);

        writeThreadVector.push_back(threadId);
     }

    for (unsigned i=0; i<numThread; ++i)
    {
        MojThreadT threadId = MojInvalidThread;
        SearchThreadInfo* info = new SearchThreadInfo;

        info->thiz = this;
        info->argument = i;

        err = MojThreadCreate(threadId, readThread, info);
        MojTestErrCheck(err);

        readThreadVector.push_back(threadId);
     }

    for (unsigned i=0; i<numThread; ++i)
    {
        MojErr threadErr = MojErrNone;
        err = MojThreadJoin(readThreadVector[i], threadErr);
        //MojTestErrCheck(err);
        MojTestAssert(threadErr == MojErrNone || threadErr == MojErrDbKindNotRegistered );
    }

    m_readThreadDone = true;

    for (unsigned i=0; i<numWriteThread; ++i)
    {
        MojErr threadErr = MojErrNone;
        err = MojThreadJoin(writeThreadVector[i], threadErr);
        //MojTestErrCheck(err);
        MojTestAssert(threadErr == MojErrNone || threadErr == MojErrDbKindNotRegistered );
    }

    return err;
}

MojErr MojDbPerfCacheReadTest::testFindPaged(unsigned threadNumber)
{
    MojErr err = MojPrintF("\n--------------\n");
    MojTestErrCheck(err);
    err = MojPrintF("  PAGED FIND");
    MojTestErrCheck(err);
    err = MojPrintF("\n--------------\n");
    MojTestErrCheck(err);

    for (unsigned i=0; i<numRetryTest; ++i)
    {
        MojDbQuery q;
        err = q.from(MojPerfSmKindId);
        MojTestErrCheck(err);
        err = findObjsPaged(threadNumber, MojPerfSmKindId, &MojDbPerfTest::createSmallObj, q);
        MojTestErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::findObjsPaged(unsigned threadNumber, const MojChar* kindId, MojErr (MojDbPerfTest::*createFn)(MojObject&, MojUInt64), MojDbQuery& query)
{
    MojErr err;

    if (createFn)
    {
        // put objects using createFn and get them individually and in groups
        MojObject ids;
        err = putObjs(kindId, numInsertForFind, createFn, ids);
        MojTestErrCheck(err);
    }

    MojUInt32 count;
    MojUInt64 countTime = 0;
    MojDbQuery::Page nextPage;

    MojUInt64 find30Time = 0;
    query.limit(30);
    err = timeFind(query, find30Time, true, nextPage, false, count, countTime);
    MojTestErrCheck(err);

    MojUInt64 findTime = find30Time;
    err = MojPrintF("\n -------------------- \n");
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time to find %d objects out of %llu of kind %s %llu times (with writer): %llu nanosecs\n", threadNumber, 30, numInsertForFind, kindId, numRepetitionsForFind, findTime);
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time per object: %llu nanosecs", threadNumber, (findTime) / (30 * numRepetitionsForFind));
    MojTestErrCheck(err);
    err = MojPrintF("\n\n");
    MojTestErrCheck(err);

    MojUInt64 find2nd30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find2nd30Time, true, nextPage, false, count, countTime);
    MojTestErrCheck(err);
    findTime = find2nd30Time;
    err = MojPrintF("\n -------------------- \n");
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time to find 2nd %d objects out of %llu of kind %s %llu times (with writer): %llu nanosecs\n", threadNumber, 30, numInsertForFind, kindId, numRepetitionsForFind, findTime);
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time per object: %llu nanosecs", threadNumber, (findTime) / (30 * numRepetitionsForFind));
    MojTestErrCheck(err);
    err = MojPrintF("\n\n");
    MojTestErrCheck(err);

    MojUInt64 find3rd30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find3rd30Time, true, nextPage, false, count, countTime);
    MojTestErrCheck(err);
    findTime = find3rd30Time;
    err = MojPrintF("\n -------------------- \n");
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time to find 3rd %d objects out of %llu of kind %s %llu times (with writer): %llu nanosecs\n", threadNumber, 30, numInsertForFind, kindId, numRepetitionsForFind, findTime);
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time per object: %llu nanosecs", threadNumber, (findTime) / (30 * numRepetitionsForFind));
    MojTestErrCheck(err);
    err = MojPrintF("\n\n");
    MojTestErrCheck(err);

    MojUInt64 find4th30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find4th30Time, true, nextPage, false, count, countTime);
    MojTestErrCheck(err);
    findTime = find4th30Time;
    err = MojPrintF("\n -------------------- \n");
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time to find 4th %d objects out of %llu of kind %s %llu times (with writer): %llu nanosecs\n", threadNumber, 30, numInsertForFind, kindId, numRepetitionsForFind, findTime);
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time per object: %llu nanosecs", threadNumber, (findTime) / (30 * numRepetitionsForFind));
    MojTestErrCheck(err);
    err = MojPrintF("\n\n");
    MojTestErrCheck(err);

    MojUInt64 find5th30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find5th30Time, true, nextPage, false, count, countTime);
    MojTestErrCheck(err);
    findTime = find5th30Time;
    err = MojPrintF("\n -------------------- \n");
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time to find 5th %d objects out of %llu of kind %s %llu times (with writer): %llu nanosecs\n", threadNumber, 30, numInsertForFind, kindId, numRepetitionsForFind, findTime);
    MojTestErrCheck(err);
    err = MojPrintF("   (%d) time per object: %llu nanosecs", threadNumber, (findTime) / (30 * numRepetitionsForFind));
    MojTestErrCheck(err);
    err = MojPrintF("\n\n");
    MojTestErrCheck(err);

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::findObjsPaged2(unsigned threadNumber, const MojChar* kindId, MojDbQuery& query)
{
    MojErr err;

    MojUInt32 count;
    MojUInt64 countTime = 0;
    MojDbQuery::Page nextPage;

    MojUInt64 find30Time = 0;
    query.limit(30);
    err = timeFind(query, find30Time, true, nextPage, false, count, countTime);
    MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
    if ( err == MojErrDbKindNotRegistered ) return MojErrNone;

    MojUInt64 find2nd30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find2nd30Time, true, nextPage, false, count, countTime);
    MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
    if ( err == MojErrDbKindNotRegistered ) return MojErrNone;

    MojUInt64 find3rd30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find3rd30Time, true, nextPage, false, count, countTime);
    MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
    if ( err == MojErrDbKindNotRegistered ) return MojErrNone;

    MojUInt64 find4th30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find4th30Time, true, nextPage, false, count, countTime);
    MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
    if ( err == MojErrDbKindNotRegistered ) return MojErrNone;

    MojUInt64 find5th30Time = 0;
    query.page(nextPage);
    err = timeFind(query, find5th30Time, true, nextPage, false, count, countTime);
    MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
    if ( err == MojErrDbKindNotRegistered ) return MojErrNone;

    return MojErrNone;
}


MojErr MojDbPerfCacheReadTest::timeFind(MojDbQuery& query, MojUInt64& findTime, bool useWriter, MojDbQuery::Page& nextPage, bool doCount, MojUInt32& count, MojUInt64& countTime)
{
    timespec startTime;
    startTime.tv_nsec = 0;
    startTime.tv_sec = 0;
    timespec endTime;
    endTime.tv_nsec = 0;
    endTime.tv_sec = 0;

    MojString localeStr;
    localeStr.assign("en_US");

    for (MojUInt64 i = 0; i < numRepetitionsForFind; i++) {
        clock_gettime(CLOCK_MONOTONIC, &startTime);
        MojDbSearchCursor cursor(localeStr);
        MojErr err = m_db.find(query, cursor);
        if (m_readWriteTest)
        {
            MojTestAssert(err == MojErrNone || err == MojErrDbKindNotRegistered);
            if (err == MojErrDbKindNotRegistered)
            {
                return MojErrNone;
            }
        }
        else
        {
            MojTestErrCheck(err);
        }

        if (useWriter) {
            MojJsonWriter writer;
            err = cursor.visit(writer);
            MojTestErrCheck(err);
            err = cursor.nextPage(nextPage);
            MojTestErrCheck(err);
        } else {
            MojObjectEater eater;
            err = cursor.visit(eater);
            MojTestErrCheck(err);
            err = cursor.nextPage(nextPage);
            MojTestErrCheck(err);
        }

        if (!doCount) {
            err = cursor.close();
            MojTestErrCheck(err);
        }
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        findTime += timeDiff(startTime, endTime);
        totalTestTime += timeDiff(startTime, endTime);

        if (doCount) {
            clock_gettime(CLOCK_MONOTONIC, &startTime);
            err = cursor.count(count);
            MojTestErrCheck(err);
            err = cursor.close();
            MojTestErrCheck(err);
            clock_gettime(CLOCK_MONOTONIC, &endTime);
            countTime += timeDiff(startTime, endTime);
            totalTestTime += timeDiff(startTime, endTime);
        }
    }

    return MojErrNone;
}

MojErr MojDbPerfCacheReadTest::putObjs(const MojChar* kindId, MojUInt64 numInsert,
        MojErr (MojDbPerfTest::*createFn) (MojObject&, MojUInt64), MojObject& ids)
{
    for (MojUInt64 i = 0; i < numInsert; i++) {
        MojObject obj;
        MojErr err = obj.putString(MojDb::KindKey, kindId);
        MojTestErrCheck(err);
        err = (*this.*createFn)(obj, i);
        MojTestErrCheck(err);

        {
            MojThreadGuard guard(m_lock);
            err = m_db.put(obj);
        }

        MojTestAssert( err == MojErrNone || err == MojErrDbKindNotRegistered );
        if ( err == MojErrDbKindNotRegistered )
            return MojErrNone;
        MojTestErrCheck(err);

        MojObject id;
        err = obj.getRequired(MojDb::IdKey, id);
        MojTestErrCheck(err);
        err = ids.push(id);
        MojTestErrCheck(err);
    }

    return MojErrNone;
}

void MojDbPerfCacheReadTest::cleanup()
{
    (void) MojRmDirRecursive(MojDbTestDir);
}
