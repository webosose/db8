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
*  @file TestLmdbTxn.cpp
****************************************************************/

#include "TestLmdbTxn.h"

TEST_F(TestTxn, begin) {
	MojRefCountedPtr<MojDbLmdbTxn> ttxn= new MojDbLmdbTxn();
	MojExpectNoErr(ttxn->begin(engine.get(), txn.get()));
}

TEST_F(TestTxn, abort) {
	MojDbLmdbItem key, val;
	bool found = false;
	MojRefCountedPtr<MojDbLmdbTxn> ttxn= new MojDbLmdbTxn();
	MojExpectNoErr(ttxn->begin(engine.get(), txn.get()));

	LmdbItem("name", key);
	LmdbItem("val", val);
	putData(key, val, ttxn.get(), true);
	MojExpectNoErr(ttxn->abort());

	ttxn = new MojDbLmdbTxn();
	MojExpectNoErr(ttxn->begin(engine.get(), txn.get()));

	MojExpectNoErr(database->get(key, ttxn.get(), true, val, found));
	ASSERT_TRUE(!found);
}

TEST_F(TestTxn, commit) {
	MojDbLmdbItem key, val;
	bool found = false;
	MojRefCountedPtr<MojDbLmdbTxn> ttxn= new MojDbLmdbTxn();
	MojExpectNoErr(ttxn->begin(engine.get(), txn.get()));

	LmdbItem("name", key);
	LmdbItem("val", val);
	putData(key, val, ttxn.get(), true);
	MojExpectNoErr(ttxn->commit());

	ttxn = new MojDbLmdbTxn();
	MojExpectNoErr(ttxn->begin(engine.get(), txn.get()));

	MojExpectNoErr(database->get(key, ttxn.get(), true, val, found));
	ASSERT_TRUE(found);
}
