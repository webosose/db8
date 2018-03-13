// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "MojDbProfileTest.h"
#include "db/MojDb.h"
#include "db/MojDbProfileStat.h"
#include "core/MojObjectBuilder.h"

static const MojChar* const TestKind =
	_T("{\"id\":\"TestKind:1\",")
		_T("\"owner\":\"mojodb.admin\",")
		_T("\"indexes\":[")
			_T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\",\"collate\":\"primary\"}]},")
			_T("{\"name\":\"barfoo\",\"props\":[{\"name\":\"bar\"},{\"name\":\"foo\",\"tokenize\":\"all\",\"collate\":\"primary\"}]},")
			_T("{\"name\":\"multiprop\",\"props\":[{\"name\":\"multiprop\",\"type\":\"multi\",\"collate\":\"primary\",\"include\":[{\"name\":\"hello\",\"tokenize\":\"all\"},{\"name\":\"world\",\"tokenize\":\"all\"}]}]}")
	_T("]}");

static const MojChar* const QueryByMethod = _T("{\"where\":[{\"prop\":\"method\",\"op\":\"=\",\"val\":\"method1\"}]}");
static const MojChar* const QueryByApplication = _T("{\"where\":[{\"prop\":\"application\",\"op\":\"=\",\"val\":\"testapp\"}]}");

const MojDbProfileEngine::duration_t TestDuration() {
	auto start = MojDbProfileEngine::clock_t::now();

	// do something here
	MojSleep(1000);

	auto end = MojDbProfileEngine::clock_t::now();

	return end - start;
}

MojDbProfileTest::MojDbProfileTest()
: MojTestCase(_T("MojDbProfile"))
{
}

MojErr MojDbProfileTest::run()
{
	MojDb db;
	MojErr err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	// add kind
	MojObject kindObj;
	err = kindObj.fromJson(TestKind);
	MojTestErrCheck(err);
	err = db.putKind(kindObj);
	MojTestErrCheck(err);

	err = enableDissableEngineTest(db);
	MojTestErrCheck(err);

	err = initEngineTest(db);
	MojTestErrCheck(err);

	err = dbReopenTest(db);
	MojTestErrCheck(err);

	err = struct_profileApplicationTest(db);
	MojTestErrCheck(err);

	err = struct_profileApplicationStatTest(db);
	MojTestErrCheck(err);

	err = profileApplicationTest(db);
	MojTestErrCheck(err);

	err = saveSomeFakeProfileDataTest(db);
	MojTestErrCheck(err);

	err = profilePermissionsTest(db);
	MojTestErrCheck(err);

	err = releaseAdminProfileTest(db);
	MojTestErrCheck(err);

	err = statsDropTest(db);
	MojTestErrCheck(err);

	err = isProfiledTest(db);
	MojTestErrCheck(err);

	err = permissionModelTest(db);
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	return MojErrNone;
}

void MojDbProfileTest::cleanup()
{
	(void) MojRmDirRecursive(MojDbTestDir);
}

MojErr MojDbProfileTest::enableDissableEngineTest(MojDb& db)
{
	MojErr err;

	MojDbProfileEngine profileEngine;

	MojObject conf;
	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	MojTestAssert(profileEngine.isEnabled());

	err = conf.fromJson(_T("{\"canProfile\" : false }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	MojTestAssert(!profileEngine.isEnabled());

	return MojErrNone;
}

MojErr MojDbProfileTest::initEngineTest(MojDb& db)
{
	MojErr err;

	MojDbProfileEngine profileEngine;

	MojObject conf;

	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	// check for admin caller
	MojDbReq req(true);
	err = req.begin(&db, true);
	MojTestErrCheck(err);

	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);

	// check if this kind REALLY exists in database
	MojDbQuery query;
	err = query.from(_T("com.palm.db.profileApps:1"));
	MojTestErrCheck(err);

	MojDbCursor cursor;
	err = db.find(query, cursor, req);
	MojTestErrCheck(err);

	bool found;
	MojObject nullObject;
	err = cursor.get(nullObject, found);
	MojTestErrCheck(err);
	MojTestAssert(!found);

	return MojErrNone;
}

MojErr MojDbProfileTest::dbReopenTest(MojDb& db)
{
	MojErr err;

	MojObject conf;
	err = conf.fromJson(_T("{ \"db\" : { \"canProfile\" : true }}"));
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	err = db.configure(conf);
	MojTestErrCheck(err);

	err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	err = db.configure(conf);
	MojTestErrCheck(err);

	err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileTest::struct_profileApplicationTest(MojDb& db)
{
	MojErr err;

	MojDbReq req;
	err = initProfileEngine(&db,req);
	MojTestErrCheck(err);
	MojDbProfileApplication profileAppNonExist;

	bool found;
	err = profileAppNonExist.load(_T("testapp"), &db, req, &found);
	MojTestErrCheck(err);
	MojTestAssert(!found);

	MojDbProfileApplication profileApp;
	err = profileApp.application(_T("testapp"));
	MojTestErrCheck(err);

	err = profileApp.kindName(_T("testapp.kind"));
	MojTestErrCheck(err);

	err = profileApp.save(&db, req);
	MojTestErrCheck(err);

	// test that this row exists in database
	MojDbProfileApplication profileApp1;
	err = profileApp1.load(_T("testapp"), &db, req, &found);
	MojTestErrCheck(err);

	MojTestAssert(found);
	MojAssert(profileApp1.application() == _T("testapp"));
	MojAssert(profileApp1.kindName() == _T("testapp.kind"));

	// test re-saving
	err = profileApp.save(&db, req);
	MojTestErrCheck(err);

	err = profileApp1.load(_T("testapp"), &db, req, &found);
	MojTestErrCheck(err);

	MojTestAssert(found);
	MojAssert(profileApp1.application() == _T("testapp"));
	MojAssert(profileApp1.kindName() == _T("testapp.kind"));

	// test updating subkind name
	err = profileApp.kindName(_T("blahkind"));
	MojTestErrCheck(err);

	err = profileApp.save(&db, req);
	MojTestErrCheck(err);

	err = profileApp1.load(_T("testapp"), &db, req, &found);
	MojTestErrCheck(err);

	MojTestAssert(found);
	MojAssert(profileApp1.application() == _T("testapp"));
	MojAssert(profileApp1.kindName() == _T("blahkind"));

	err = profileApp1.drop(&db, req, &found);
	MojTestErrCheck(err);
	return MojErrNone;
}

MojErr MojDbProfileTest::struct_profileApplicationStatTest(MojDb& db)
{
	MojErr err;

	MojDbReq req;

	MojDbProfileApplication profileAppNonExist;
	err = initProfileEngine(&db,req);
	MojTestErrCheck(err);
	bool found;
	err = profileAppNonExist.load(_T("testapp"), &db, req, &found);
	MojTestErrCheck(err);

	MojDbProfileApplication profileApp;
	err = profileApp.application(_T("testapp"));
	MojTestErrCheck(err);

	err = profileApp.kindName(_T("testapp.kind"));
	MojTestErrCheck(err);

	err = profileApp.save(&db, req);
	MojTestErrCheck(err);


	// test itself
	MojObject stat;
	MojDbProfileApplication app;
	err = app.application(_T("testapp"));
	MojTestErrCheck(err);

	err = stat.putString(_T("metric1"), _T("metric1"));
	MojTestErrCheck(err);
	err = stat.putString(_T("metric2"), _T("metric2"));
	MojTestErrCheck(err);
	err = stat.putString(_T("metric3"), _T("metric3"));
	MojTestErrCheck(err);

	MojDbProfileStat profileStat;
	err = profileStat.save(app, stat, &db, req);
	MojTestErrCheck(err);

	MojObject anotherStat;

	err = stat.putString(_T("metric4"), _T("metric4"));
	MojTestErrCheck(err);
	err = stat.putString(_T("metric5"), _T("metric5"));
	MojTestErrCheck(err);
	err = stat.putString(_T("metric6"), _T("metric6"));
	MojTestErrCheck(err);
	err = profileStat.save(app, anotherStat, &db, req);
	MojTestErrCheck(err);


	// ok, let test if this data goes into database
	MojObjectBuilder someStat;
	someStat.beginArray();

	MojObject query;
	err = profileStat.load(app, &db, req, query, &someStat);
	MojTestErrCheck(err);
	someStat.endArray();

	MojString metric1;
	err = someStat.object().arrayBegin()->getRequired(_T("metric1"), metric1);
	MojTestErrCheck(err);
	MojTestAssert( metric1.compare(_T("metric1")) == 0);

	someStat.object().toJson(metric1);

	metric1.data();

	/* store profile data to test load with query */
	MojObject testStat;
	err = testStat.putString(_T("method"), _T("method1"));
	MojTestErrCheck(err);
	err = profileStat.save(app, testStat, &db, req);
	MojTestErrCheck(err);

	MojObject anoTherTestStat;
	err = anoTherTestStat.putString(_T("method"), _T("method2"));
	MojTestErrCheck(err);
	err = profileStat.save(app, anoTherTestStat, &db, req);
	MojTestErrCheck(err);

	MojObject queryObj;
	err = queryObj.fromJson(QueryByMethod);
	MojTestErrCheck(err);

	/* test profile data with query for method */
	MojObjectBuilder queryStat;
	queryStat.beginArray();
	err = profileStat.load(app, &db, req, queryObj, &queryStat);
	MojTestErrCheck(err);
	queryStat.endArray();

	MojString method;
	err = queryStat.object().arrayBegin()->getRequired(_T("method"), method);
	MojTestErrCheck(err);
	MojTestAssert(method.compare(_T("method1")) == 0);

	/* test profile data with application for method */
	MojObject queryAppObj;
	queryAppObj.fromJson(QueryByApplication);

	MojObjectBuilder queryByApplicationStat;
	err = queryByApplicationStat.beginArray();
	MojTestErrCheck(err);

	err = profileStat.load(app, &db, req, queryAppObj, &queryByApplicationStat);
	MojTestErrCheck(err);
	err = queryByApplicationStat.endArray();
	MojTestErrCheck(err);

	MojString application;
	err = queryByApplicationStat.object().arrayBegin()->getRequired(_T("application"), application);
	MojTestErrCheck(err);
	MojTestAssert(application.compare(_T("testapp")) == 0);
	err = loadAllEnabledTest(db);
	MojTestErrCheck(err);

	err = profileApp.drop(&db, req, &found);
	MojTestErrCheck(err);
	return MojErrNone;
}

MojErr MojDbProfileTest::profileApplicationTest(MojDb& db)
{
	MojErr err;

	MojDbProfileEngine profileEngine;

	MojObject conf;

	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	// check for admin caller
	MojDbReq applicationReq(true);
	err = applicationReq.begin(&db ,true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, applicationReq);
	MojTestErrCheck(err);

	// --------------------------------------------------------------
	// test if simple application can enable profiling for itself
	// --------------------------------------------------------------
	err = applicationReq.domain(_T("com.testapp"));
	MojTestErrCheck(err);

	const MojChar* appId = _T("com.testapp");
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);

	// try to enable profile again
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);

	// and lets dissable profile
	err = profileEngine.profile(appId, false, applicationReq);
	MojTestErrCheck(err);

	// again dissable
	err = profileEngine.profile(appId, false, applicationReq);
	MojTestAssert(err == MojErrDbAppProfileDisabled);

	return MojErrNone;
}

MojErr MojDbProfileTest::saveSomeFakeProfileDataTest(MojDb& db)
{
	MojErr err;

	auto duration1 = TestDuration();
	auto duration2 = TestDuration();

	MojDbProfileEngine profileEngine;

	MojObject conf;

	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	// check for admin caller
	MojDbReq req(true);
	err = req.begin(&db, true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);

	err = req.end(true);
	MojTestErrCheck(err);

	// --------------------------------------------------------------
	// test if simple application can enable profiling for itself
	// --------------------------------------------------------------
	MojDbReq applicationReq;
	err = applicationReq.domain(_T("com.testapp"));
	MojTestErrCheck(err);

	const MojChar* appId = _T("com.testapp");
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);

	MojObject testPayload;
	err = testPayload.fromJson(_T("{\"from\" : \"com.test:1\"}"));
	MojTestErrCheck(err);

	MojString memInfoStr;
	err = profileEngine.updateMemInfo(memInfoStr);
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appId, "/", "find", testPayload, duration1, memInfoStr, applicationReq);
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appId, "/", "search", testPayload, duration2, memInfoStr, applicationReq);
	MojTestErrCheck(err);

	MojObjectBuilder builder;
	builder.beginArray();
	MojObject query;
	err = profileEngine.getStats(appId, &builder, applicationReq, query);
	MojTestErrCheck(err);
	builder.endArray();

	MojObject& prof = builder.object();

	MojString dta;
	err = prof.arrayBegin()->getRequired(_T("method"), dta);
	MojTestErrCheck(err);
	MojTestAssert(dta.compare(_T("find")) == 0);

	MojString time;
	err = prof.arrayBegin()->getRequired(_T("time"), time);
	MojTestErrCheck(err);
	MojTestAssert(time.compare(std::to_string(std::chrono::duration<double, std::milli>(duration1).count()).data()) == 0);

	err = profileEngine.profile(appId, false, applicationReq);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileTest::profilePermissionsTest(MojDb& db)
{
	MojErr err;

	MojDbProfileEngine profileEngine;

	MojObject conf;

	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	// check for admin caller
	MojDbReq req(true);
	err = req.begin(&db,true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);

	err = req.end(true);
	MojTestErrCheck(err);

	// --------------------------------------------------------------
	// test if simple application can enable profiling for itself
	// --------------------------------------------------------------
	MojDbReq applicationReq(false);
	err = applicationReq.domain(_T("com.testapp"));
	MojTestErrCheck(err);

	const MojChar* appId = _T("com.testapp");
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);

	MojObject testPayload;
	err = testPayload.fromJson(_T("{\"from\" : \"com.test:1\"}"));
	MojTestErrCheck(err);

	MojString memInfoStr;
	err = profileEngine.updateMemInfo(memInfoStr);
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appId, "/", "find", testPayload, TestDuration(), memInfoStr, applicationReq);
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appId, "/", "search", testPayload, TestDuration(), memInfoStr, applicationReq);
	MojTestErrCheck(err);

	MojObjectBuilder builder;
	builder.beginArray();
	MojObject query;
	err = profileEngine.getStats(appId, &builder, applicationReq, query);
	MojTestErrCheck(err);
	builder.endArray();

	MojObject& prof = builder.object();

	MojString dta;
	err = prof.arrayBegin()->getRequired(_T("method"), dta);
	MojTestErrCheck(err);
	MojTestAssert(dta.compare(_T("find")) == 0);

	err = profileEngine.profile(appId, false, applicationReq);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileTest::loadAllEnabledTest(MojDb& db)
{
	MojErr err;
	auto duration = TestDuration();
	MojDbProfileStat stat;
	MojDbReq req(true);

	MojDbProfileEngine profileEngine;
	err = req.begin(&db, true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	const MojChar* appId = _T("com.palm.test");
	err = req.domain(_T("com.palm.test"));
	MojTestErrCheck(err);
	err = profileEngine.profile(appId, true, req);
	MojTestErrCheck(err);

	MojObject testPayload;
	err = testPayload.fromJson(_T("{\"from\" : \"com.palm.test:1\"}"));
	MojTestErrCheck(err);

	MojString memInfoStr;
	err = profileEngine.updateMemInfo(memInfoStr);
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appId, "/", "find", testPayload, duration, memInfoStr, req);
	MojTestErrCheck(err);

	const MojChar* appIdTwo = _T("com.palm.testTwo");
	err = req.domain(_T("com.palm.testTwo"));
	MojTestErrCheck(err);
	err = profileEngine.profile(appIdTwo, true, req);
	MojTestErrCheck(err);

	MojObject testPayloadTwo;
	err = testPayloadTwo.fromJson(_T("{\"from\" : \"com.palm.testTwo:1\"}"));
	MojTestErrCheck(err);

	err = profileEngine.saveStat(appIdTwo, "/", "search", testPayloadTwo, duration, memInfoStr, req);
	MojTestErrCheck(err);

	MojObjectBuilder builder;
	builder.beginArray();
	MojObject queryAll;
	err = stat.loadAll(&db, req, queryAll, &builder);
	MojTestErrCheck(err);
	builder.endArray();
	MojObject obj = builder.object();
	MojString applicationStr;
	MojString method;
	for (MojObject::ConstArrayIterator iter = obj.arrayBegin(); iter != obj.arrayEnd(); iter++) {
		err= iter->getRequired(_T("application"), applicationStr);
		MojTestErrCheck(err);
                err = iter->getRequired(_T("method"), method);
                MojTestErrCheck(err);
                if (applicationStr == _T("com.palm.test") || (applicationStr == _T("com.palm.testTwo"))) {
			if (applicationStr == _T("com.palm.test")) {
				MojTestAssert(method.compare(_T("find")) == 0);
			} else {
				MojTestAssert(applicationStr.compare(_T("com.palm.testTwo")) == 0);
                                MojTestAssert(method.compare(_T("search")) == 0);
			}
		}

	}

	return MojErrNone;
}

MojErr MojDbProfileTest::releaseAdminProfileTest(MojDb& db)
{

	MojErr err;
	MojDbProfileEngine profileEngine;
	MojObject conf;
	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojErrCheck(err);
	err = profileEngine.configure(conf);
	MojTestErrCheck(err);
	MojDbReq req(true);
	err = req.begin(&db, true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);
	err = req.end(true);
	MojTestErrCheck(err);
	MojDbReq applicationReq(true);
	err = applicationReq.domain(_T("com.testapp"));
	MojTestErrCheck(err);
	const MojChar* appId = _T("com.testapp");
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);
	MojDbReq appReq(false);
	err = profileEngine.releaseAdminProfile(appId, appReq);
	MojTestErrExpected(err, MojErrDbAccessDenied);
	err = appReq.end(true);
	MojDbReq adminReq(true);
	err = profileEngine.releaseAdminProfile(appId, adminReq);
	MojTestErrCheck(err);
	MojDbProfileApplication app;
	bool found = false;
	err = app.load(appId, &db, adminReq, &found);
	MojTestErrCheck(err);
	MojTestAssert(found);
	if(found) {
		MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_UNRESTRICTED);
	}
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.save(&db, adminReq);
	MojTestErrCheck(err);
	err = profileEngine.releaseAdminProfile(appId, adminReq);
	MojTestErrCheck(err);
	err = app.load(appId, &db, adminReq, &found);
	MojTestErrCheck(err);
	MojTestAssert(!found);
	const MojChar* wildcardAppId = _T("*");
	err = profileEngine.releaseAdminProfile(wildcardAppId, adminReq);
	MojTestErrCheck(err);
	err = profileEngine.releaseAdminProfile(wildcardAppId, adminReq);
	MojTestErrCheck(err);
	const MojChar* App = _T("testapp");
	err= app.application(App);
	MojTestErrCheck(err);
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED);
	MojTestErrCheck(err);
	err = app.save(&db, adminReq);
	MojTestErrCheck(err);
	const MojChar* application = _T("testapplication");
	err= app.application(application);
	MojTestErrCheck(err);
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED);
	MojTestErrCheck(err);
	err = app.save(&db, adminReq);
	MojTestErrCheck(err);
	err = profileEngine.releaseAdminProfile(wildcardAppId, adminReq);
	MojTestErrCheck(err);
	err = app.load(application, &db, adminReq, &found);
	MojTestErrCheck(err);
	MojTestAssert(!found);
	return MojErrNone;
}

MojErr MojDbProfileTest::statsDropTest(MojDb& db)
{
	MojErr err;
	MojDbProfileEngine profileEngine;
	MojObject conf;
	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojTestErrCheck(err);

	err = profileEngine.configure(conf);
        MojTestErrCheck(err);

	MojDbReq req(true);
	err = req.begin(&db,true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);

	err = req.end(true);
	MojTestErrCheck(err);

	MojDbReq applicationReq(false);
	err = applicationReq.domain(_T("com.testapp"));
	MojTestErrCheck(err);

	const MojChar* appId = _T("com.testapp");
	err = profileEngine.profile(appId, true, applicationReq);
	MojTestErrCheck(err);

	MojObject testPayload;
	err = testPayload.fromJson(_T("{\"from\" : \"com.test:1\"}"));
	MojTestErrCheck(err);
	MojString memInfoStr;
	err = profileEngine.updateMemInfo(memInfoStr);
	MojTestErrCheck(err);
	//putting fake data into stats
	err = profileEngine.saveStat(appId, "/", "find", testPayload, TestDuration(), memInfoStr, applicationReq);
	MojTestErrCheck(err);

	MojDbProfileApplication app;
	MojDbReq appReq(true);
	bool found = false;
	err = app.load(appId, &db, appReq, &found);
	MojTestErrCheck(err);
	MojDbProfileStat stat;
	MojUInt32 count;
	err = stat.drop(app, &db, appReq, &count); //deleting stats data and related kind
	MojTestErrCheck(err);
	err = stat.drop(app, &db, appReq, &count); //calling drop on deleted king
	MojTestErrExpected(err, MojErrDbKindNotRegistered);
	return MojErrNone;
}

MojErr MojDbProfileTest::isProfiledTest(MojDb& db)
{

	MojErr err;
	MojDbProfileEngine profileEngine;
	MojObject conf;
	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojTestErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	MojDbReq req(true);
	err = req.begin(&db,true);
	MojTestErrCheck(err);
	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);
	err = req.end(true);
	MojTestErrCheck(err);
	MojDbProfileApplication app;
	err = app.application("com.palm.test");
	MojTestErrCheck(err);
	err = app.enabled(true);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_UNRESTRICTED);
	MojTestErrCheck(err);
	bool isProfiled;
	err = app.canProfile(&isProfiled);//testing app enble and admin unrestricted state;
	MojTestAssert(isProfiled);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED);
	MojTestErrCheck(err);
	err = app.canProfile(&isProfiled);//testing app enble and admin disable state;
	MojTestAssert(!isProfiled);
	err = app.enabled(true);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED);
	MojTestErrCheck(err);
	err = app.canProfile(&isProfiled);//testing in app enble and admin enble state;
	MojTestAssert(isProfiled);
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_UNRESTRICTED);
	MojTestErrCheck(err);
	err = app.canProfile(&isProfiled);//testing in app disable and admin unrestricted state;
	MojTestAssert(!isProfiled);
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED);
	MojTestErrCheck(err);
	err = app.canProfile(&isProfiled);//testing in app disable and admin disble state;
	MojTestAssert(!isProfiled);
	err = app.enabled(false);
	MojTestErrCheck(err);
	err = app.setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED);
	MojTestErrCheck(err);
	err = app.canProfile(&isProfiled);//testing in app disable and admin enble state;
	MojTestAssert(isProfiled);
	return MojErrNone;
}

MojErr MojDbProfileTest::permissionModelTest(MojDb& db)
{
	MojErr err;
	bool profileEnabled;
	MojDbProfileEngine profileEngine;
	const MojChar* testAppName = _T("com.profile.test");
	const MojChar* adminAppName = _T("com.palm.configurator");

	err = db.close();
	MojTestErrCheck(err);

	err = MojRmDirContent(MojDbTestDir);
	MojErrCheck(err);

	err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	MojDbReq appReq(false);
	err = appReq.domain(testAppName);
	MojTestErrCheck(err);
	MojDbReq req(true);
	err = req.domain(adminAppName);
	MojTestErrCheck(err);
	err = initializeProfileEngine(profileEngine,db, req);
	MojTestErrCheck(err);

	//default state unrestricted
	err = profileEngine.profile(testAppName, true, appReq);
	MojTestErrCheck(err);
	MojDbProfileApplication app;
	err = loadApp(app, testAppName, db, req);
	MojTestErrCheck(err);
	MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_UNRESTRICTED);
	err = app.canProfile(&profileEnabled);
	MojTestErrCheck(err);
	MojTestAssert(profileEnabled);

	//change state to admin force enable & check
	err = setAdminState(app ,MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED,db,req);
	MojTestErrCheck(err);
	err = loadApp(app, testAppName, db, req);
	MojTestErrCheck(err);
	MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED);
	err = app.canProfile(&profileEnabled);
	MojTestErrCheck(err);
	MojTestAssert(profileEnabled);

	//3rd party try to change profile state ,should fail
	err = profileEngine.profile(testAppName, false, appReq);
	MojTestErrExpected(err, MojErrDbAppProfileAdminRestriction);

	//change state to admin force disable & check
	err = setAdminState(app, MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED,db,req);
	MojTestErrCheck(err);
	err = loadApp(app, testAppName, db, req);
	MojTestErrCheck(err);
	MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED);
	err = app.canProfile(&profileEnabled);
	MojTestErrCheck(err);
	MojTestAssert(!profileEnabled);

	//3rd party try to change profile state ,should fail
	err = profileEngine.profile(testAppName, true, appReq);
	MojTestErrExpected(err, MojErrDbAppProfileAdminRestriction);

	//3rd party try to getProfile on ADMIN_STATE_FORCE_DISABLED,should fail
	MojObjectBuilder builder;
	builder.beginArray();
	MojObject query;
	err = profileEngine.getStats(testAppName, &builder, appReq, query);
	MojTestErrExpected(err, MojErrDbAppProfileAdminRestriction);

	// wildcard(*) force admin enable to all app
	err = profileEngine.profile(MojDbPermissionEngine::WildcardOperation, true, req);
	MojTestErrCheck(err);
	err = loadApp(app, testAppName, db, req);
	MojTestErrCheck(err);
	MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED);
	err = app.canProfile(&profileEnabled);
	MojTestErrCheck(err);
	MojTestAssert(profileEnabled);

	// wildcard(*) force admin disable to all app
	err = profileEngine.profile(MojDbPermissionEngine::WildcardOperation, false, req);
	MojTestErrCheck(err);
	err = loadApp(app, testAppName, db, req);
	MojTestErrCheck(err);
	MojTestAssert(app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED);
	err = app.canProfile(&profileEnabled);
	MojTestErrCheck(err);
	MojTestAssert(!profileEnabled);

	//check * app not removed after profile(*,false)
	err = loadApp(app, testAppName, db, req);
	MojTestErrExpected(err, MojErrNone);

	return MojErrNone;
}

MojErr MojDbProfileTest::initializeProfileEngine(
		MojDbProfileEngine &profileEngine, MojDb &db, MojDbReqRef req)
{
	MojErr err;
	MojObject conf;

	err = conf.fromJson(_T("{\"canProfile\" : true }"));
	MojTestErrCheck(err);

	err = profileEngine.configure(conf);
	MojTestErrCheck(err);

	err = req->begin(&db, true);
	MojTestErrCheck(err);

	err = profileEngine.open(&db, req);
	MojTestErrCheck(err);

	err = req->end(true);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileTest::loadApp(MojDbProfileApplication &app,
		const MojChar* appName, MojDb &db, MojDbReqRef req)
{
	MojErr err;
	bool found;
	err = app.load(appName, &db, req, &found);
	MojTestErrCheck(err);

	if (!found) return MojErrDbAppProfileDisabled;
	return MojErrNone;
}

MojErr MojDbProfileTest::setAdminState(MojDbProfileApplication &app,
		MojDbProfileApplication::AdminState state, MojDb &db, MojDbReqRef req)
{
	MojErr err;
	err = app.setAdminState(state);
	MojTestErrCheck(err);

	err = app.save(&db, req);
	MojTestErrCheck(err);

	return MojErrNone;
}
MojErr MojDbProfileTest::initProfileEngine(MojDb* db, MojDbReqRef req)
{
 MojErr err;
 err = req->begin(db, true);
 MojErrCheck(err);

 err = MojDbProfileApplication::init(db, req);
 MojErrCheck(err);

 err = req->end(true);
 MojErrCheck(err);

 return MojErrNone;
}
