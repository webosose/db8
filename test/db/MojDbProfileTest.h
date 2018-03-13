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

#include "MojDbTestRunner.h"
#include "db/MojDbProfileEngine.h"
#include "db/MojDbProfileApplication.h"

class MojDbProfileTest : public MojTestCase
{
public:
	MojDbProfileTest();

	virtual MojErr run();
	virtual void cleanup();

private:
	MojErr enableDissableEngineTest(MojDb& db);
	MojErr dbReopenTest(MojDb& db);
	MojErr initEngineTest(MojDb& db);
	MojErr struct_profileApplicationTest(MojDb& db);
	MojErr struct_profileApplicationStatTest(MojDb& db);

	MojErr profileApplicationTest(MojDb& db);
	MojErr saveSomeFakeProfileDataTest(MojDb& db);

	MojErr profilePermissionsTest(MojDb& db);
	MojErr loadAllEnabledTest(MojDb& db);
	MojErr releaseAdminProfileTest(MojDb& db);
	MojErr statsDropTest(MojDb& db);
	MojErr isProfiledTest(MojDb& db);
	MojErr permissionModelTest(MojDb& db);
	MojErr initializeProfileEngine(MojDbProfileEngine &profilingEngine,MojDb &db, MojDbReqRef req);
	MojErr loadApp(MojDbProfileApplication &app, const MojChar* appName, MojDb &db, MojDbReqRef req);
	MojErr setAdminState(MojDbProfileApplication &app,MojDbProfileApplication::AdminState state ,MojDb &db,MojDbReqRef req);
	MojErr initProfileEngine(MojDb* db, MojDbReqRef req);
};
