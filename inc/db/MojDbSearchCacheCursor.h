// Copyright (c) 2009-2022 LG Electronics, Inc.
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


#pragma once

#include "db/MojDbDefs.h"
#include "db/MojDbCursor.h"
#include "db/MojDbObjectItem.h"
#include "db/MojDbSearchCache.h"
#include "db/MojDbQuery.h"
#include <map>

class MojDbSearchCursor : public MojDbCursor
{
public:
    MojDbSearchCursor() : m_limit(0), m_pos(nullptr), m_startPos(0), m_count(0){}
    MojDbSearchCursor(MojString locale);
    ~MojDbSearchCursor() override;
    MojErr close() override;
    MojErr get(MojDbStorageItem*& itemOut, bool& foundOut) override;
    MojErr count(MojUInt32& countOut) override;
    MojErr nextPage(MojDbQuery::Page& pageOut) override;

    MojDbCollationStrength collation() const { return m_collation; }

private:
	struct ItemComp
	{
		int operator()(const MojRefCountedPtr<MojDbObjectItem>& i1, const MojRefCountedPtr<MojDbObjectItem>& i2) const
		{
			return i1->sortKeys().compare(i2->sortKeys());
		}
	};
	struct ObjectInfo {
		const MojObject* id;
		MojUInt32 seqNum;
	};
	typedef MojSet<MojDbKey> KeySet;
	typedef MojSet<MojObject> ObjectSet;
	typedef MojMap<MojUInt32, MojSharedPtr<ObjectSet> > GroupMap;
	typedef MojMap<MojUInt32, MojRefCountedPtr<MojDbObjectItem> > SequencedItems;
	typedef MojVector<MojRefCountedPtr<MojDbObjectItem>, MojEq<MojRefCountedPtr<MojDbObjectItem> >, ItemComp > ItemVec;

	static const MojUInt32 MaxResults = 10000;

	virtual MojErr setPagePosition();
	virtual MojErr getIds(MojDbSearchCache::IdSet& sortedId);
	virtual MojErr loadFromCache(const MojDbSearchCache* cache);
	virtual MojErr init(const MojDbQuery& query);

    MojErr retrieveCollation(const MojDbQuery& query);
	bool loaded() const { return m_pos != NULL; }
	MojErr begin();
	MojErr load(MojDbSearchCache* a_cache, bool fromCache);
	MojErr loadIds(ObjectSet& idsOut);
	MojErr loadObjects(const ObjectSet& ids);
	static void searchThread_caller(void *arg, void *user_data);
	MojErr searchThread_callee(const ObjectInfo *a_info);
        MojErr loadWithImmediateReturn();
	MojErr sort();
	MojErr distinct();
    const MojDbQuery::Page& page() const { return m_page; }

	ItemVec m_items;
	MojString m_orderProp;
	MojString m_distinct;
	MojUInt32 m_limit;
        MojUInt32 m_startPos;
	ItemVec::ConstIterator m_pos;
	ItemVec::ConstIterator m_limitPos;
	MojString m_locale;
	SequencedItems m_sequencedItems;
    MojDbCollationStrength m_collation;
    MojDbQuery::Page m_page;
    MojObject m_pageObject;
    MojUInt32 m_count;
    MojDbSearchCache::QueryKey m_queryKey;
    MojDbQuery m_cacheQuery;
    MojThreadMutex m_items_mutex;
};
