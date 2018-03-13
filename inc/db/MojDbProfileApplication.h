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

#pragma once

#include "core/MojObject.h"
#include "db/MojDbDefs.h"
#include "db/MojDbReq.h"

class MojDb;

class MojDbProfileApplication
{
public:
	enum class AdminState {ADMIN_STATE_UNRESTRICTED, ADMIN_STATE_FORCE_ENABLED, ADMIN_STATE_FORCE_DISABLED};
	MojDbProfileApplication();

	static MojErr init(MojDb* db, MojDbReqRef req);

	const MojString& application() const { return m_application; }
	const MojString& kindName() const { return m_kindName; }
	bool enabled() const { return m_enabled; }
	MojErr canProfile(bool* profileEnabled);
	bool unrestricted() const { return (m_adminState == AdminState::ADMIN_STATE_UNRESTRICTED); }

	MojErr save(MojDb* db, MojDbReqRef req);
	MojErr load(const MojChar* application, MojDb* db, MojDbReqRef req, bool* found);
	MojErr drop(MojDb* db, MojDbReqRef req, bool* found);
	MojErr application(const MojChar* application);
	MojErr kindName(const MojChar* kindName);
	MojErr enabled(const bool enabled);
	MojErr setAdminState(const AdminState adminState);
	AdminState adminState() const { return m_adminState;}
	void clear();
	bool empty() const { return m_empty; }
private:
	int toInt(const AdminState state) const;
	AdminState toAdminState(const int state) const;

	MojObject m_data;
	MojString m_application;
	MojString m_kindName;		// <-- associated kind name for this application
	bool m_enabled;
	bool m_empty;
	AdminState m_adminState;

	static const MojChar* KindId;
	static const MojChar* KindJson;
	static const MojChar* KeyApplication;
	static const MojChar* KeyKindName;
	static const MojChar* KeyEnabled;
	static const MojChar* KeyAdminState;
};
