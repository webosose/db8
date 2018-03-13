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

#ifndef MOJLMDBSEQ_H_
#define MOJLMDBSEQ_H_

#include <mutex>
#include <atomic>

#include "db/MojDbStorageEngine.h"
#include "engine/lmdb/MojDbLmdbDatabase.h"

class MojDbLmdbSeq : public MojDbStorageSeq
{
public:
	MojDbLmdbSeq()
	: m_db(nullptr),
	  m_next(0),
	  m_allocated(0)
	{
	}
	~MojDbLmdbSeq();

	MojErr open(const MojChar* name, MojDbStorageTxn* txn,
			MojDbLmdbDatabase* db);
	MojErr close(MojDbStorageTxn* txn = nullptr);
	MojErr get(MojInt64& valOut ,MojDbStorageTxn* txn);

private:

	MojErr store(MojUInt64 next ,MojDbStorageTxn* txn);
	MojDbLmdbDatabase* m_db;
	MojDbLmdbItem m_key;
	std::atomic<MojUInt64> m_next; // requires atomic increment
	std::atomic<MojUInt64> m_allocated; // requires atomic load/store
	std::mutex m_allocationLock;
};

#endif /* MOJLMDBSEQ_H_ */
