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

/**
* ***************************************************************************************************
* Filename              : MojDbDatabaseIdTest.cpp
* Description           : Source file for MojDbDatabaseIdTest test.
****************************************************************************************************
**/

#include "MojDbDatabaseIdTest.h"
#include "db/MojDb.h"

#include <iostream>

MojDbDatabaseIdTest::MojDbDatabaseIdTest()
: MojTestCase(_T("MojDbDatabaseId"))
{
}

MojErr MojDbDatabaseIdTest::run ()
{
    MojDb db;
    MojErr err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    MojDbReq req;
    err = req.begin(&db, true);
    MojTestErrCheck(err);

    MojString id0;
    err = db.databaseId(id0, req);
    MojTestErrCheck(err);
    MojTestAssert(!id0.empty());

    MojString id1;
    err = db.databaseId(id1, req);
    MojTestErrCheck(err);
    MojTestAssert(!id1.empty());
    MojTestAssert(id0.compare(id1) == 0);

    err = req.end(true);
    MojTestErrCheck(err);


    err = db.close();
    MojTestErrCheck(err);

    MojDb otherDb;

    err = otherDb.open(MojDbTestDir);
    MojTestErrCheck(err);

    MojString id2;
    err = otherDb.databaseId(id2);

    MojTestAssert(!id2.empty());
    MojTestAssert(id0.compare(id2) == 0);

    return MojErrNone;
}
