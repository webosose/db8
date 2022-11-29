// Copyright (c) 2014-2021 LG Electronics, Inc.
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

#ifndef MOJDBSEARCHCACHE_H_
#define MOJDBSEARCHCACHE_H_

#include "db/MojDbShardInfo.h"
#include "db/MojDbQuery.h"
#include "db/MojDbCursor.h"
#include "core/MojVector.h"
#include "core/MojThread.h"
#include <map>

class MojDbSearchCache : public MojRefCounted
{
public :
    class QueryKey{
        friend class MojDbSearchCache;

    public:
        MojUInt32 getRev() const { return m_rev; }
        void setRev(MojUInt32 rev) { m_rev = rev; }

        const MojString& getQuery() const { return m_query; }
        void setQuery(const MojString& a_query) { m_query = a_query; }
        MojErr setQuery(const MojDbQuery& query);

        const MojString& getKind() const { return m_kind; }
        void setKind(const MojString& a_kind) { m_kind = a_kind; }

        bool operator<(const QueryKey& rhsKey) const;
        bool operator==(const QueryKey& rhsKey) const;

        MojErr fromQuery(const MojDbQuery& a_query, MojUInt32 a_revision);

    private:
        MojString m_kind;
        MojUInt32 m_rev = 0U;
        MojString m_query;
    };

    friend class QueryKey;

    typedef MojVector<MojObject> IdSet;
    typedef std::map<QueryKey, IdSet> Container;

    MojDbSearchCache() :
        shardStatusChanged([this] (const MojDbShardInfo &) { return wipeWholeCache(); })
    {}

    const Container& container() const { return m_container; }
    MojErr createCache(const QueryKey& a_key, const IdSet& a_ids);
    MojErr destroyCache(const QueryKey& a_key);
    MojErr destroyCache(const MojString& a_kind);
    MojErr wipeWholeCache();
    MojErr updateCache(const QueryKey& key, const IdSet& ids);
    MojErr getIdSet(const QueryKey& a_key, IdSet& a_ids) const;

    bool contain(const QueryKey& a_key) const;

    MojDbShardInfo::Signal::EasySlot shardStatusChanged;

private :
    Container m_container;
    mutable MojThreadRwLock m_lock;
};

#endif /* MOJDBSEARCHCACHE_H_ */
