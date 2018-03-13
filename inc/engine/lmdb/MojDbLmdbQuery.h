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

#ifndef MOJDBLMDBQUERY_H_
#define MOJDBLMDBQUERY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbIsamQuery.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"
#include "engine/lmdb/MojDbLmdbCursor.h"
#include "engine/lmdb/MojDbLmdbItem.h"

class MojDbLmdbQuery: public MojDbIsamQuery
{
public:
	MojDbLmdbQuery();
	~MojDbLmdbQuery();

	MojErr open(MojDbLmdbDatabase* db, MojDbLmdbDatabase* joinDb, MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn);
	MojErr getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut);
	MojErr getById(const MojObject& id, MojObject& itemOut, bool& foundOut, MojDbKindEngine* kindEngine);
	MojErr close();

private:
	static const MojUInt32 OpenFlags;
	static const MojUInt32 SeekFlags;
	static const MojUInt32 SeekIdFlags;
	static const MojUInt32 SeekEmptyFlags[2];
	static const MojUInt32 NextFlags[2];

	MojErr seekImpl(const ByteVec& key, bool desc, bool& foundOut);
	MojErr next(bool& foundOut);
	MojErr getVal(MojDbStorageItem*& itemOut, bool& foundOut);
	MojErr getKey(bool& foundOut, MojUInt32 flags);

	MojDbLmdbCursor m_cursor;
	MojDbLmdbItem m_key;
	MojDbLmdbItem m_val;
	MojDbLmdbItem m_primaryVal;
	MojDbLmdbDatabase* m_db;
};

#endif /* MOJDBLMDBQUERY_H_ */
