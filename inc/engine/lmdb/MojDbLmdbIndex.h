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

#ifndef MOJDBLMDBINDEX_H_
#define MOJDBLMDBINDEX_H_

#include "db/MojDbStorageEngine.h"
#include "MojDbLmdbDatabase.h"

class MojDbLmdbIndex: public MojDbStorageIndex
{
public:
	MojDbLmdbIndex();
	virtual ~MojDbLmdbIndex();

	MojErr open(const MojObject& id, MojDbLmdbDatabase* db, MojDbStorageTxn* txn);
	MojErr close();
	MojErr drop(MojDbStorageTxn* txn);
	MojErr stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut);
	MojErr insert(const MojDbKey& key, MojDbStorageTxn* txn);
	MojErr del(const MojDbKey& key, MojDbStorageTxn* txn);
	MojErr find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn,
			MojRefCountedPtr<MojDbStorageQuery>& queryOut);
	MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
private:
	bool isOpen() const
	{
		return m_primaryDb.get() != NULL;
	}

	MojObject m_id;
	MojRefCountedPtr<MojDbLmdbDatabase> m_primaryDb;
	MojRefCountedPtr<MojDbLmdbDatabase> m_db;
};

#endif /* MOJDBLMDBINDEX_H_ */
