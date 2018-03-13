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

#include "db/MojDbProfileEngine.h"
#include "db/MojDb.h"
#include "db/MojDbProfileStat.h"
#include <sys/sysinfo.h>
#include <core/MojUtil.h>
#include <sstream>

#define KILOBYTE  1024
#define PAGES_TO_KILOBYTE(pages) (pages * sysconf(_SC_PAGESIZE))/KILOBYTE
const MojChar* MojDbProfileEngine::KindId = _T("com.palm.db.profileApps:1");

MojDbProfileEngine::MojDbProfileEngine()
: m_db(nullptr),
  m_enabled(false)
{
}

MojErr MojDbProfileEngine::open(MojDb* db, MojDbReqRef req)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(db);

	MojErr err;

	m_db = db;

	err = MojDbProfileApplication::init(db, req);
	MojErrCheck(err);
	if(!isEnabled()) {
		err = clearProfileData(req);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::configure(const MojObject& conf)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	if (!conf.get(_T("canProfile"), m_enabled)) {
		m_enabled = false;
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::saveStat(const MojChar* application, const MojChar* category, const MojChar* method, const MojObject& payload, const duration_t& duration,const MojString &info, MojDbReqRef req)
{
	MojAssert(m_db);
	MojAssert(application);
	MojAssert(method);

	MojErr err;

	// permission logic.
	MojDbAdminGuard guard(req, false);
	if (!req->admin() && MojStrCmp(req->domain(), application) == 0) {
		guard.set();
	}

	bool found;
	MojDbProfileApplication app;
	err = app.load(application, m_db, req, &found);
	MojErrCheck(err);

	if (!found)
		MojErrThrow(MojErrNotFound);

	MojObject obj;

	err = obj.putString(_T("application"), application);
	MojErrCheck(err);

	err = obj.putString(_T("category"), category);
	MojErrCheck(err);

	err = obj.putString(_T("method"), method);
	MojErrCheck(err);

	MojString durationStr;
	err = durationStr.assign(std::to_string(std::chrono::duration<double, std::milli>(duration).count()).data());
	MojErrCheck(err);

	err = obj.putString(_T("time"), durationStr);
	MojErrCheck(err);

	err = obj.put(_T("payload"), payload);
	MojErrCheck(err);

	err = obj.putString(_T("memory_info"), info);
	MojErrCheck(err);

	MojDbProfileStat stat;
	err = stat.save(app, obj, m_db, req);
	MojErrCheck(err);


	return MojErrNone;
}

MojErr MojDbProfileEngine::profile(const MojChar* application, bool enable, MojDbReqRef req)
{
	MojAssert(m_db);
	MojAssert(application);

	MojErr err;
	bool found, wildcardAppFound;
	bool isAdminCaller, isSelfProfiling;

	MojDbProfileApplication app;
	MojDbProfileApplication wildcardApp;

	isAdminCaller = req->admin();
	isSelfProfiling = (MojStrCmp(req->domain(), application) == 0);

	MojDbAdminGuard guard(req, false);
	if (!isAdminCaller && isSelfProfiling) {
		guard.set();
	}

	err = app.load(application, m_db, req, &found);
	MojErrCheck(err);

	if (!enable && !found) {
		MojErrThrow(MojErrDbAppProfileDisabled);
	}

	err = wildcardApp.load(MojDbPermissionEngine::WildcardOperation, m_db, req, &wildcardAppFound);
	MojErrCheck(err);

	if (!isAdminCaller && (!app.unrestricted() || wildcardAppFound)) {
		return MojErrDbAppProfileAdminRestriction;
	}

	bool isWildCardOperation = (MojStrCmp(MojDbPermissionEngine::WildcardOperation, application)==0);

	if (enable) {	// enable profile
		if (!found) {
			err = app.application(application);
			MojErrCheck(err);
		}
		if (isAdminCaller && !isSelfProfiling) {// * & 3rd_party app by admin
			err = handleAdminProfileState(isWildCardOperation, app, enable, req);
			MojErrCheck(err);
			return MojErrNone;
		}
		err = app.enabled(enable);
		MojErrCheck(err);

		err = app.save(m_db, req);
		MojErrCheck(err);
	} else {
		if (isAdminCaller && !isSelfProfiling) {// * & 3rd_party app by admin
			err = handleAdminProfileState(isWildCardOperation, app, enable, req);
			MojErrCheck(err);
			return MojErrNone;
		}
		// drop
		MojDbProfileStat stat;
		if (!app.kindName().empty()) {
			MojUInt32 count;
			err = stat.drop(app, m_db, req, &count);
			MojErrCheck(err);
		}
		err = app.drop(m_db, req, &found);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::releaseAdminProfile(const MojChar* application, MojDbReqRef req)
{
	MojAssert(application);
	MojErr err;

	if (!req->admin())
	{
		return MojErrDbAccessDenied;
	}
	if (MojStrCmp(application,MojDbPermissionEngine::WildcardOperation) == 0)
	{
		MojDbQuery query;
		err = query.from(KindId);
		MojErrCheck(err);
		MojDbCursor cursor;
		err = m_db->find(query, cursor, req);
		MojErrCheck(err);
		for (;;) {
			MojDbStorageItem* storageItem = nullptr;
			bool found = false;
			err = cursor.get(storageItem, found);
			MojErrCheck(err);
			if (!found || !storageItem) break;

			MojObject obj;
			err = storageItem->toObject(obj, *m_db->kindEngine(), true);
			MojErrCheck(err);

			MojString appIdStr;
			err = obj.getRequired(_T("application"), appIdStr);
			MojErrCheck(err);
			MojDbProfileApplication app;
			err = app.load(appIdStr, m_db, req, &found);
			MojErrCheck(err);
			if (found) {
				if (!app.enabled()) {
					err = dropAppAndStat(&app, req);
				}
				else {
					err = setToUnrestricted(&app, req);
				}
				MojErrCheck(err);
			}
		}
		err = removeWildCardApp(req);
		MojErrCheck(err);

	} else {
		bool appFound;
		MojDbProfileApplication app;
		err = app.load(application, m_db, req, &appFound);
		MojErrCheck(err);
		if (!appFound) {
			return MojErrDbAppProfileDisabled;
		}
		if (!app.enabled()) {
			err = dropAppAndStat(&app, req);
		} else {
			err = setToUnrestricted(&app, req);
		}
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::dropAppAndStat(MojDbProfileApplication* app, MojDbReqRef req)
{
	MojAssert(app);
	MojErr err;
	bool isAppDroped;
	if (!app->kindName().empty()) {
		MojDbProfileStat stat;
		MojUInt32 count;
		err = stat.drop(*app, m_db, req, &count);
		MojErrCheck(err);
	}
	err = app->drop(m_db, req, &isAppDroped);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbProfileEngine::removeWildCardApp(MojDbReqRef req)
{
	MojErr err;
	bool isAppDroped, appFound;
	MojString appIdStr;
	err = appIdStr.assign(MojDbPermissionEngine::WildcardOperation);
	MojErrCheck(err);
	MojDbProfileApplication app;
	err = app.load(appIdStr, m_db, req, &appFound);
	MojErrCheck(err);
	if(appFound) {
		err = app.drop(m_db, req, &isAppDroped);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::setToUnrestricted(MojDbProfileApplication* app, MojDbReqRef req)
{
	MojAssert(app);
	MojErr err;
	err = app->setAdminState(MojDbProfileApplication::AdminState::ADMIN_STATE_UNRESTRICTED);
	MojErrCheck(err);
	err = app->save(m_db, req);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbProfileEngine::isProfiled(const MojChar* application, MojDbReqRef req, bool* enabled)
{
	MojAssert(m_db);
	MojAssert(application);
	MojAssert(enabled);
	*enabled = false;
	MojErr err;
	bool found;

	MojDbAdminGuard guard(req, false);
	if (!req->admin() && (MojStrCmp(req->domain(), application) == 0)) {
		guard.set();
	}

	MojDbProfileApplication profileApp;
	err = profileApp.load(application, m_db, req, &found);
	MojErrCheck(err);
	if (found) {
		err = profileApp.canProfile(enabled);
		MojErrCheck(err);
	} else {
		MojString adminAppId;
		err = adminAppId.assign(MojDbPermissionEngine::WildcardOperation);
		MojErrCheck(err);

		err = profileApp.load(adminAppId, m_db, req, &found);
		MojErrCheck(err);
		if (found) {
			bool wildcardEnabled;
			err = profileApp.canProfile(&wildcardEnabled);
			MojErrCheck(err);
			if (wildcardEnabled) {
				MojDbProfileApplication app;
				err = app.application(application);
				MojErrCheck(err);
				err = app.setAdminState(profileApp.adminState());
				MojErrCheck(err);
				err = app.save(m_db, req);
				MojErrCheck(err);
				*enabled = true;
			}
		}
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::getStats(const MojChar* application, MojObjectVisitor* visitor, MojDbReqRef req, MojObject& query)
{
	MojAssert(m_db);
	MojAssert(application);
	MojAssert(visitor);

	MojErr err;

	// permission logic.
	MojDbAdminGuard guard(req, false);
	if (!req->admin() && MojStrCmp(req->domain(), application) == 0) {
		guard.set();
	}

	bool found;
	MojDbProfileApplication app;

	err = app.load(application, m_db, req, &found);
	MojErrCheck(err);

	if (!found) {
		MojErrThrow(MojErrDbAppProfileDisabled);
	}

	if (app.adminState() == MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED) {
		return MojErrDbAppProfileAdminRestriction;
	}

	MojDbProfileStat stat;
	if (MojStrCmp(application, MojDbPermissionEngine::WildcardOperation) != 0) {
		err = stat.load(app, m_db, req, query, visitor);
        } else {
		err = stat.loadAll(m_db, req, query, visitor);
        }

	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileEngine::setAdminStateForAll(const MojDbProfileApplication::AdminState adminState, bool clearProfile, MojDbReqRef req)
{
	MojErr err;
	MojDbQuery query;
	err = query.from(KindId);
	MojErrCheck(err);

	MojDbCursor cursor;
	err = m_db->find(query, cursor, req);
	MojErrCheck(err);

	for (;;) {
		MojDbStorageItem* storageItem = nullptr;
		bool found = false;
		err = cursor.get(storageItem, found);
		MojErrCheck(err);
		if (!found || !storageItem) break;

		MojObject obj;
		err = storageItem->toObject(obj, *m_db->kindEngine(), true);
		MojErrCheck(err);

		MojString appIdStr;
		err = obj.getRequired(_T("application"), appIdStr);
		MojErrCheck(err);
		MojDbProfileApplication app;
		err = app.load(appIdStr, m_db, req, &found);
		MojErrCheck(err);

		if (found) {
			err = setAdminState(adminState, app, clearProfile, req);
			MojErrCheck(err);
		}
	}
	return MojErrNone;
}

MojErr MojDbProfileEngine::handleAdminProfileState(bool isWildcard, MojDbProfileApplication& app, bool enable, MojDbReqRef req)
{
	MojErr err;
	MojDbProfileApplication::AdminState adminState;
	adminState = enable ? MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_ENABLED : MojDbProfileApplication::AdminState::ADMIN_STATE_FORCE_DISABLED;

	if (isWildcard) {
		err = app.enabled(enable);
		MojErrCheck(err);

		err = app.save(m_db, req);
		MojErrCheck(err);

		err = setAdminStateForAll(adminState, !enable, req);
		MojErrCheck(err);
	} else {
		err = setAdminState(adminState, app, !enable, req);
		MojErrCheck(err);
	}

	return MojErrNone;
}

MojErr MojDbProfileEngine::setAdminState(const MojDbProfileApplication::AdminState adminState,MojDbProfileApplication& app, bool clearProfile, MojDbReqRef req)
{
	MojErr err;
	err = app.setAdminState(adminState);
	MojErrCheck(err);

	if (clearProfile && !app.kindName().empty()) {
		MojDbProfileStat stat;
		MojUInt32 count;
		err = stat.drop(app, m_db, req, &count);
		MojErrCheck(err);

		err = app.kindName(MojString::Empty);
		MojErrCheck(err);
	}

	err = app.save(m_db, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileEngine::updateMemInfo(MojString &formattedStr)
{
	MojErr err;
	unsigned long size = 0, resident = 0, share = 0;
	MojString statmStr;
	static const MojChar* statmPath = "/proc/self/statm";

	struct sysinfo memInfo;
	if (sysinfo(&memInfo) < 0) {
		return MojErrRequiredPropNotFound;
	}

	err = MojFileToString(statmPath, statmStr);
	MojErrCheck(err);

	std::stringstream s(statmStr.data());
	s >> size >> resident >> share;
	unsigned long totalMemInKB = (memInfo.totalram / KILOBYTE);
	MojDouble percentageRAMUsed = ((MojDouble) 100 / totalMemInKB) * PAGES_TO_KILOBYTE(resident);

	err = formattedStr.format("%%MEM=%.2f, VSS=%lu, RSS=%lu, SHR=%lu",
			percentageRAMUsed, PAGES_TO_KILOBYTE(size),
			PAGES_TO_KILOBYTE(resident), PAGES_TO_KILOBYTE(share));
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbProfileEngine::clearProfileData(MojDbReqRef req)
{
	MojErr err;
	MojDbQuery query;
	err = query.from(KindId);
	MojErrCheck(err);
	MojDbCursor cursor;
	err = m_db->find(query, cursor, req);
	MojErrCheck(err);
	for (;;) {
		MojDbStorageItem* storageItem = nullptr;
		bool found = false;
		err = cursor.get(storageItem, found);
		MojErrCheck(err);
		if (!found || !storageItem) break;

		MojObject obj;
		err = storageItem->toObject(obj, *m_db->kindEngine(), true);
		MojErrCheck(err);

		MojString appIdStr;
		err = obj.getRequired(_T("application"), appIdStr);
		MojErrCheck(err);
		MojDbProfileApplication app;
		err = app.load(appIdStr, m_db, req, &found);
		MojErrCheck(err);
		err = dropAppAndStat(&app, req);
		MojErrCheck(err);
	}
	return MojErrNone;
}
