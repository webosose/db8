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

#ifndef MOJDBLMDBTXN_H_
#define MOJDBLMDBTXN_H_

#include <lmdb.h>
#include <db/MojDbStorageEngine.h>
#include <engine/lmdb/MojDbLmdbEngine.h>
class MojDbLmdbEngine;
class MojDbLmdbTxn: public MojDbStorageTxn
{
public:
	MojDbLmdbTxn();
	~MojDbLmdbTxn();

	MojErr begin(MojDbLmdbEngine* eng, bool isWriteOp, MojDbStorageTxn* txnParent = nullptr);
	MojErr abort();
	bool isValid();
	MDB_txn* impl()
	{
		return m_txn;
	}
	MojDbLmdbEngine* engine()
	{
		return m_engine;
	}
	MojErr reset();
	MojErr renew();
private:
	virtual MojErr commitImpl();

	MojDbLmdbEngine* m_engine;
	MDB_txn* m_txn;
};

#endif /* MOJDBLMDBTXN_H_ */

