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
*  @file TestLmdbTxn.h
****************************************************************/

#ifndef __TestLmdbTxn_H__
#define __TestLmdbTxn_H__

#include "TestLmdb.h"

struct TestTxn: public TestLmdb
{

	void initTxn(MojRefCountedPtr<MojDbLmdbTxn> &ttxn)
	{
		const char *name = "name";
		MojDbLmdbItem key,val;
		key.fromBytesNoCopy((const MojByte*)name, strlen(name));
		const char *value = "val";
		val.fromBytesNoCopy((const MojByte*)value, strlen(value));
		MojExpectNoErr(database->put(key, val, ttxn.get(), true));
	}
	void putData(MojDbLmdbItem& key, MojDbLmdbItem& val, MojRefCountedPtr<MojDbLmdbTxn> ttxn, bool updateIdQuota){
          MojExpectNoErr(database->put(key, val, ttxn.get(), updateIdQuota));
        }
};

#endif
