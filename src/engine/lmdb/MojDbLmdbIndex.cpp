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

#include "core/MojLogDb8.h"
#include "engine/lmdb/MojDbLmdbIndex.h"
#include "engine/lmdb/MojDbLmdbQuery.h"

MojDbLmdbIndex::MojDbLmdbIndex()
{
}

MojDbLmdbIndex::~MojDbLmdbIndex()
{
    MojErr err =  close();
    MojErrCatchAll(err);

}

MojErr MojDbLmdbIndex::open(const MojObject& id, MojDbLmdbDatabase* db, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(db && db->engine());

	m_id = id;
	m_db.reset(db->engine()->indexDb());
	m_primaryDb.reset(db);

	return MojErrNone;
}

MojErr MojDbLmdbIndex::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	m_db.reset();
	m_primaryDb.reset();

	return MojErrNone;
}

MojErr MojDbLmdbIndex::drop(MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbLmdbCursor cursor;
	MojErr err = cursor.open(m_db.get(), txn);
	MojErrCheck(err);
	MojDbKey prefix;
	err = prefix.assign(m_id);
	MojErrCheck(err);
	err = cursor.delPrefix(prefix);
	MojErrCheck(err);
	cursor.close();

	return MojErrNone;
}

MojErr MojDbLmdbIndex::stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbLmdbCursor cursor;
	MojErr err = cursor.open(m_db.get(), txn);
	MojErrCheck(err);
	MojDbKey prefix;
	err = prefix.assign(m_id);
	MojErrCheck(err);
	err = cursor.statsPrefix(prefix, countOut, sizeOut);
	MojErrCheck(err);
	cursor.close();

	return MojErrNone;
}

MojErr MojDbLmdbIndex::insert(const MojDbKey& key, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(txn);

	MojDbLmdbItem keyItem;
	keyItem.fromBytesNoCopy(key.data(), key.size());
	MojDbLmdbItem valItem;
	MojErr err = m_db->put(keyItem, valItem, txn, true);
#ifdef MOJ_DEBUG
	char s[1024];
	size_t size1 = keyItem.size();
	size_t size2 = valItem.size();
	MojErr err2 = MojByteArrayToHex(keyItem.data(), size1, s);
	MojErrCheck(err2);
	LOG_DEBUG("[db.mdb] mdbindexinsert: %s; keylen: %zu, key: %s ; vallen = %zu; err = %d\n",
        m_db->m_name.data(), size1, s, size2, err);
#endif
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbIndex::del(const MojDbKey& key, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(txn);

	MojDbLmdbItem keyItem;
	keyItem.fromBytesNoCopy(key.data(), key.size());

	bool found = false;
	MojErr err = m_db->del(keyItem, found, txn);

#ifdef MOJ_DEBUG
	char s[1024];
	size_t size1 = keyItem.size();
	MojErr err2 = MojByteArrayToHex(keyItem.data(), size1, s);
	MojErrCheck(err2);
	LOG_DEBUG("[db.mdb] mdbindexdel: %s; keylen: %zu, key: %s ; err = %d\n", m_db->m_name.data(), size1, s, err);

	if (!found)
        LOG_WARNING(MSGID_DB_LMDB_TXN_WARNING, 0, "mdbindexdel_warn: not found: %s", s);
#endif

	MojErrCheck(err);
	if (!found) {
		MojErrThrow(MojErrInternalIndexOnDel);
	}
	return MojErrNone;
}

MojErr MojDbLmdbIndex::find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn,
		MojRefCountedPtr<MojDbStorageQuery>& queryOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
	MojAssert(plan.get() && txn);

	MojRefCountedPtr<MojDbLmdbQuery> storageQuery(new MojDbLmdbQuery());
	MojAllocCheck(storageQuery.get());
	MojErr err = storageQuery->open(m_db.get(), m_primaryDb.get(), plan, txn);
	MojErrCheck(err);
	queryOut = storageQuery;

	return MojErrNone;
}

MojErr MojDbLmdbIndex::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());

	return m_primaryDb->beginTxn(txnOut, isWriteOp);
}
