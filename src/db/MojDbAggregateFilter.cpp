// Copyright (c) 2015-2021 LG Electronics, Inc.
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


#include "db/MojDbAggregateFilter.h"
#include "core/MojLogDb8.h"
#include "db/MojDb.h"

MojDbAggregateFilter::MojDbAggregateFilter(): m_desc(false)
{
}

MojDbAggregateFilter::~MojDbAggregateFilter()
{
}

MojErr MojDbAggregateFilter::init(const MojDbQuery& query)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    m_aggregateProps = query.aggregate();
    m_groupByProps = query.groupBy();
    m_desc = query.desc();

    return MojErrNone;
}

MojErr MojDbAggregateFilter::initAggregateInfo(MojObject obj, MojObject val, MojByte op, AggregateInfo& aggregateInfoOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    aggregateInfoOut.m_op = op;
    aggregateInfoOut.m_count = 1;
    aggregateInfoOut.m_min = val;
    aggregateInfoOut.m_max =val;
    aggregateInfoOut.m_first = obj;
    aggregateInfoOut.m_last = obj;

    MojObject::Type valType = val.type();
    // sum & avg, it should be number type
    if ((op & MojDbQuery::OpSum) == MojDbQuery::OpSum
        || (op & MojDbQuery::OpAvg) == MojDbQuery::OpAvg) {
        // type check
        if (valType != MojObject::TypeInt && valType != MojObject::TypeDecimal) {
            MojErrThrowMsg(MojErrDbInvalidAggregateType, _T("db: property type of sum and avg should be number"));
        }
        // integer
        if (valType == MojObject::TypeInt) {
            aggregateInfoOut.m_sum.assign(val.intValue(), 0);
            aggregateInfoOut.m_avg = static_cast<MojDouble>(val.intValue());
        // decimal
        } else {
            aggregateInfoOut.m_sum = val.decimalValue();
            aggregateInfoOut.m_avg = val.decimalValue().floatValue();
        }
    }

    return MojErrNone;
}

MojErr MojDbAggregateFilter::getValue(MojString key, MojObject obj, MojObject& valOut, bool& found)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojVector<MojString> parts;
    MojErr err = key.split(_T('.') ,parts);
    MojErrCheck(err);

    for(auto i = parts.begin(); i != parts.end(); ++i) {
        found = obj.get(i->data(), valOut);
        if(!found) return MojErrNone;
        obj = valOut;
    }

    return MojErrNone;
}


MojErr MojDbAggregateFilter::aggregate(const MojObject& obj)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    LOG_WARNING(MSGID_MOJ_SERVICE_WARNING, 0, "Agregate API is deprecated");

    MojErr err;
    MojObject groupBy;
    bool found = false;
    if(!m_groupByProps.empty()) {
        // TODO : Use only one property for groupping. Multi-property groupBy will be extended.
        MojString groupStr = *(m_groupByProps.begin());
        err = getValue(groupStr, obj, groupBy, found);
        MojErrCheck(err);
    }

    if(found && groupBy.type() == MojObject::TypeArray) {
        for (MojObject::ConstArrayIterator i = groupBy.arrayBegin(); i != groupBy.arrayEnd(); ++i) {
            err = aggregateImpl(obj, *i);
            MojErrCheck(err);
        }
    } else {
        err = aggregateImpl(obj, groupBy);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbAggregateFilter::compareKey(const MojRefCountedPtr<MojDbPropExtractor>& extractor, const MojObject& obj1, const MojObject& obj2, int& compareResult) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    SortKey key1, key2;
    err = extractor->vals(obj1, key1);
    MojErrCheck(err);
    err = extractor->vals(obj2, key2);
    MojErrCheck(err);
    compareResult = key1.compare(key2);

    return MojErrNone;
}

MojErr MojDbAggregateFilter::aggregateImpl(const MojObject& obj, const MojObject& groupBy)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    // find and get groupBy property from existing group aggregation map
    GroupAggregateMap::Iterator aggregateInfoMapIter;
    err = m_groupAggregateMap.find(groupBy, aggregateInfoMapIter);
    MojErrCheck(err);
    bool hasAggregateInfoMap = true;
    if(aggregateInfoMapIter == m_groupAggregateMap.end())
        hasAggregateInfoMap = false;

    AggregateInfoMap aggregateInfoMap;
    // Iterate to extract aggregation property from item
    for(auto i = m_aggregateProps.begin(); i != m_aggregateProps.end(); i++) {
        const MojString& propName = i.key();
        const MojDbQuery::AggregatePropInfo& propInfo = i.value();
        const MojByte& op = propInfo.m_op;

        MojObject val;
        bool found = false;
        // data extract from item
        err = getValue(propName, obj, val, found);
        MojErrCheck(err);
        if (!found) continue;
        MojObject::Type valType = val.type();

        AggregateInfo aggregateInfo;
        initAggregateInfo(obj, val, op, aggregateInfo);
        // if group does not exist, create new aggregateInfo map
        if(!hasAggregateInfoMap) {
            aggregateInfoMap.put(propName, aggregateInfo);
			continue;
        }
        AggregateInfoMap::Iterator aggregateInfoIter;
        err = aggregateInfoMapIter->find(propName, aggregateInfoIter);
        MojErrCheck(err);
        // if property does not exist, put new aggregation info
        if (aggregateInfoIter == aggregateInfoMapIter->end()) {
            aggregateInfoMapIter->put(propName, aggregateInfo);
			continue;
        }

        // count
        if ((op & MojDbQuery::OpCount) || (op & MojDbQuery::OpAvg)) {
            aggregateInfoIter->m_count++;
        }
        // min
        if (op & MojDbQuery::OpMin || op & MojDbQuery::OpFirst) {
            MojAssert(propInfo.m_extractor.get());
            int compareResult;
            err = compareKey(propInfo.m_extractor, obj, aggregateInfoIter->m_first, compareResult);
            MojErrCheck(err);
            if (compareResult < 0) {
                aggregateInfoIter->m_min = val;
                aggregateInfoIter->m_first = obj;
            }
        }
        // max
        if (op & MojDbQuery::OpMax || op & MojDbQuery::OpLast) {
            MojAssert(propInfo.m_extractor.get());
            int compareResult;
            err = compareKey(propInfo.m_extractor, obj, aggregateInfoIter->m_last, compareResult);
            MojErrCheck(err);
            if (compareResult > 0) {
                aggregateInfoIter->m_max = val;
                aggregateInfoIter->m_last = obj;
            }
        }
        // sum & avg
        if ((op & MojDbQuery::OpSum) || (op & MojDbQuery::OpAvg)) {
            // type check
            if (valType != MojObject::TypeInt && valType != MojObject::TypeDecimal) {
                MojErrThrowMsg(MojErrDbInvalidAggregateType, _T("db: property type of sum and avg should be number"));
            }
            // integer
            if (valType == MojObject::TypeInt && aggregateInfoIter->m_sum.fraction()== 0) {
                aggregateInfoIter->m_sum.assign(aggregateInfoIter->m_sum.magnitude()+val.intValue(), 0);
            // decimal
            } else {
                aggregateInfoIter->m_sum = MojDecimal(aggregateInfoIter->m_sum.floatValue() + val.decimalValue().floatValue());
            }
            // calculate avg
            aggregateInfoIter->m_avg = aggregateInfoIter->m_sum.floatValue() / static_cast<MojDouble>(aggregateInfoIter->m_count);
        }
    }
    // push group info newly created from above
    if (!hasAggregateInfoMap) {
        m_groupAggregateMap.put(groupBy, aggregateInfoMap);
    }

    return MojErrNone;
}

MojErr MojDbAggregateFilter::visit(MojObjectVisitor& visitor)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojString groupStr;
    if(!m_groupByProps.empty()) {
        groupStr = *(m_groupByProps.begin());
    }

    MojErr err;
    MojVector<MojObject> objVec;
    GroupAggregateMap::Iterator gIter;
    err = m_groupAggregateMap.begin(gIter);
    MojErrCheck(err);
    for(; gIter != m_groupAggregateMap.end(); ++gIter) {
        const MojObject& groupVal = gIter.key();
        AggregateInfoMap& aggregateInfoMap = gIter.value();

        MojObject resultObj;
        if(!groupStr.empty()) {
            MojObject groupObj;
            err = groupObj.put(groupStr, groupVal);
            MojErrCheck(err);
            err = resultObj.put(MojDbQuery::GroupByKey, groupObj);
            MojErrCheck(err);
        }
        AggregateInfoMap::Iterator iIter;
        err = aggregateInfoMap.begin(iIter);
        MojErrCheck(err);
        for(; iIter != aggregateInfoMap.end(); iIter++) {
            const MojString& propName = iIter.key();
            AggregateInfo& aggregateInfo = iIter.value();

            MojObject aggregateObj;
            bool found;
            if ((aggregateInfo.m_op & MojDbQuery::OpCount) == MojDbQuery::OpCount) {
                err = aggregateObj.putInt(MojDbQuery::CountKey , aggregateInfo.m_count);
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpMin) == MojDbQuery::OpMin) {
                err = aggregateObj.put(MojDbQuery::MinKey, aggregateInfo.m_min);
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpMax) == MojDbQuery::OpMax) {
                err = aggregateObj.put(MojDbQuery::MaxKey, aggregateInfo.m_max);
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpFirst) == MojDbQuery::OpFirst) {
                err = aggregateInfo.m_first.del(MojDb::IdKey, found);
                MojErrCheck(err);
                err = aggregateInfo.m_first.del(MojDb::RevKey, found);
                MojErrCheck(err);
                err = aggregateObj.put(MojDbQuery::FirstKey, aggregateInfo.m_first);
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpLast) == MojDbQuery::OpLast) {
                err = aggregateInfo.m_last.del(MojDb::IdKey, found);
                MojErrCheck(err);
                err = aggregateInfo.m_last.del(MojDb::RevKey, found);
                MojErrCheck(err);
                err = aggregateObj.put(MojDbQuery::LastKey, aggregateInfo.m_last);
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpSum) == MojDbQuery::OpSum) {
                if(aggregateInfo.m_sum.fraction() == 0) {
                    err = aggregateObj.put(MojDbQuery::SumKey, aggregateInfo.m_sum.magnitude());
                } else {
                    err = aggregateObj.put(MojDbQuery::SumKey, aggregateInfo.m_sum);
                }
                MojErrCheck(err);
            }
            if ((aggregateInfo.m_op & MojDbQuery::OpAvg) == MojDbQuery::OpAvg) {
                err = aggregateObj.put(MojDbQuery::AvgKey, MojDecimal(aggregateInfo.m_avg));
                MojErrCheck(err);
            }
            err = resultObj.put(propName, aggregateObj);
            MojErrCheck(err);
        }
        if(m_desc) {
            err = objVec.push(resultObj);
        } else {
            err = resultObj.visit(visitor);
        }
        MojErrCheck(err);
    }
    if(m_desc && (objVec.size() > 0)) {
        for(MojSize i=objVec.size()-1;; i--) {
            err = objVec[i].visit(visitor);
            MojErrCheck(err);
            if(0 == i) break;
        }
    }

    return MojErrNone;
}

