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

#ifndef MOJDBLMDBITEM_H_
#define MOJDBLMDBITEM_H_

#include <lmdb.h>
#include "db/MojDbStorageEngine.h"
#include "db/MojDbObjectHeader.h"

class MojDbLmdbItem: public MojDbStorageItem
{
public:
	MojDbLmdbItem();
	virtual ~MojDbLmdbItem()
	{
		clear();
	}
	virtual MojErr close()
	{
		return MojErrNone;
	}
	virtual MojErr kindId(MojString& kindIdOut, MojDbKindEngine& kindEngine);
	virtual MojErr visit(MojObjectVisitor& visitor, MojDbKindEngine& kindEngine, bool headerExpected = true) const;
	virtual const MojObject& id() const
	{
		return m_header.id();
	}
	virtual MojSize size() const
	{
		return m_dbt.mv_size;
	}
	const MojByte* data() const
        {
                return static_cast<const MojByte*>(m_dbt.mv_data);
        }
	bool hasPrefix(const MojDbKey& prefix) const;
	MDB_val* impl() { return &m_dbt; }
	MojErr toObject(MojObject& objOut) const;
	MojErr fromBuffer(MojBuffer& buf);
	MojErr fromBytes(const MojByte* bytes, MojSize size);
	MojErr fromObject(const MojObject& obj);
	void setHeader(const MojObject& id);
        void fromBytesNoCopy(const MojByte* bytes, MojSize size);
	void clear();
private:
	void freeData();
	void setData(MojByte* bytes, MojSize size, void (*free)(void*));

	void (*m_free)(void*);
	MDB_val m_dbt;
	MojAutoPtr<MojBuffer::Chunk> m_chunk;
	mutable MojDbObjectHeader m_header;
};

#endif /* MOJDBLMDBITEM_H_ */
