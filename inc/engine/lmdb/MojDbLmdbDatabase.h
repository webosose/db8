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

#ifndef MOJLMDBDATABASE_H_
#define MOJLMDBDATABASE_H_

#include <lmdb.h>
#include "db/MojDbDefs.h"
#include "engine/lmdb/MojDbLmdbEngine.h"
#include "db/MojDbStorageEngine.h"
#include "engine/lmdb/MojDbLmdbItem.h"
class MojDbLmdbEngine;
class MojDbLmdbItem;

class MojDbLmdbDatabase: public MojDbStorageDatabase
{
public:
	MojDbLmdbDatabase();
	~MojDbLmdbDatabase();

	MojErr open(const MojChar* dbName, MojDbLmdbEngine* eng, MojDbStorageTxn* txn);
	MojErr close();
	MojErr drop(MojDbStorageTxn* txn);
	MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut);
	MojErr insert(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn);
	MojErr update(const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn);
	MojErr del(const MojObject& id, MojDbStorageTxn* txn, bool& foundOut);
	MojErr get(const MojObject& id, MojDbStorageTxn* txn, bool forUpdate,
			MojRefCountedPtr<MojDbStorageItem>& itemOut);
	MojErr put(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn, bool updateIdQuota);
	MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn,
			MojRefCountedPtr<MojDbStorageQuery>& queryOut);
	MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
	MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, bool isWriteOp, MojRefCountedPtr<MojDbStorageTxn>& txnOut);
	MojErr openIndex(const MojObject& id, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageIndex>& indexOut);

	MojErr put(MojDbLmdbItem& key, MojDbLmdbItem& val, MojDbStorageTxn* txn, bool updateIdQuota);
	MojErr del(MojDbLmdbItem& key, bool& foundOut, MojDbStorageTxn* txn);
	MojErr get(MojDbLmdbItem& key, MojDbStorageTxn* txn, bool forUpdate, MojDbLmdbItem& valOut, bool& foundOut);

	MDB_dbi impl()
	{
		return m_db;
	}
	MojDbLmdbEngine* engine()
	{
		return m_engine;
	}

private:
	friend class MojDbLmdbEngine;
	friend class MojDbLmdbIndex;
	void closeImpl();
	MDB_dbi m_db;
	MojDbLmdbEngine* m_engine;
	MojString m_name;
};

#endif /* MOJLMDBDATABASE_H_ */
