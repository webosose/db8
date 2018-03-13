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
#include "engine/lmdb/MojDbLmdbErr.h"
#include "engine/lmdb/MojDbLmdbEnv.h"
#include "engine/lmdb/MojDbLmdbQuery.h"
#include "engine/lmdb/MojDbLmdbIndex.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"


MojDbLmdbDatabase::MojDbLmdbDatabase()
: m_db(0),
  m_engine(nullptr)
{

}

MojDbLmdbDatabase::~MojDbLmdbDatabase()
{
	MojErr err = close();
	MojErrCatchAll(err);

}

MojErr MojDbLmdbDatabase::open(const MojChar* dbName, MojDbLmdbEngine* eng, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(dbName && eng && txn);

	// save eng, name and file
	m_engine = eng;
	MojErr err = m_name.assign(dbName);
	MojErrCheck(err);
	// create and open db
	MDB_dbi db;
	MojUInt32 flags = 0;
	MDB_txn* dbTxn = MojLmdbTxnFromStorageTxn(txn);

	int dbErr = mdb_open(dbTxn, dbName, flags, &db);
	if (dbErr == MDB_NOTFOUND ) {
		// if open failed, we know that we had to create the db
		flags |= MDB_CREATE;
		dbErr = mdb_open(dbTxn, dbName, flags, &db);
	}
	MojLmdbErrCheck(dbErr, _T("mdb_open"));
	MojAssert(db);
	m_db = db;

	//keep a reference to this database
	err = eng->addDatabase(this);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = MojErrNone;
	if (m_db) {
		closeImpl();
		err = engine()->removeDatabase(this);
	}
	return err;
}

MojErr MojDbLmdbDatabase::drop(MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	return MojErrNone;
}

MojErr MojDbLmdbDatabase::stats(MojDbStorageTxn* txn, MojSize& countOut, MojSize& sizeOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojDbLmdbCursor cursor;
	MojErr err = cursor.open(this, txn);
	MojErrCheck(err);
	err = cursor.stats(countOut, sizeOut);
	MojErrCheck(err);
	cursor.close();
	return MojErrNone;
}

MojErr MojDbLmdbDatabase::insert(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = put(id, val, txn, true);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbLmdbDatabase::update(const MojObject& id, MojBuffer& val, MojDbStorageItem* oldVal, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(oldVal && txn);
	MojErr err = txn->offsetQuota(-(MojInt64) oldVal->size());
	MojErrCheck(err);
	err = put(id, val, txn, false);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::put(const MojObject& id, MojBuffer& val, MojDbStorageTxn* txn, bool updateIdQuota)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbLmdbItem idItem;
	MojErr err = idItem.fromObject(id);
	MojErrCheck(err);
	MojDbLmdbItem valItem;
	err = valItem.fromBuffer(val);
	MojErrCheck(err);
	err = put(idItem, valItem, txn, updateIdQuota);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::put(MojDbLmdbItem& key, MojDbLmdbItem& val, MojDbStorageTxn* txn, bool updateIdQuota)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_db && txn);
	MojErr err;
	if(txn) {
		MojInt64 quotaOffset = val.size();
		if (updateIdQuota) quotaOffset += key.size();
		err = txn->offsetQuota(quotaOffset);
		MojErrCheck(err);
	}
	MDB_txn* dbTxn = MojLmdbTxnFromStorageTxn(txn);
	int dbErr = mdb_put(dbTxn, m_db, key.impl(), val.impl(), 0);

#if defined(MOJ_DEBUG)
	size_t size1 = key.size();
	size_t size2 = val.size();
	char *s = new char[(size1*2)+1];
	MojErr err2 = MojByteArrayToHex(key.data(), size1, s);
	MojErrCheck(err2);
	char *v = new char[(size2*2)+1];
	err2 = MojByteArrayToHex(val.data(), size2, v);
	MojErrCheck(err2);
	LOG_DEBUG("[db.lmdb] lmdbput: %s; keylen: %zu; keyHex: %s; keyStr:[%s]; valHex: [%s]; valStr: [%s]; vallen = %zu; err = %d\n",
			this->m_name.data(), size1, s, (char *)(key.data()), v, (char *)(val.data()), size2, dbErr);
	delete[] v;
	delete[] s;
#endif
	MojLmdbErrCheck(dbErr, _T("db->put"));

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::del(const MojObject& id, MojDbStorageTxn* txn, bool& foundOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojDbLmdbItem idItem;
	MojErr err = idItem.fromObject(id);
	MojErrCheck(err);
	err = del(idItem, foundOut, txn);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::del(MojDbLmdbItem& key, bool& foundOut, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_db && txn);
	foundOut = false;
	MojErr err = txn->offsetQuota(-(MojInt64) key.size());
	MojErrCheck(err);
	MDB_txn* dbTxn = MojLmdbTxnFromStorageTxn(txn);
	int dbErr = mdb_del(dbTxn, m_db, key.impl(), NULL);

#if defined(MOJ_DEBUG)
	size_t size = key.size();
	char *s = new char[(size*2)+1];
	MojErr err2 = MojByteArrayToHex(key.data(), size, s);
	MojErrCheck(err2);
	LOG_DEBUG("[db.bdb] bdbdel: %s; keylen: %zu, key= %s; err= %d \n", this->m_name.data(), size, s, dbErr);
	delete[] s;
#endif

	if (dbErr != MDB_NOTFOUND) {
		MojLmdbErrCheck(dbErr, _T("db->del"));
		foundOut = true;
	}
	return MojErrNone;
}

MojErr MojDbLmdbDatabase::get(MojDbLmdbItem& key, MojDbStorageTxn* txn, bool forUpdate,
			MojDbLmdbItem& valOut, bool& foundOut)
{
	MojAssert(m_db && txn);
	foundOut = false;
	MDB_txn* dbTxn = MojLmdbTxnFromStorageTxn(txn);
	MDB_val mdbVal;
	int dbErr = mdb_get(dbTxn, m_db, key.impl(), &mdbVal);
	if (dbErr != MDB_NOTFOUND) {
		MojLmdbErrCheck(dbErr, _T("db->get"));
		valOut.fromBytes((MojByte*) mdbVal.mv_data, mdbVal.mv_size);
		foundOut = true;
	}
#if defined(MOJ_DEBUG)
	size_t size1 = key.size();
	char *s = new char[(size1*2)+1];
	MojErr err2 = MojByteArrayToHex(key.data(), size1, s);
	MojErrCheck(err2);
	LOG_DEBUG("[db.lmdb] lmdbGet: %s; keylen: %zu, key: %s ; keyStr: [%s] err = %d found: %d\n",
			this->m_name.data(), size1, s, (char *)(key.data()), dbErr, foundOut);
	delete[] s;
	if(foundOut) {
		size_t size2 = valOut.size();
		char *v = new char[(size2*2)+1];
		err2 = MojByteArrayToHex(valOut.data(), size2, v);
		MojErrCheck(err2);
		LOG_DEBUG("[db.lmdb] lmdbGet valueStr:[%s]; valhex:[%s]; valLen=%d; err = %d\n", (char *)(valOut.data()), v, size2, dbErr);
		delete[] v;
	}
#endif

	return MojErrNone;
}
MojErr MojDbLmdbDatabase::get(const MojObject& id, MojDbStorageTxn* txn, bool forUpdate,
		MojRefCountedPtr<MojDbStorageItem>& itemOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	itemOut.reset();
	MojDbLmdbItem idItem;
	MojErr err = idItem.fromObject(id);
	MojErrCheck(err);
	MojRefCountedPtr<MojDbLmdbItem> valItem(new MojDbLmdbItem);
	MojAllocCheck(valItem.get());
	bool found = false;
	err = get(idItem, txn, forUpdate, *valItem, found);
	MojErrCheck(err);

	if (found) {
		valItem->setHeader(id);
		itemOut = valItem;
	}

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::find(MojAutoPtr<MojDbQueryPlan> plan, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageQuery>& queryOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_db);

	MojRefCountedPtr<MojDbLmdbQuery> storageQuery(new MojDbLmdbQuery);
	MojAllocCheck(storageQuery.get());
	MojErr err = storageQuery->open(this, nullptr, plan, txn);
	MojErrCheck(err);
	queryOut = storageQuery;

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!txnOut.get());
	MojErr err = m_engine->beginTxn(txnOut, isWriteOp);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbDatabase::beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, bool isWriteOp, MojRefCountedPtr<MojDbStorageTxn>& txnOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!txnOut.get());
	MojErr err = m_engine->beginTxn(txnParent, isWriteOp, txnOut);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbLmdbDatabase::openIndex(const MojObject& id, MojDbStorageTxn* txn,
		MojRefCountedPtr<MojDbStorageIndex>& indexOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!indexOut.get());

	MojRefCountedPtr<MojDbLmdbIndex> index(new MojDbLmdbIndex());
	MojAllocCheck(index.get());
	MojErr err = index->open(id, this, txn);
	MojErrCheck(err);
	indexOut = index;

	return MojErrNone;
}

void MojDbLmdbDatabase::closeImpl()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	mdb_close(m_engine->env()->impl(), m_db);
	m_db = 0;
}

