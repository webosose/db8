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
*  @file TestLmdbCursor.cpp
****************************************************************/

#include "engine/lmdb/MojDbLmdbCursor.h"
#include "TestLmdbCursor.h"

TEST_F(TestCursor, OpenAndClose) {
	MojDbLmdbCursor cursor;
	MojDbLmdbTxn txn;
	txn.begin(engine.get(), false);
	MojExpectNoErr(cursor.open(database.get(), &txn));
	cursor.close();
}

TEST_F(TestCursor, get) {
	MojDbLmdbItem key, val;
	bool found = false;
	MojDbLmdbTxn txn;
	MojDbLmdbCursor cursor;
	initDatabase();
	txn.begin(engine.get(), true);
	MojExpectNoErr(cursor.open(database.get(), &txn));
	LmdbItem("1", key);
	MojExpectNoErr(cursor.get(key, val, found, MDB_FIRST));
	ASSERT_TRUE(found);

	LmdbItem("101", key);
	MojExpectNoErr(cursor.get(key, val, found, MDB_NEXT));
	ASSERT_TRUE(!found);

}
