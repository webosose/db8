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

#include "db/MojDbProfileStat.h"
#include "db/MojDbProfileApplication.h"
#include "db/MojDb.h"
#include "db/MojDbKind.h"

const MojChar* MojDbProfileStat::KindIdSuffix = _T(".profile:1");
const MojChar* MojDbProfileStat::StatKindJson = _T("{\"indexes\":[{\"name\":\"application\",\"props\":[{\"name\":\"application\"}]},")
                                                              _T("{\"name\":\"method\",\"props\":[{\"name\":\"method\"}]}]}");
const MojChar* MojDbProfileStat::KeyApplication = _T("application");
const MojChar* MojDbProfileStat::DefaultOwner = _T("com.palm.admin");


MojErr MojDbProfileStat::formatKindName(const MojChar* application, MojString* kindName)
{
	MojAssert(application);

	MojErr err;

	err = kindName->assign(application);
	MojErrCheck(err);

	err = kindName->append(KindIdSuffix);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileStat::registerKind(MojDbProfileApplication* app, MojDb* db, MojDbReqRef req)
{
	MojAssert(db);
	MojAssert(app);
	MojAssert(!app->application().empty());

	MojErr err;

	MojString kindName;
	err = formatKindName(app->application().data(), &kindName);
	MojErrCheck(err);

	MojObject obj;

	err = obj.fromJson(StatKindJson);
	MojErrCheck(err);

	err = obj.putString(_T("id"), kindName);
	MojErrCheck(err);

	const MojChar* owner = (!req->domain().empty()) ? req->domain() : DefaultOwner;
	err = obj.putString(MojDbKind::OwnerKey, owner);
	MojErrCheck(err);

	err = db->putKind(obj, MojDbFlagNone, req);
	MojErrCheck(err);

	err = app->kindName(kindName.data());
	MojErrCheck(err);

	err = app->save(db, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileStat::save(MojDbProfileApplication& application, MojObject& stat, MojDb* db, MojDbReqRef req)
{
	MojAssert(!application.empty());
	MojAssert(db);

	MojErr err;

	if (application.kindName().empty()) {
		err = registerKind(&application, db, req);
		MojErrCheck(err);
	}

	// recheck, that kind REALLY exists in database. Lets be pation
	MojDbKind* kind;
	err = db->kindEngine()->getKind(application.kindName().data(), kind);
	MojErrCatch(err, MojErrDbKindNotRegistered) {
		err = registerKind(&application, db, req);
		MojErrCheck(err);
	}
	MojErrCheck(err);

	err = stat.putString(MojDb::KindKey, application.kindName());
	MojErrCheck(err);

	err = stat.putString(KeyApplication, application.application());
	MojErrCheck(err);

	err = db->put(stat, MojDbFlagNone, req);
	MojErrCheck(err);

	return MojErrNone;
}


MojErr MojDbProfileStat::load(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojObject& queryObj, MojDbCursor* cursor)
{
	MojAssert(!application.empty());
	MojAssert(db);
	MojAssert(cursor);

	MojErr err;

	MojDbQuery query;
	err = queryObj.putString("from",application.kindName());
	MojErrCheck(err);

	err = query.fromObject(queryObj);
	MojErrCheck(err);

	// permission logic.
	MojDbAdminGuard guard(req, false);
	if (!req->admin() && MojStrCmp(req->domain(), application.application()) == 0) {
		guard.set();
	}

	err = db->find(query, *cursor, req);
	MojErrCheck(err);

	guard.unset();

	return MojErrNone;
}

MojErr MojDbProfileStat::load(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojObject& query, MojObjectVisitor* visitor)
{
	MojAssert(!application.empty());
	MojAssert(db);
	MojAssert(visitor);

	MojErr err;
	MojDbCursor cursor;

	err = load(application, db, req, query, &cursor);
	MojErrCheck(err);

	err = cursor.visit(*visitor);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileStat::drop(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojUInt32* count)
{
	MojAssert(!application.empty());
	MojAssert(db);
	MojAssert(count);
	MojErr err;
	bool found;

	err = db->delKind(application.kindName(), found, MojDbFlagNone, req);
	MojErrCheck(err);

	MojAssert(found);

	return MojErrNone;
}

MojErr MojDbProfileStat::loadAll(MojDb* db, MojDbReqRef req, MojObject& queryObj, MojObjectVisitor* visitor)
{
	MojAssert(db);
	MojAssert(visitor);

	MojErr err;
	MojDbCursor cursor;

	MojDbQuery query;
	err = query.from(_T("com.palm.db.profileApps:1"));
	MojErrCheck(err);

	err = db->find(query, cursor, req);
	MojErrCheck(err);

	for (;;) {
		MojDbStorageItem* storageItem = nullptr;
		bool found;
		err = cursor.get(storageItem, found);
		MojErrCheck(err);
		if (!found || !storageItem) break;

		MojObject obj;
		err = storageItem->toObject(obj, *db->kindEngine(), true);
		MojErrCheck(err);

		MojString appIdStr;
		err = obj.get(_T("application"), appIdStr, found);
		MojErrCheck(err);

		if ((appIdStr.compare(MojDbPermissionEngine::WildcardOperation) == 0) || !found)
			continue;

		MojDbProfileApplication app;

		err = app.load(appIdStr, db, req, &found);
		MojErrCatchAll(err);
		if (err != MojErrNone || !found)
			continue;

		err = load(app, db, req, queryObj, visitor);
		MojErrCatchAll(err);

	}

	return MojErrNone;
}
