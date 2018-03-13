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


#ifndef MOJDBPERFCACHEREADTEST_H_
#define MOJDBPERFCACHEREADTEST_H_

#include "MojDbPerfTest.h"
#include "db/MojDb.h"
#include "db/MojDbQuery.h"

class MojDbPerfCacheReadTest : public MojDbPerfTest {
public:
    MojDbPerfCacheReadTest();

    virtual MojErr run();
    virtual void cleanup();

private:
    static MojErr searchThread(void* arg);
    static MojErr readThread(void* arg);
    static MojErr writeThread(void* arg);

    MojErr testGet(unsigned threadNumber);
    MojErr testConcurrentRead();
    MojErr testConcurrentReadWrite();

    MojErr testFindPaged(unsigned threadNumber);
    MojErr findObjsPaged(unsigned threadNumber, const MojChar* kindId, MojErr (MojDbPerfTest::*createFn)(MojObject&, MojUInt64), MojDbQuery& query);
    MojErr findObjsPaged2(unsigned threadNumber, const MojChar* kindId, MojDbQuery& query);
    MojErr timeFind(MojDbQuery& query, MojUInt64& findTime, bool useWriter, MojDbQuery::Page& nextPage, bool doCount, MojUInt32& count, MojUInt64& countTime);


    MojErr putObjs(const MojChar* kindId, MojUInt64 numInsert,
            MojErr (MojDbPerfTest::*createFn) (MojObject&, MojUInt64), MojObject& ids);

    MojDb m_db;
    bool m_readWriteTest;
    bool m_readThreadDone;

    mutable MojThreadMutex m_lock;

    MojUInt64 totalTestTime;
};


#endif /* MOJDBPERFCACHEREADTEST_H_ */
