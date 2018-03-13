// Copyright (c) 2009-2018 LG Electronics, Inc.
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


#include "MojDbDumpLoadTest.h"
#include "db/MojDb.h"
#include "core/MojUtil.h"

namespace {
	MojString sandboxFileName(const MojChar *name)
	{
		MojString path;
		MojErr err = path.format("%s/%s", MojDbTestDir, name);
		assert(err == MojErrNone);
		return path;
	}
}

static const MojChar* const MojLoadTestFileName = _T("loadtest.json");
static const MojChar* const MojDumpTestFileName1 = _T("dumptest_with_config.json");
static const MojChar* const MojDumpTestFileName2 = _T("dumptest_without_config.json");
static const MojChar* const MojTestStr =
	_T("{\"_id\":\"_kinds/LoadTest:1\",\"_kind\":\"Kind:1\",\"id\":\"LoadTest:1\",\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},{\"name\":\"barfoo\",\"props\":[{\"name\":\"bar\"},{\"name\":\"foo\"}]}]}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}")
	_T("{\"_kind\":\"LoadTest:1\",\"foo\":\"hello\",\"bar\":\"world\"}");

MojDbDumpLoadTest::MojDbDumpLoadTest()
: MojTestCase(_T("MojDbDumpLoad"))
{
}

MojErr MojDbDumpLoadTest::run()
{
	MojDb db;

    // Dump-Load Test Phase 1 - add root kind(Object:1)
    db.enableRootKind(true);

	// open
	MojErr err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	// load
	err = MojFileFromString(sandboxFileName(MojLoadTestFileName), MojTestStr);
	MojTestErrCheck(err);
	MojUInt32 count = 0;
	err = db.load(sandboxFileName(MojLoadTestFileName), count);
	MojTestErrCheck(err);
	MojTestAssert(count == 11);
	err = checkCount(db);
	MojTestErrCheck(err);

    count = 0;
    // dump test with config (use 'Object:1' kind)
    err = testDump(db, db.isRootKindEnabled());
    MojTestErrCheck(err);

    // del and purge
    MojString id;
    err = id.assign(_T("LoadTest:1"));
    MojTestErrCheck(err);
    bool found = false;
    err = db.delKind(id, found);
    MojTestErrCheck(err);
    MojTestAssert(found);
    err = db.purge(count, 0);
    MojTestErrCheck(err);

    // load again
    err = db.load(sandboxFileName(MojDumpTestFileName1), count);
    MojTestErrCheck(err);
    MojTestAssert(count == 12);
    err = checkCount(db);
    MojTestErrCheck(err);

    // analyze
    MojObject analysis;
    err = db.stats(analysis);
    MojErrCheck(err);

    // close db <-- end of phase 1
    // delete test kind before close
    err = db.delKind(id, found);
    MojTestErrCheck(err);
    MojTestAssert(found);
    err = db.close();
    MojTestErrCheck(err);
    cleanup();

    // Dump-Load Test Phase 2 - DO NOT add root kind(Object:1)
    db.enableRootKind(false);
    db.enablePurge(false);      // if enableRootKind is false, enablePurge should be false

    err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    // load
    err = MojFileFromString(sandboxFileName(MojLoadTestFileName), MojTestStr);
    MojTestErrCheck(err);
	count = 0;
	err = db.load(sandboxFileName(MojLoadTestFileName), count);
	MojTestErrCheck(err);
	MojTestAssert(count == 11);
	err = checkCount(db);
	MojTestErrCheck(err);

    count = 0;
    // dump test with config (DO NOT use 'Object:1' kind)
    err = testDump(db, db.isRootKindEnabled());
    MojTestErrCheck(err);

    // del and purge
    found = false;
    err = db.delKind(id, found);
    MojTestErrCheck(err);
    MojTestAssert(found);
    err = db.purge(count, 0);
    MojTestErrCheck(err);

    // load again
    err = db.load(sandboxFileName(MojDumpTestFileName2), count);
    MojTestErrCheck(err);
    MojTestAssert(count == 11);     // nothing to purge
    err = checkCount(db);
    MojTestErrCheck(err);

    // analyze
    err = db.stats(analysis);
    MojErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbDumpLoadTest::checkCount(MojDb& db)
{
	MojDbQuery query;
	MojErr err = query.from(_T("LoadTest:1"));
	MojTestErrCheck(err);
	MojDbCursor cursor;
	err = db.find(query, cursor);
	MojTestErrCheck(err);
	MojUInt32 count = 0;
	err = cursor.count(count);
	MojTestErrCheck(err);
	MojTestAssert(count == 10);

	return MojErrNone;
}

MojErr MojDbDumpLoadTest::testDump(MojDb& db, bool a_enable) {

    // Check Object:1 kind is added or not
    MojDbQuery query;
    MojDbCursor cursor;
    MojErr err = query.from(_T("Object:1"));
    MojTestErrCheck(err);

    // make sure that result of dump should be same whether 'enableRootKind' flag is true or not
    MojUInt32 count = 0;
    if (a_enable) {
        err = db.find(query, cursor);
        MojTestErrCheck(err);               // Check 'Object:1' is exist
        MojUInt32 countOut = 0;
        for(;;) {
            MojObject obj;
            bool found;
            MojString str;
            err = cursor.get(obj, found);
            MojTestErrCheck(err);
            if (!found)
                break;
            countOut++;
        }
        // check Object:1 kind has all object properly
        MojTestAssert(countOut == 11);
        err = db.dump(sandboxFileName(MojDumpTestFileName1), count);
        MojTestErrCheck(err);
    }
    else {
        err = db.find(query, cursor);
        MojTestAssert(err == MojErrDbKindNotRegistered);    // expected error (enableRootKind flag is false)
        err = db.dump(sandboxFileName(MojDumpTestFileName2), count);
        MojTestErrCheck(err);
    }
    MojTestAssert(count == 11);

    return MojErrNone;
}

void MojDbDumpLoadTest::cleanup()
{
	(void) MojUnlink(sandboxFileName(MojLoadTestFileName));
	(void) MojUnlink(sandboxFileName(MojDumpTestFileName1));
    (void) MojUnlink(sandboxFileName(MojDumpTestFileName2));
	(void) MojRmDirRecursive(MojDbTestDir);
}
