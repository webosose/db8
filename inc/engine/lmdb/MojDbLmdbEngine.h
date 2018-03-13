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

#ifndef MOJDBLMDBENGINE_H_
#define MOJDBLMDBENGINE_H_

#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"
#include "engine/lmdb/MojDbLmdbTxn.h"

class MojDbLmdbDatabase;
class MojDbLmdbEnv;
class MojDbLmdbSeq;
class MojDbLmdbEngine: public MojDbStorageEngine
{
public:
	MojDbLmdbEngine();
	virtual ~MojDbLmdbEngine();

	MojErr configure(const MojObject& conf);
	MojErr drop(const MojChar* path, MojDbStorageTxn* txn);
	MojErr open(const MojChar* path);
	MojErr open(const MojChar* path, MojDbEnv* env);
	MojErr close();
	MojErr compact();
	MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnOut, bool isWriteOp = false);
	MojErr beginTxn(MojRefCountedPtr<MojDbStorageTxn>& txnParent, bool isWriteOp, MojRefCountedPtr<MojDbStorageTxn>& txnOut);
	MojErr openDatabase(const MojChar* name, MojDbStorageTxn* txn,
			MojRefCountedPtr<MojDbStorageDatabase>& dbOut);
	MojErr openSequence(const MojChar* name, MojDbStorageTxn* txn, MojRefCountedPtr<MojDbStorageSeq>& seqOut);
	MojErr addDatabase(MojDbLmdbDatabase* db);
	MojErr removeDatabase(MojDbLmdbDatabase* db);
	MojErr addSeq(MojDbLmdbSeq* db);
	MojErr removeSeq(MojDbLmdbSeq* seq);
	const MojString& path() const { return m_path; }
	MojDbLmdbEnv* env() { return m_env.get(); }
	MojDbLmdbDatabase* indexDb() { return m_indexDb.get(); }

private:

	typedef MojVector<MojRefCountedPtr<MojDbLmdbDatabase> > DatabaseVec;
	typedef MojVector<MojRefCountedPtr<MojDbLmdbSeq> > SequenceVec;
	MojRefCountedPtr<MojDbLmdbEnv> m_env;
	MojRefCountedPtr<MojDbLmdbDatabase> m_indexDb;
	MojRefCountedPtr<MojDbLmdbDatabase> m_seqDb;
	MojString m_path;
	MojThreadMutex m_dbMutex;
	DatabaseVec m_dbs;
	SequenceVec m_seqs;
	bool m_isOpen;
};

#endif /* MOJDBLMDBENGINE_H_ */
