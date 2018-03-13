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
*  @file TestLmdbCursor.h
****************************************************************/

#ifndef __TestLmdbCursor_H__
#define __TestLmdbCursor_H__

#include "TestLmdb.h"

struct TestCursor: public TestLmdb
{

	void initDatabase() {
	MojDbLmdbTxn ttxn;
	ttxn.begin(engine.get(), true);
	MojDbLmdbItem key,val;
	for(int i = 0; i < 100; i++) {
		char name[10];
		sprintf(name, "%d", i);
		key.fromBytesNoCopy((const MojByte*)name, strlen(name));
		const char *value = "val";
		key.fromBytesNoCopy((const MojByte*)value, strlen(value));
		MojExpectNoErr(database->put(key, val, &ttxn, true));
		}
	}
};

#endif
