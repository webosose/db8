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
#include "engine/lmdb/MojDbLmdbTxn.h"

MojDbLmdbTxn::MojDbLmdbTxn()
: m_engine(nullptr),
  m_txn(nullptr)

{
}

MojDbLmdbTxn::~MojDbLmdbTxn()
{
	if (m_txn) {
		abort();
	}

}

MojErr MojDbLmdbTxn::abort()
{
	MojAssert(m_txn);
	LOG_TRACE("Entering function %s [%d]", __FUNCTION__,mdb_txn_id(m_txn));
	LOG_WARNING(MSGID_DB_LMDB_TXN_WARNING, 0, "lmdb: transaction aborted");

	if (m_txn) {
		mdb_txn_abort(m_txn);
		m_txn = nullptr;
	}
	return MojErrNone;
}

bool MojDbLmdbTxn::isValid()
{
	return true;
}

MojErr MojDbLmdbTxn::commitImpl()
{
	MojAssert(m_txn);
	LOG_TRACE("Entering function %s [%d]", __FUNCTION__ ,mdb_txn_id(m_txn));

	if (m_txn) {
		int dbErr = mdb_txn_commit(m_txn);
		MojLmdbErrCheck(dbErr, _T("txn->commit"));
		m_txn = nullptr;
	}

	return MojErrNone;

}

MojErr MojDbLmdbTxn::begin(MojDbLmdbEngine* eng, bool isWriteOp, MojDbStorageTxn* txnParent)
{
	MojAssert(!m_txn && eng && eng->env());
	unsigned int flags = 0;
	MDB_env* dbEnv = eng->env()->impl();
	MDB_txn* txn = nullptr;
	MDB_txn* pTxn = MojLmdbTxnFromStorageTxn(txnParent);
	if (!isWriteOp)
		flags |= MDB_RDONLY;
	int dbErr = mdb_txn_begin(dbEnv, pTxn, flags, &txn);
	MojLmdbErrCheck(dbErr, _T("env->txn_begin"));
	MojAssert(txn);
	LOG_TRACE("Entering function %s [%d] [%d]", __FUNCTION__, mdb_txn_id(txn) ,mdb_txn_id(pTxn));
	m_txn = txn;
	m_engine = eng;

	return MojErrNone;

}

MojErr MojDbLmdbTxn::reset()
{
	MojAssert(m_txn);
	mdb_txn_reset(m_txn);

	return MojErrNone;
}

MojErr MojDbLmdbTxn::renew()
{
	MojAssert(m_txn);
	int dbErr = mdb_txn_renew(m_txn);
	MojLmdbErrCheck(dbErr, _T("txn->renew"));

	return MojErrNone;
}
