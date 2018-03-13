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

#ifndef MOJDBLMDBCURSOR_H_
#define MOJDBLMDBCURSOR_H_

#include <lmdb.h>
#include "db/MojDbDefs.h"

class MojDbLmdbDatabase;
class MojDbLmdbItem;

class MojDbLmdbCursor: public MojNoCopy
{
public:
	MojDbLmdbCursor();
	~MojDbLmdbCursor();

	MojErr open(MojDbLmdbDatabase* db, MojDbStorageTxn* txn);
	void close();
	MojErr del();
	MojErr delPrefix(const MojDbKey& prefix);
	MojErr get(MojDbLmdbItem& key, MojDbLmdbItem& val, bool& foundOut, MDB_cursor_op flags);
	MojErr stats(MojSize& countOut, MojSize& sizeOut);
	MojErr statsPrefix(const MojDbKey& prefix, MojSize& countOut, MojSize& sizeOut);

	MDB_cursor* impl()
	{
		return m_dbc;
	}

private:
	MDB_cursor* m_dbc;
	MojDbStorageTxn* m_txn;
	MojSize m_recSize;
};

#endif /* MOJDBLMDBCURSOR_H_ */
