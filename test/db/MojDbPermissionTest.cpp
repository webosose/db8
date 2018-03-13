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


#include "MojDbPermissionTest.h"
#include "db/MojDb.h"

static const MojChar* const MojTestConf =
		_T("{\"db\":{\"permissionsEnabled\":true}}");
static const MojChar* const MojTestKind1 =
		_T("{\"id\":\"PermissionTest:1\",")
		_T("\"owner\":\"com.foo\"}");
static const MojChar* const MojTestKind2 =
		_T("{\"id\":\"PermissionTest2:1\",")
		_T("\"owner\":\"com.foo\"}");
static const MojChar* const MojTestKind3 =
        _T("{\"id\":\"PermissionTest3:1\",")
        _T("\"owner\":\"com.admin\"}");
static const MojChar* const MojTestKind4 =
	_T("{\"id\":\"PermissionTest4:1\",")
	_T("\"owner\":\"com.bar\",")
	_T("\"extends\":[\"PermissionTest3:1\"]}");
static const MojChar* const MojTestPermission1 =
		_T("{\"type\":\"db.kind\",\"object\":\"PermissionTest:1\",\"caller\":\"com.granted\",\"operations\":{\"create\":\"allow\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"allow\"}}");
static const MojChar* const MojTestPermissionWildcard =
		_T("{\"type\":\"db.kind\",\"object\":\"PermissionTest:1\",\"caller\":\"com.foo.*\",\"operations\":{\"create\":\"allow\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"allow\"}}");
static const MojChar* const MojTestPermissionWildcard2 =
		_T("{\"type\":\"db.kind\",\"object\":\"PermissionTest2:1\",\"caller\":\"*\",\"operations\":{\"create\":\"allow\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"allow\"}}");
static const MojChar* const MojTestAdminPermission1 =
		_T("{\"type\":\"db.role\",\"object\":\"admin\",\"caller\":\"com.admin\",\"operations\":{\"*\":\"allow\"}}");
static const MojChar* const MojTestPermissionDeny1 =
        _T("{\"type\":\"db.kind\",\"object\":\"PermissionTest3:1\",\"caller\":\"com.foo\",\"operations\":{\"create\":\"deny\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"allow\"}}");
static const MojChar* const MojTestPermissionDeny2 =
        _T("{\"type\":\"db.kind\",\"object\":\"PermissionTest3:1\",\"caller\":\"com.foo\",\"operations\":{\"create\":\"allow\",\"read\":\"deny\",\"update\":\"allow\",\"delete\":\"allow\"}}");
static const MojChar* const MojTestPermissionDeny3 =
        _T("{\"type\":\"db.kind\",\"object\":\"PermissionTest3:1\",\"caller\":\"com.foo\",\"operations\":{\"create\":\"allow\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"deny\"}}");
static const MojChar* const MojTestPermissionDeny4 =
        _T("{\"type\":\"db.kind\",\"object\":\"PermissionTest3:1\",\"caller\":\"com.bar\",\"operations\":{\"create\":\"allow\",\"read\":\"allow\",\"update\":\"allow\",\"delete\":\"allow\", \"extend\": \"deny\"}}");
static const MojChar* const MojTestInvalidPermission1 =
		_T("{\"type\":\"db.role\",\"object\":\"admin\",\"caller\":\"\",\"operations\":{\"*\":\"allow\"}}");
static const MojChar* const MojTestInvalidPermission2 =
		_T("{\"type\":\"db.role\",\"object\":\"admin\",\"caller\":\"com.*.admin\",\"operations\":{\"*\":\"allow\"}}");
static const MojChar* const MojTestInvalidPermission3 =
		_T("{\"type\":\"db.role\",\"object\":\"admin\",\"caller\":\"*com.admin\",\"operations\":{\"*\":\"allow\"}}");
static const MojChar* const MojTestInvalidPermission4 =
		_T("{\"type\":\"db.role\",\"object\":\"admin\",\"caller\":\"*com.admin\",\"operations\":{\"*\":\"allow\"}}");


MojDbPermissionTest::MojDbPermissionTest()
: MojTestCase(_T("MojDbPermission"))
{
}

MojErr MojDbPermissionTest::run()
{
	MojDb db;
	MojObject conf;
	MojErr err = conf.fromJson(MojTestConf);
	MojTestErrCheck(err);
	err = db.configure(conf);
	MojErrCheck(err);
	err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	err = testInvalidPermissions(db);
	MojTestErrCheck(err);
	err = testAdminPermissions(db);
	MojTestErrCheck(err);
	err = testKindPermissions(db);
	MojTestErrCheck(err);
	err = testObjectPermissions(db);
    MojTestErrCheck(err);
    err = testDenyPermissions(db);
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbPermissionTest::testInvalidPermissions(MojDb& db)
{
	MojErr err = checkInvalid(MojTestInvalidPermission1, db);
	MojTestErrCheck(err);
	err = checkInvalid(MojTestInvalidPermission2, db);
	MojTestErrCheck(err);
	err = checkInvalid(MojTestInvalidPermission3, db);
	MojTestErrCheck(err);
	err = checkInvalid(MojTestInvalidPermission4, db);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbPermissionTest::testAdminPermissions(MojDb& db)
{
	MojObject permission;
	MojErr err = permission.fromJson(MojTestAdminPermission1);
	MojTestErrCheck(err);

	err = db.putPermissions(&permission, &permission + 1);
	MojTestErrCheck(err);

	MojDbReq reqAdmin(false);
	err = reqAdmin.domain(_T("com.admin"));
	MojTestErrCheck(err);

	// put kind with non-matching owner with admin role
	MojObject kind;
	err = kind.fromJson(MojTestKind1);
	MojTestErrCheck(err);
	err = db.putKind(kind, MojDbFlagNone, reqAdmin);
	MojTestErrCheck(err);
	err = kind.fromJson(MojTestKind2);
	MojTestErrCheck(err);
	err = db.putKind(kind, MojDbFlagNone, reqAdmin);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbPermissionTest::testKindPermissions(MojDb& db)
{
	MojObject kind;
	MojErr err = kind.fromJson(MojTestKind1);
	MojTestErrCheck(err);

	// new kind, mismatched owner and caller
	MojDbReq req(false);
	err = req.domain(_T("com.bar"));
	MojTestErrCheck(err);
	err = db.putKind(kind, MojDbFlagNone, req);
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	// new kind, matched owner and caller
	err = req.domain(_T("com.foo"));
	MojTestErrCheck(err);
	err = db.putKind(kind, MojDbFlagNone, req);
	MojTestErrCheck(err);
	// existing kind, matched owner and caller
	err = db.putKind(kind, MojDbFlagNone, req);
	MojTestErrCheck(err);
	// existing kind, mismatched owner and caller
	err = req.domain(_T("com.bar"));
	MojTestErrCheck(err);
	err = db.putKind(kind, MojDbFlagNone, req);
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	// delKind, mismatched owner and caller
	MojString id;
	err = id.assign(_T("PermissionTest:1"));
	MojTestErrCheck(err);
	bool found = false;
	err = db.delKind(id, found, MojDbFlagNone, req);
	MojTestErrExpected(err, MojErrDbPermissionDenied);

	return MojErrNone;
}

MojErr MojDbPermissionTest::testObjectPermissions(MojDb& db)
{
	MojErr err = putPermissions(db);
	MojTestErrCheck(err);
	err = checkPermissions(db);
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);
	err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	err = checkPermissions(db);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbPermissionTest::putPermissions(MojDb& db)
{
	MojDbReq reqOwner(false);
	MojErr err = reqOwner.domain(_T("com.foo"));
	MojTestErrCheck(err);
	MojDbReq reqGranted(false);
	err = reqGranted.domain("com.granted 3287");
	MojTestErrCheck(err);
	MojDbReq reqBad(false);
	err = reqBad.domain("com.bad");
	MojTestErrCheck(err);

	MojObject permission;
	err = permission.fromJson(MojTestPermission1);
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission + 1, reqBad);
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	err = reqBad.abort();
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission + 1, reqOwner);
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission + 1, reqGranted);
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	err = reqGranted.abort();
	MojTestErrCheck(err);

	err = permission.fromJson(MojTestPermissionWildcard);
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission + 1, reqOwner);
	MojTestErrCheck(err);
	err = permission.fromJson(MojTestPermissionWildcard2);
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission + 1, reqOwner);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbPermissionTest::checkPermissions(MojDb& db)
{
	MojDbReq reqOwner(false);
	MojErr err = reqOwner.domain(_T("com.foo"));
	MojTestErrCheck(err);
	MojDbReq reqFooBar(false);
	err = reqFooBar.domain(_T("com.foo.bar"));
	MojTestErrCheck(err);
	MojDbReq reqFooBarBad(false);
	err = reqFooBarBad.domain(_T("com.foobar"));
	MojTestErrCheck(err);
	MojDbReq reqGranted(false);
	err = reqGranted.domain("com.granted");
	MojTestErrCheck(err);
	MojDbReq reqEmpty(false);
	MojDbReq reqBad(false);
	err = reqBad.domain("com.bad");
	MojTestErrCheck(err);

	MojObject objNoId;
	err = objNoId.putString(MojDb::KindKey, _T("PermissionTest:1"));
	MojTestErrCheck(err);
	MojObject obj2NoId;
	err = obj2NoId.putString(MojDb::KindKey, _T("PermissionTest2:1"));
	MojTestErrCheck(err);

	// put new obj
	MojObject obj1 = objNoId;
	err = db.put(obj1, MojDbFlagNone, reqOwner);
	MojTestErrCheck(err);
	MojObject obj2 = objNoId;
	err = db.put(obj2, MojDbFlagNone, reqGranted);
	MojTestErrCheck(err);
	MojObject obj3 = objNoId;
	err = db.put(obj3, MojDbFlagNone, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	obj3 = objNoId;
	err = db.put(obj3, MojDbFlagNone, reqFooBarBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqFooBarBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	obj3 = objNoId;
	err = db.put(obj3, MojDbFlagNone, reqFooBar);
	MojTestErrCheck(err);
	err = db.put(obj2NoId, MojDbFlagNone, reqEmpty);
	MojTestErrCheck(err);
	// put existing obj
	err = obj1.put(_T("foo"), 1);
	MojTestErrCheck(err);
	err = db.put(obj1, MojDbFlagNone, reqOwner);
	MojTestErrCheck(err);
	err = obj2.put(_T("foo"), 2);
	MojTestErrCheck(err);
	err = db.put(obj2, MojDbFlagNone, reqGranted);
	MojTestErrCheck(err);
	err = obj2.put(_T("foo"), 3);
	MojTestErrCheck(err);
	err = db.put(obj2, MojDbFlagNone, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);

	// find
	MojDbQuery query;
	err = query.from(_T("PermissionTest:1"));
	MojTestErrCheck(err);
	MojDbCursor cursor;
	err = db.find(query, cursor, reqOwner);
	MojTestErrCheck(err);
	err = cursor.close();
	MojTestErrCheck(err);
	err = db.find(query, cursor, reqGranted);
	MojTestErrCheck(err);
	err = cursor.close();
	MojTestErrCheck(err);
	err = db.find(query, cursor, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	err = cursor.close();
	MojTestErrCheck(err);

	// get
	MojObject id1;
	MojTestAssert(obj1.get(MojDb::IdKey, id1));
	bool found = false;
	MojObject gotten;
	err = db.get(id1, gotten, found, reqOwner);
	MojTestErrCheck(err);
	err = db.get(id1, gotten, found, reqGranted);
	MojTestErrCheck(err);
	err = db.get(id1, gotten, found, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);

	// del by id
	err = db.del(id1, found, MojDbFlagNone, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);
	MojTestAssert(!found);
	err = db.del(id1, found, MojDbFlagNone, reqOwner);
	MojTestErrCheck(err);
	MojTestAssert(found);
	MojObject id2;
	MojTestAssert(obj2.get(MojDb::IdKey, id2));
	err = db.del(id2, found, MojDbFlagNone, reqGranted);
	MojTestErrCheck(err);
	MojTestAssert(found);

	// del query
	MojUInt32 count = 0;
	err = db.del(query, count, MojDbFlagNone, reqOwner);
	MojTestErrCheck(err);
	err = db.del(query, count, MojDbFlagNone, reqGranted);
	MojTestErrCheck(err);
	err = db.del(query, count, MojDbFlagNone, reqBad);
#ifdef LMDB_ENGINE_SUPPORT
	reqBad.end();
#endif
	MojTestErrExpected(err, MojErrDbPermissionDenied);

	return MojErrNone;
}

MojErr MojDbPermissionTest::checkInvalid(const MojChar* json, MojDb& db)
{
	MojObject permission;
	MojErr err = permission.fromJson(json);
	MojTestErrCheck(err);
	err = db.putPermissions(&permission, &permission+1);
	MojTestErrExpected(err, MojErrDbInvalidCaller);

	return MojErrNone;
}

MojErr MojDbPermissionTest::testDenyPermissions(MojDb& db)
{
    MojObject kind;
    MojErr err = kind.fromJson(MojTestKind3);
    MojTestErrCheck(err);
    MojDbReq adminReq(false);
    err = adminReq.domain(_T("com.admin"));
    MojTestErrCheck(err);
    MojDbReq req(false);
    err = req.domain(_T("com.foo"));
    MojTestErrCheck(err);
    err = db.putKind(kind, MojDbFlagNone, adminReq);
    MojTestErrCheck(err);
    // create
    MojObject permission;
    err = permission.fromJson(MojTestPermissionDeny1);
    MojTestErrCheck(err);
    err = db.putPermissions(&permission, &permission + 1, adminReq);
    MojTestErrCheck(err);
    MojObject obj;
    err = obj.putString(MojDb::KindKey, _T("PermissionTest3:1"));
    MojTestErrCheck(err);
    err = db.put(obj, MojDbFlagNone, req);
    MojTestErrExpected(err, MojErrDbPermissionDenied);
    req.abort();

    // read
    err = permission.fromJson(MojTestPermissionDeny2);
    MojTestErrCheck(err);
    err = db.putPermissions(&permission, &permission + 1, adminReq);
    MojTestErrCheck(err);
    err = db.put(obj, MojDbFlagNone, req);
    MojTestErrCheck(err);
    MojDbQuery query;
    err = query.from(_T("PermissionTest3:1"));
    MojTestErrCheck(err);
    MojDbCursor cursor;
    err = db.find(query, cursor, req);
    MojTestErrExpected(err, MojErrDbPermissionDenied);
    req.abort();

    // delete
    MojString id;
    err = id.assign(_T("PermissionTest3:1"));
    MojTestErrCheck(err);
    err = permission.fromJson(MojTestPermissionDeny3);
    MojTestErrCheck(err);
    err = db.putPermissions(&permission, &permission + 1, adminReq);
    MojTestErrCheck(err);
    bool found = false;
    err = db.delKind(id, found, MojDbFlagNone, req);
    MojTestErrExpected(err, MojErrDbPermissionDenied);
    req.abort();

    // extend
    err = permission.fromJson(MojTestPermissionDeny4);
    MojTestErrCheck(err);
    err = db.putPermissions(&permission, &permission + 1, adminReq);
    MojTestErrCheck(err);

    MojObject childKind;
    err = childKind.fromJson(MojTestKind4);
    MojTestErrCheck(err);

    MojDbReq childReq(false);
    err = childReq.domain(_T("com.bar"));
    MojTestErrCheck(err);
    err = db.putKind(childKind, MojDbFlagNone, childReq);
    MojTestErrExpected(err, MojErrDbPermissionDenied);

    return MojErrNone;
}

void MojDbPermissionTest::cleanup()
{
	(void) MojRmDirRecursive(MojDbTestDir);
}
