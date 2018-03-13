// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include "db/MojDbKind.h"
#include "db/MojDbQuery.h"
#include "db/MojDbSearchCache.h"
#include <vector>

MojErr MojDbSearchCache::QueryKey::setQuery(const MojDbQuery& query)
{
    MojObject obj;
    MojErr err = query.toObject(obj);
    MojErrCheck(err);

    bool foundOut=false;
    obj.del(MojDbQuery::PageKey, foundOut);
    obj.del(MojDbQuery::LimitKey, foundOut);

    err = obj.toJson(m_query);
    MojErrCheck(err);

    setKind(query.from());
    return err;
}

MojErr MojDbSearchCache::QueryKey::fromQuery(const MojDbQuery& query, MojUInt32 a_revision)
{
    m_rev = a_revision;
    return setQuery(query);
}

bool MojDbSearchCache::contain(const QueryKey& a_key) const
{
    MojThreadReadGuard guard(m_lock);
    return (m_container.find(a_key) != m_container.end());
}

bool MojDbSearchCache::QueryKey::operator==(const QueryKey& a_rhsKey) const
{
    if ((m_rev == a_rhsKey.getRev()) &&
            m_query == a_rhsKey.m_query &&
            m_kind == a_rhsKey.m_kind)
        return true;
    return false;
}

bool MojDbSearchCache::QueryKey::operator<(const QueryKey& a_rhsKey) const
{
    int eqKind = m_kind.compare(a_rhsKey.m_kind);
    if (eqKind < 0)
        return true;

    int eqQuery = strncmp(m_query.data(), a_rhsKey.m_query.data(),
                m_query.length());
    if (eqKind == 0 && eqQuery < 0)
        return true;

    if (eqKind == 0 && eqQuery == 0 && m_rev < a_rhsKey.getRev())
        return true;

    return false;
}

MojErr MojDbSearchCache::createCache(const QueryKey& a_key, const IdSet& a_ids)
{
    MojThreadWriteGuard guard(m_lock);
    m_container.insert(Container::value_type(a_key, a_ids));
    return MojErrNone;
}

MojErr MojDbSearchCache::destroyCache(const QueryKey& a_key)
{
    MojThreadWriteGuard guard(m_lock);
    m_container.erase(a_key);
    return MojErrNone;
}

MojErr MojDbSearchCache::wipeWholeCache()
{
    MojThreadWriteGuard guard(m_lock);
    m_container.clear();
    return MojErrNone;
}

MojErr MojDbSearchCache::destroyCache(const MojString& a_kind)
{
    // TODO: Change this to use map::lower_bound or upper_bound.
    //
    MojThreadWriteGuard guard(m_lock);
    std::vector<Container::key_type> keys;
    for(auto i = m_container.begin(); i != m_container.end(); ++i)
    {
        if(i->first.m_kind == a_kind)
            keys.push_back(i->first);
    }

    for(const auto& key: keys)
    {
        m_container.erase(key);
    }

    return MojErrNone;
}

MojErr MojDbSearchCache::updateCache(const QueryKey& a_key, const IdSet& a_ids)
{
    // Skip updating the cache ONLY if up-to-date cache is available.
    //
    if (contain(a_key) == true)
        return MojErrNone;

    // Delete any cache for this query/kind here.
    //
    MojErr err = destroyCache(a_key.getKind());
    MojErrCheck(err);

    // Add new cache.
    //
    err = createCache(a_key, a_ids);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbSearchCache::getIdSet(const QueryKey& a_key, IdSet& a_ids) const
{
    MojThreadReadGuard guard(m_lock);
    Container::const_iterator citer = m_container.find(a_key);
    if (citer != m_container.end())
        a_ids = citer->second;

    return MojErrNone;
}

