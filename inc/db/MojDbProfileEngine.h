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

#include "db/MojDbReq.h"
#include "db/MojDbProfileApplication.h"
#include <chrono>

class MojDbProfileEngine: private MojNoCopy
{
public:

	using clock_t = std::chrono::high_resolution_clock;
	using duration_t = clock_t::duration;

	MojDbProfileEngine();

	MojErr open(MojDb* db, MojDbReqRef req);
	MojErr configure(const MojObject& conf);

	MojErr saveStat(const MojChar* application, const MojChar* category, const MojChar* method, const MojObject& payload, const duration_t& duration,const MojString &info, MojDbReqRef req);
	MojErr getStats(const MojChar* application, MojObjectVisitor* visitor, MojDbReqRef req, MojObject& query);
	MojErr profile(const MojChar* application, bool enable, MojDbReqRef req);
	MojErr releaseAdminProfile(const MojChar* application, MojDbReqRef req);
	MojErr isProfiled(const MojChar* application, MojDbReqRef req, bool* enabled);
	MojErr clearProfileData(MojDbReqRef req);
	MojErr updateMemInfo(MojString &formattedStr);
	bool isEnabled() const { return m_enabled; }
	void enable(bool status) { m_enabled = status; }

private:
	MojErr setAdminStateForAll(const MojDbProfileApplication::AdminState adminState, bool clearProfile, MojDbReqRef req);
	MojErr setAdminState(const MojDbProfileApplication::AdminState adminState, MojDbProfileApplication& app, bool clearProfile, MojDbReqRef req);
	MojErr handleAdminProfileState(bool isWildcard, MojDbProfileApplication& app ,bool enable, MojDbReqRef req);
	MojErr setToUnrestricted(MojDbProfileApplication* app, MojDbReqRef req);
	MojErr dropAppAndStat(MojDbProfileApplication* app, MojDbReqRef req);
	MojErr removeWildCardApp(MojDbReqRef req);
	MojDb* m_db;
	static const MojChar* KindId;
	bool m_enabled;
};
