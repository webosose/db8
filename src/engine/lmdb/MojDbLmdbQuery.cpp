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
#include "engine/lmdb/MojDbLmdbQuery.h"

const MojUInt32 MojDbLmdbQuery::SeekFlags = MDB_SET_RANGE;
const MojUInt32 MojDbLmdbQuery::SeekIdFlags = MDB_GET_BOTH_RANGE;
const MojUInt32 MojDbLmdbQuery::NextFlags[2] = {MDB_NEXT, MDB_PREV};
const MojUInt32 MojDbLmdbQuery::SeekEmptyFlags[2] = {MDB_FIRST, MDB_LAST};

MojDbLmdbQuery::MojDbLmdbQuery()
:m_db(nullptr)
{
}

MojDbLmdbQuery::~MojDbLmdbQuery()
{
	MojErr err = close();
	MojErrCatchAll(err);
}

MojErr MojDbLmdbQuery::open(MojDbLmdbDatabase* db, MojDbLmdbDatabase* joinDb, MojAutoPtr<MojDbQueryPlan> plan,
							MojDbStorageTxn* txn)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(!m_isOpen);
	MojAssert(db && db->impl() && plan.get());

	MojErr err = MojDbIsamQuery::open(plan, txn);
	MojErrCheck(err);
	err = m_cursor.open(db, txn);
	MojErrCheck(err);

	m_db = joinDb;

	return MojErrNone;

}

MojErr MojDbLmdbQuery::close()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = MojErrNone;

	if (m_isOpen) {
		MojErr errClose = MojDbIsamQuery::close();
		MojErrAccumulate(err, errClose);
		m_cursor.close();
		m_db = nullptr;
		m_isOpen = false;
	}
	return err;

}

MojErr MojDbLmdbQuery::getById(const MojObject& id, MojDbStorageItem*& itemOut, bool& foundOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	itemOut = nullptr;
	foundOut = false;
	MojDbLmdbItem* item = nullptr;
	MojErr err;

	if (m_db) {
		// retrun val from primary db
		MojDbLmdbItem primaryKey;
		err = primaryKey.fromObject(id);
		MojErrCheck(err);
		err = m_db->get(primaryKey, m_txn, false, m_primaryVal, foundOut);
		MojErrCheck(err);
		if (!foundOut) {
#if defined(MOJ_DEBUG)
			char *s;
			int size = static_cast<int>(primaryKey.size());
			s = new char[(size*2)+1];
			(void) MojByteArrayToHex(primaryKey.data(), size, s);
			LOG_DEBUG("[db.mdb] mdb get_byId_warnindex: KeySize: %d; %s ;id: %s \n",
					size, s, primaryKey.data() + 1);
			delete[] s;
#endif
			MojErrThrow(MojErrInternalIndexOnFind);
		}
		item = &m_primaryVal;
	} else {
		item = &m_val;
	}
	item->setHeader(id);

	bool exclude = false;
	err = checkExclude(item, exclude);
	MojErrCheck(err);
	if (!exclude) {
		itemOut = item;
	}
	return MojErrNone;
}

MojErr MojDbLmdbQuery::getById(const MojObject& id, MojObject& itemOut, bool& foundOut, MojDbKindEngine* kindEngine)
{

    foundOut = false;
    if (!m_db)
        return MojErrNotOpen;

    MojDbLmdbItem primaryKey, primaryVal;
    MojDbStorageItem* item = &primaryVal;
    MojErr err = primaryKey.fromObject(id);
    MojErrCheck(err);
    err = m_db->get(primaryKey, m_txn, false, primaryVal, foundOut);
    MojErrCheck(err);
    if (!foundOut) {
        char s[1024];
        size_t size = primaryKey.size();
        (void) MojByteArrayToHex(primaryKey.data(), size, s);
        LOG_DEBUG("[db_mdb] mdbq_byId_warnindex: KeySize: %zu; %s ;id: %s \n", size, s, primaryKey.data()+1);

        MojErrThrow(MojErrInternalIndexOnFind);
    }
    primaryVal.setHeader(id);

    foundOut = false;
    // check for exclusions
    bool exclude = false;
    err = checkExclude(item, exclude);
    MojErrCheck(err);
    if (!exclude) {
        err=item->toObject(itemOut, *kindEngine);
      MojErrCheck(err);
        foundOut = true;
    }

    return MojErrNone;
}


MojErr MojDbLmdbQuery::seekImpl(const ByteVec& key, bool desc, bool& foundOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err;
	if (key.empty()) {
		// if key is empty, seek to beginning (or end if desc)
		err = getKey(foundOut, static_cast<MDB_cursor_op>(SeekEmptyFlags[desc]));
		MojErrCheck(err);
	} else {
		// otherwise seek to the key
		err = m_key.fromBytes(key.begin(), key.size());
		MojErrCheck(err);
		err = getKey(foundOut, SeekFlags);
		MojErrCheck(err);
		// if descending, skip the first result (which is outside the range)
		if (desc) {
			err = next(foundOut);
			MojErrCheck(err);
		}
	}
	return MojErrNone;
}

MojErr MojDbLmdbQuery::next(bool& foundOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(m_state == StateNext);

	MojErr err = getKey(foundOut, NextFlags[m_plan->desc()]);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbQuery::getVal(MojDbStorageItem*& itemOut, bool& foundOut)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject id;
	MojErr err = parseId(id);
	MojErrCheck(err);
	err = getById(id, itemOut, foundOut);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLmdbQuery::getKey(bool& foundOut, MojUInt32 flags)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = m_cursor.get(m_key, m_val, foundOut, static_cast<MDB_cursor_op>(flags));
	MojErrCheck(err);
	m_keyData = m_key.data();
	m_keySize = m_key.size();
	return MojErrNone;
}
