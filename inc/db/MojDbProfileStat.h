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

#include "core/MojCoreDefs.h"
#include "db/MojDbReq.h"

class MojDbProfileApplication;

class MojDbProfileStat
{
public:
	MojErr save(MojDbProfileApplication& application, MojObject& stat,  MojDb* db, MojDbReqRef req);
	MojErr load(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojObject& queryObj, MojDbCursor* cursor);
	MojErr load(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojObject& query, MojObjectVisitor* visitor);
	MojErr loadAll(MojDb* db, MojDbReqRef req, MojObject& query, MojObjectVisitor* visitor);
	MojErr drop(const MojDbProfileApplication& application, MojDb* db, MojDbReqRef req, MojUInt32* count);

private:
	MojErr registerKind(MojDbProfileApplication* app, MojDb* db, MojDbReqRef req);

	static MojErr formatKindName(const MojChar* application, MojString* kindName);
	static const MojChar* KindIdSuffix;

	static const MojChar* StatKindJson;
	static const MojChar* KeyApplication;
	static const MojChar* DefaultOwner;
};
