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
#include "engine/lmdb/MojDbLmdbCursor.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"
#include "engine/lmdb/MojDbLmdbErr.h"


MojDbLmdbCursor::MojDbLmdbCursor()
:m_dbc(nullptr),
 m_txn(nullptr),
 m_recSize(0)
{
}

MojDbLmdbCursor::~MojDbLmdbCursor()
{
	close();
}

MojErr MojDbLmdbCursor::open(MojDbLmdbDatabase* db, MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojAssert(db && txn);
	MojAssert(!m_dbc);

	MDB_dbi bdb = db->impl();
	MDB_txn* dbTxn = MojLmdbTxnFromStorageTxn(txn);
	MDB_cursor* dbc = nullptr;
	int dbErr = mdb_cursor_open(dbTxn, bdb, &dbc);
	MojLmdbErrCheck(dbErr, _T("db->cursor"));
	MojAssert(dbc);
	m_dbc = dbc;
	m_txn = txn;

	return MojErrNone;
}

void MojDbLmdbCursor::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	if (m_dbc) {
		mdb_cursor_close(m_dbc);
		m_dbc = nullptr;
		m_txn = nullptr;
	}
}

MojErr MojDbLmdbCursor::del()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_dbc);

	int dbErr = mdb_cursor_del(m_dbc, 0);
	MojLmdbErrCheck(dbErr, _T("dbc->del"));
	MojErr err = m_txn->offsetQuota(-static_cast<MojInt64>(m_recSize));
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbCursor::delPrefix(const MojDbKey& prefix)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbLmdbItem val, key;
	MojErr err = key.fromBytes(prefix.data(), prefix.size());
	MojErrCheck(err);
	bool found = false;
	err = get(key, val, found, MDB_SET_RANGE);
	MojErrCheck(err);
	while (found && key.hasPrefix(prefix)) {
		err = del();
		MojErrCheck(err);
		err = get(key, val, found, MDB_NEXT);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbLmdbCursor::get(MojDbLmdbItem& key, MojDbLmdbItem& val, bool& foundOut, MDB_cursor_op flags)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_dbc);

	MojDbLmdbItem keyItem, valItem;
	keyItem.fromBytesNoCopy(key.data(), key.size());
	valItem.fromBytesNoCopy(val.data(), val.size());
	foundOut = false;

	int dbErr = mdb_cursor_get(m_dbc, keyItem.impl(), valItem.impl(), flags);
	if (dbErr != MDB_NOTFOUND) {
		MojLmdbErrCheck(dbErr, _T("m_dbc->get"));
		foundOut = true;
		key.fromBytes(keyItem.data(), keyItem.size());
		val.fromBytes(valItem.data(), valItem.size());
		m_recSize = key.size() + val.size();
	}
	return MojErrNone;
}

MojErr MojDbLmdbCursor::stats(MojSize& countOut, MojSize& sizeOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = statsPrefix(MojDbKey(), countOut, sizeOut);
	LOG_DEBUG("[db.mdb] mdbcursor_stats: count: %d, size: %d, err: %d", (int)countOut, (int)sizeOut, (int)err);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbLmdbCursor::statsPrefix(const MojDbKey& prefix, MojSize& countOut, MojSize& sizeOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	countOut = 0;
	sizeOut = 0;
	MojDbLmdbItem val, key;
	MojErr err = key.fromBytes(prefix.data(), prefix.size());
	MojErrCheck(err);
	MojSize count = 0, size = 0;
	bool found = false;
	err = get(key, val, found, MDB_SET_RANGE);
	MojErrCheck(err);
	while (found && key.hasPrefix(prefix)) {
		++count;
		// debug code for verifying index keys
		size += key.size() + val.size();
		err = get(key, val, found, MDB_NEXT);
		MojErrCheck(err);
	}
	sizeOut = size;
	countOut = count;
	return MojErrNone;
}
