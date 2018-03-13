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

#include "db/MojDbProfileApplication.h"
#include "db/MojDb.h"
#include "db/MojDbKind.h"

const MojChar* MojDbProfileApplication::KindId = _T("com.palm.db.profileApps:1");
const MojChar* MojDbProfileApplication::KindJson =
			_T("{\"id\":\"com.palm.db.profileApps:1\",")
				_T("\"owner\":\"mojodb.admin\",")
				_T("\"indexes\":[{\"name\":\"application\",\"props\":[{\"name\":\"application\"}]},")
                                             _T("{\"name\":\"enabled\",\"props\":[{\"name\":\"enabled\"}]}],")
				_T("\"permissions\":[")
				_T("{")
						_T("\"operations\":{")
							_T("\"read\":\"allow\",")
							_T("\"create\":\"allow\",")
							_T("\"update\":\"allow\",")
							_T("\"delete\":\"allow\"")
					_T("}")
				_T("}")
				_T("]")
			_T("}");

const MojChar* MojDbProfileApplication::KeyApplication = _T("application");
const MojChar* MojDbProfileApplication::KeyKindName = _T("kindName");
const MojChar* MojDbProfileApplication::KeyEnabled = _T("enabled");
const MojChar* MojDbProfileApplication::KeyAdminState = _T("adminState");

MojDbProfileApplication::MojDbProfileApplication()
: m_enabled(false),
  m_empty(true),
  m_adminState(AdminState::ADMIN_STATE_UNRESTRICTED)
{
}


MojErr MojDbProfileApplication::init(MojDb* db, MojDbReqRef req)
{
	MojAssert(db);
	MojErr err;
	MojObject kindObj;
	err = kindObj.fromJson(KindJson);
	MojErrCheck(err);

	err = db->kindEngine()->putKind(kindObj, req, true);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileApplication::load(const MojChar* application, MojDb* db, MojDbReqRef req, bool* found)
{
	MojAssert(application);
	MojAssert(db);
	MojAssert(found);

	MojErr err;

	MojString val;
	err = val.assign(application);
	MojErrCheck(err);

	MojDbQuery query;

	err = query.from(KindId);
	MojErrCheck(err);

	err = query.where(KeyApplication, MojDbQuery::OpEq, val);
	MojErrCheck(err);

	MojDbCursor cursor;

	err = db->find(query, cursor, req);
	MojErrCheck(err);

	err = cursor.get(m_data, *found);
	MojErrCheck(err);

	if (*found) {
		err = m_data.getRequired(KeyApplication, m_application);
		MojErrCheck(err);

		bool kindFound;
		m_data.get(KeyKindName, m_kindName, kindFound);
		if (!found) m_kindName.clear();

		m_data.get(KeyEnabled, m_enabled);

		MojInt32 adminStateVal;
		bool adminStateFound;
		m_data.get(KeyAdminState, adminStateVal, adminStateFound);
		m_adminState = adminStateFound ? toAdminState(adminStateVal) : AdminState::ADMIN_STATE_UNRESTRICTED;

		m_empty = false;
	}

	return MojErrNone;
}

MojErr MojDbProfileApplication::save(MojDb* db, MojDbReqRef req)
{
	MojAssert(db);

	MojErr err;

	err = m_data.putString(MojDb::KindKey, KindId);
	MojErrCheck(err);

#ifdef MOJ_DEBUG
	MojString d_application;
	err = m_data.getRequired(KeyApplication, d_application);
	MojErrCheck(err);
#endif

	err = db->put(m_data, MojDbFlagNone, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileApplication::drop(MojDb* db, MojDbReqRef req, bool* found)
{
	MojAssert(db);

	MojErr err;

	MojObject id;
	err = m_data.getRequired(MojDb::IdKey, id);
	MojErrCheck(err);

	err = db->del(id, *found, MojDbFlagNone, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileApplication::application(const MojChar* application)
{
	MojAssert(application);

	MojErr err;

	err = m_application.assign(application);
	MojErrCheck(err);

	err = m_data.put(KeyApplication, m_application);
	MojErrCheck(err);

	m_empty = false;

	return MojErrNone;
}

MojErr MojDbProfileApplication::kindName(const MojChar* kindName)
{
	MojAssert(kindName);

	MojErr err;

	err = m_kindName.assign(kindName);
	MojErrCheck(err);

	err = m_data.put(KeyKindName, m_kindName);
	MojErrCheck(err);

	m_empty = false;

	return MojErrNone;
}

MojErr MojDbProfileApplication::enabled(const bool enabled)
{
	MojErr err;
	m_enabled = enabled;

	err = m_data.putBool(KeyEnabled, m_enabled);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbProfileApplication::setAdminState(const AdminState adminState)
{
	MojErr err;
	m_adminState = adminState;

	err = m_data.put(KeyAdminState, toInt(m_adminState));
	MojErrCheck(err);

	return MojErrNone;
}

void MojDbProfileApplication::clear()
{
	m_application.clear();
	m_kindName.clear();
	m_data.clear();
	m_enabled = false;
	m_empty = true;
	m_adminState = AdminState::ADMIN_STATE_UNRESTRICTED;
}

MojErr MojDbProfileApplication::canProfile(bool* profileEnabled)
{
	*profileEnabled = false;

	if (unrestricted() && enabled()) {
		*profileEnabled = true;
	}
	if (m_adminState == AdminState::ADMIN_STATE_FORCE_ENABLED) {
		*profileEnabled = true;
	}

	return MojErrNone;
}

int MojDbProfileApplication::toInt(const AdminState state) const
{
	switch (state) {
	case AdminState::ADMIN_STATE_UNRESTRICTED:
		return 0;
	case AdminState::ADMIN_STATE_FORCE_ENABLED:
		return 1;
	case AdminState::ADMIN_STATE_FORCE_DISABLED:
		return 2;
	default:
		return -1;
	}
}

MojDbProfileApplication::AdminState MojDbProfileApplication::toAdminState(const int state) const
{
	switch (state) {
	case 0:
		return AdminState::ADMIN_STATE_UNRESTRICTED;
	case 1:
		return AdminState::ADMIN_STATE_FORCE_ENABLED;
	case 2:
	default:
		return AdminState::ADMIN_STATE_FORCE_DISABLED;
	}
}
