// Copyright (c) 2017-2018 LG Electronics, Inc.
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

/****************************************************************
*  @file TestLmdb.h
****************************************************************/

#ifndef __TestLmdb_h__
#define __TestLmdb_h__

#include "core/MojString.h"

#include "Runner.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"
#include "engine/lmdb/MojDbLmdbEngine.h"
#include "engine/lmdb/MojDbLmdbEnv.h"

#define AssertLdbOk( S ) ASSERT_TRUE( (S).ok() ) << (S).ToString();
#define ExpectLdbOk( S ) EXPECT_TRUE( (S).ok() ) << (S).ToString();

struct TestLmdb : public ::testing::Test
{
	const MojChar* const TestDbName = _T("TestDataBase.mdb");
	MojObject conf;
	MojRefCountedPtr<MojDbLmdbEnv> env = new MojDbLmdbEnv();
	MojRefCountedPtr<MojDbLmdbTxn> txn = new MojDbLmdbTxn();
	MojRefCountedPtr<MojDbLmdbDatabase> database = new MojDbLmdbDatabase();
	MojRefCountedPtr<MojDbLmdbEngine> engine = new MojDbLmdbEngine();
	void SetUp() {
		MojExpectNoErr(conf.fromJson(_T("{\"mapSize\" : 1073741824,\"maxDBCount\":6,\"noLock\":0}")));
		MojExpectNoErr(env->configure(conf));
		MojExpectNoErr(env->open(tempFolder));
		MojExpectNoErr(engine->open(nullptr, env.get()));
		MojExpectNoErr(txn->begin(engine.get(), true));
		MojExpectNoErr(database->open(TestDbName, engine.get(), txn.get()));
		MojExpectNoErr(txn->commit());
	}

	void LmdbItem(const char *key, MojDbLmdbItem &itemOut) {
		itemOut.fromBytesNoCopy((const MojByte*)key, strlen(key));
	}

	void TearDown() {
		MojExpectNoErr(database->close());
	}

};

#endif

