// Copyright (c) 2015-2018 LG Electronics, Inc.
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


#ifndef MOJDBAGGREGATEFILTER_H_
#define MOJDBAGGREGATEFILTER_H_

#include "db/MojDbDefs.h"
#include "db/MojDbQuery.h"
#include "db/MojDbExtractor.h"

class MojDbAggregateFilter
{
public:
    MojDbAggregateFilter();
    ~MojDbAggregateFilter();

    MojErr init(const MojDbQuery& query);
    MojErr aggregate(const MojObject& obj);
    MojErr aggregateImpl(const MojObject& obj, const MojObject& groupBy);
    MojErr visit(MojObjectVisitor& visitor);
    MojSize count() const { return m_groupAggregateMap.size(); }

private:

    struct AggregateInfo
    {
        bool operator<(const AggregateInfo& rhs) const { return m_count < rhs.m_count; }

        MojByte m_op;
        MojSize m_count;
        MojObject m_min;
        MojObject m_max;
        MojDecimal m_sum;
        MojDouble m_avg;
        MojObject m_first;
        MojObject m_last;
    };

    typedef MojSet<MojDbKey> SortKey;
    typedef MojMap<MojString, AggregateInfo> AggregateInfoMap;
    typedef MojMap<MojObject, AggregateInfoMap> GroupAggregateMap;

    MojErr initAggregateInfo(MojObject obj, MojObject val, MojByte op, AggregateInfo& aggregateInfoOut);
    MojErr getValue(MojString key, MojObject obj, MojObject& valOut, bool& found);
    MojErr compareKey(const MojRefCountedPtr<MojDbPropExtractor>& extractor, const MojObject& obj1, const MojObject& obj2, int& compareResult) const;

    MojDbQuery::AggregateMap m_aggregateProps;
    MojDbQuery::StringSet m_groupByProps;
    GroupAggregateMap m_groupAggregateMap;
    bool m_desc;
};

#endif /* MOJDBQUERYFILTER_H_ */
