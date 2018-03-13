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

#include "engine/lmdb/MojDbLmdbItem.h"
#include "core/MojLogDb8.h"
#include "core/MojObjectBuilder.h"

MojDbLmdbItem::MojDbLmdbItem()
:m_free(nullptr)
{
	MojZero(&m_dbt, sizeof(MDB_val));
}

MojErr MojDbLmdbItem::kindId(MojString& kindIdOut, MojDbKindEngine& kindEngine)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojErr err = m_header.read(kindEngine);
	MojErrCheck(err);
	kindIdOut = m_header.kindId();
	return MojErrNone;
}

MojErr MojDbLmdbItem::visit(MojObjectVisitor& visitor, MojDbKindEngine& kindEngine, bool headerExpected) const
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = MojErrNone;
	MojTokenSet tokenSet;
	if (headerExpected) {
		err = m_header.visit(visitor, kindEngine);
		MojErrCheck(err);
		const MojString& kindId = m_header.kindId();
		err = kindEngine.tokenSet(kindId, tokenSet);
		MojErrCheck(err);
		m_header.reader().tokenSet(&tokenSet);
		err = m_header.reader().read(visitor);
		MojErrCheck(err);
	} else {
		MojObjectReader reader(data(), size());
		err = reader.read(visitor);
		MojErrCheck(err);
	}
	return MojErrNone;
}

void MojDbLmdbItem::setHeader(const MojObject& id)
{
	m_header.reset();
	m_header.id(id);
	m_header.reader().data(data(), size());
}

void MojDbLmdbItem::clear()
{
	freeData();
	m_chunk.reset();
	m_free = nullptr;
	m_dbt.mv_data = nullptr;
	m_dbt.mv_size = 0;
}

bool MojDbLmdbItem::hasPrefix(const MojDbKey& prefix) const
{
	return (size() >= prefix.size() &&
		MojMemCmp(data(), prefix.data(), prefix.size()) == 0);
}

MojErr MojDbLmdbItem::toObject(MojObject& objOut) const
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObjectBuilder builder;
	MojErr err = MojObjectReader::read(builder, data(), size());
	MojErrCheck(err);
	objOut = builder.object();

	return MojErrNone;
}

void MojDbLmdbItem::fromBytesNoCopy(const MojByte* bytes, MojSize size)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(bytes || 0 == size);
	MojAssert(size <= MojUInt32Max);
	if(size == 0)
		clear();
	else
		setData(const_cast<MojByte*>(bytes), size, NULL);
}

MojErr MojDbLmdbItem::fromBuffer(MojBuffer& buf)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	clear();
	if (!buf.empty()) {
		MojErr err = buf.release(m_chunk);
		MojErrCheck(err);
		MojAssert(m_chunk.get());
		setData(m_chunk->data(), m_chunk->dataSize() ,NULL);
	}
	return MojErrNone;
}

MojErr MojDbLmdbItem::fromBytes(const MojByte* bytes, MojSize size)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert (bytes || 0 == size);
	if (size == 0) {
		clear();
	} else {
		MojByte* newBytes = (MojByte*) MojMalloc(size);
		MojAllocCheck(newBytes);
		MojMemCpy(newBytes, bytes, size);
		setData(newBytes, size, MojFree);
	}
	return MojErrNone;
}

MojErr MojDbLmdbItem::fromObject(const MojObject& obj)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojObjectWriter writer;
	MojErr err = obj.visit(writer);
	MojErrCheck(err);
	err = fromBuffer(writer.buf());
	MojErrCheck(err);
	return MojErrNone;
}

void MojDbLmdbItem::freeData()
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	if (m_dbt.mv_data != nullptr && m_free) { //if we allocate then only we free
		m_free(m_dbt.mv_data);
		m_dbt.mv_data = nullptr;
		m_dbt.mv_size = 0;
		m_free = nullptr;
	}
}

void MojDbLmdbItem::setData(MojByte* bytes, MojSize size, void (*free)(void*))
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(bytes);

	freeData();
	m_free = free;
	m_dbt.mv_size = static_cast<size_t>(size);
	m_dbt.mv_data = bytes;
	m_header.reader().data(bytes, size);
}
