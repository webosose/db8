// Copyright (c) 2009-2021 LG Electronics, Inc.
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


#include "db/MojDbQuery.h"
#include "db/MojDbUtils.h"
#include "db/MojDbKind.h"
#include "db/MojDbIndex.h"
#include "core/MojObjectBuilder.h"
#include "core/MojLogDb8.h"

const MojChar* const MojDbQuery::SelectKey = _T("select");
const MojChar* const MojDbQuery::FromKey = _T("from");
const MojChar* const MojDbQuery::WhereKey = _T("where");
const MojChar* const MojDbQuery::FilterKey = _T("filter");
const MojChar* const MojDbQuery::OrderByKey = _T("orderBy");
const MojChar* const MojDbQuery::DistinctKey = _T("distinct");
const MojChar* const MojDbQuery::DescKey = _T("desc");
const MojChar* const MojDbQuery::IncludeDeletedKey = _T("incDel");
const MojChar* const MojDbQuery::LimitKey = _T("limit");
const MojChar* const MojDbQuery::ImmediateReturnKey = _T("immediateReturn");
const MojChar* const MojDbQuery::PageKey = _T("page");
const MojChar* const MojDbQuery::AggregateKey = _T("aggregate");
const MojChar* const MojDbQuery::GroupByKey = _T("groupBy");
const MojChar* const MojDbQuery::CountKey = _T("cnt");
const MojChar* const MojDbQuery::MinKey = _T("min");
const MojChar* const MojDbQuery::MaxKey = _T("max");
const MojChar* const MojDbQuery::SumKey = _T("sum");
const MojChar* const MojDbQuery::AvgKey = _T("avg");
const MojChar* const MojDbQuery::FirstKey = _T("first");
const MojChar* const MojDbQuery::LastKey = _T("last");
const MojChar* const MojDbQuery::PropKey = _T("prop");
const MojChar* const MojDbQuery::OpKey = _T("op");
const MojChar* const MojDbQuery::ValKey = _T("val");
const MojChar* const MojDbQuery::CollateKey = _T("collate");
const MojChar* const MojDbQuery::DelKey = _T("_del");

//db.query

MojDbQuery::WhereClause::WhereClause()
: m_lowerOp(OpNone),
  m_upperOp(OpNone),
  m_collation(MojDbCollationInvalid)
{
}

MojErr MojDbQuery::WhereClause::setLower(CompOp op, const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (m_lowerOp != OpNone)
		MojErrThrow(MojErrDbInvalidQueryOpCombo);

	MojErr err = collation(coll);
	MojErrCheck(err);

	m_lowerOp = op;
	m_lowerVal = val;

	return MojErrNone;
}

MojErr MojDbQuery::WhereClause::setUpper(CompOp op, const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (m_upperOp != OpNone)
		MojErrThrow(MojErrDbInvalidQueryOpCombo);

	MojErr err = collation(coll);
	MojErrCheck(err);

	m_upperOp = op;
	m_upperVal = val;

	return MojErrNone;
}

MojErr MojDbQuery::WhereClause::collation(MojDbCollationStrength collation)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (m_collation != MojDbCollationInvalid && collation != m_collation)
		MojErrThrow(MojErrDbInvalidQueryCollationMismatch);
	m_collation = collation;

	return MojErrNone;
}

bool MojDbQuery::WhereClause::operator==(const WhereClause& rhs) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	return (m_lowerOp == rhs.m_lowerOp &&
			m_upperOp == rhs.m_upperOp &&
			m_lowerVal == rhs.m_lowerVal &&
			m_upperVal == rhs.m_upperVal);
}

MojErr MojDbQuery::Page::fromObject(const MojObject& obj)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojString str;
	MojErr err = obj.stringValue(str);
	MojErrCheck(err);
	err = str.base64Decode(m_key.byteVec());
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::Page::toObject(MojObject& objOut) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojString str;
	MojErr err = str.base64Encode(m_key.byteVec(), false);
	MojErrCheck(err);
	objOut = str;

	return MojErrNone;
}

MojDbQuery::MojDbQuery()
{
	init();
}

MojDbQuery::~MojDbQuery()
{
}

MojErr MojDbQuery::toObject(MojObject& objOut) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObjectBuilder builder;
	MojErr err = toObject(builder);
	MojErrCheck(err);
	objOut = builder.object();
	return MojErrNone;
}

MojErr MojDbQuery::toObject(MojObjectVisitor& visitor) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = visitor.beginObject();
	MojErrCheck(err);

	if (!m_selectProps.empty()) {
		err = visitor.propName(SelectKey, MojStrLen(SelectKey));
		MojErrCheck(err);
		err = visitor.beginArray();
		MojErrCheck(err);
		for (StringSet::ConstIterator i = m_selectProps.begin(); i != m_selectProps.end(); i++) {
			err = visitor.stringValue(*i, i->length());
			MojErrCheck(err);
		}
		err = visitor.endArray();
		MojErrCheck(err);
	}

	err = visitor.stringProp(FromKey, m_fromType);
	MojErrCheck(err);

	if (!m_whereClauses.empty()) {
		err = appendClauses(visitor, WhereKey, m_whereClauses);
		MojErrCheck(err);
	}

	if (!m_filterClauses.empty()) {
		err = appendClauses(visitor, FilterKey, m_filterClauses);
		MojErrCheck(err);
	}

	if (!m_orderProp.empty()) {
		err = visitor.stringProp(OrderByKey, m_orderProp);
		MojErrCheck(err);
	}

	if (!m_distinct.empty()) {
		err = visitor.stringProp(DistinctKey, m_distinct);
		MojErrCheck(err);
	}

	if (m_desc) {
		err = visitor.boolProp(DescKey, true);
		MojErrCheck(err);
	}

	if (m_limit != LimitDefault) {
		err = visitor.intProp(LimitKey, m_limit);
		MojErrCheck(err);
	}

	if (!m_page.empty()) {
		MojObject obj;
		err = m_page.toObject(obj);
		MojErrCheck(err);
		err = visitor.objectProp(PageKey, obj);
		MojErrCheck(err);
	}

	err = visitor.endObject();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::fromObject(const MojObject& obj)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// TODO: validate against query schema

    bool orderBy = false;
    bool distinct = false;
    bool aggregate = false;
    bool immediateReturnVal = false;
	MojErr err;
	MojObject array;
	MojString str;

    // from
    err = obj.getRequired(FromKey, str);
    MojErrCheck(err);
    err = from(str);
    MojErrCheck(err);
	// distinct
	err = obj.get(DistinctKey, str, distinct);
	MojErrCheck(err);

    // aggregate
    MojObject aggreObj;
    aggregate = obj.get(AggregateKey, aggreObj);
    if (aggregate) {
        err = this->aggregate(aggreObj);
        MojErrCheck(err);
    }
    else if (distinct) {
		err = this->distinct(str);
		MojErrCheck(err);
		// if "distinct" is set, force "distinct" column into "select".
		err = select(str);
		MojErrCheck(err);
		// order
		err = order(str);
		MojErrCheck(err);
	} else {
		// select
		if (obj.get(SelectKey, array)) {
			if(array.empty()) {
				MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: select clause but no selected properties"));
			}
			MojObject prop;
			MojSize i = 0;
			while (array.at(i++, prop)) {
				MojErr err = prop.stringValue(str);
				MojErrCheck(err);
				err = select(str);
				MojErrCheck(err);
			}
		}
		// order
		err = obj.get(OrderByKey, str, orderBy);
		MojErrCheck(err);
		if (orderBy) {
			err = order(str);
			MojErrCheck(err);
		}
	}
	// where
	if (obj.get(WhereKey, array)) {
		err = addClauses(m_whereClauses, array);
		MojErrCheck(err);
	}
	// filter
	if (obj.get(FilterKey, array)) {
		err = addClauses(m_filterClauses, array);
		MojErrCheck(err);
	}
	// desc
	bool descVal;
	if (obj.get(DescKey, descVal)) {
		desc(descVal);
	}
	// limit
	MojInt64 lim;
    if (obj.get(ImmediateReturnKey, immediateReturnVal)) {
        if (orderBy || distinct || aggregate) {
            MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: immediateReturn property cannot use with orderBy, distinct and aggregate"));
        }
        immediateReturn(immediateReturnVal);
        // If immediateReturn is true, limit property is required.
        err = obj.getRequired(LimitKey, lim);
        MojErrCheck(err);
    } else {
	if (obj.get(LimitKey, lim)) {
		if (lim < 0)
			MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: negative query limit"));
	} else {
		lim = LimitDefault;
	}
    }

	limit((MojUInt32) lim);
	// page
	MojObject pageObj;
	if (obj.get(PageKey, pageObj)) {
		Page pagec;
		err = pagec.fromObject(pageObj);
		MojErrCheck(err);
		page(pagec);
	}
	bool incDel = false;
	if (obj.get(IncludeDeletedKey, incDel) && incDel) {
		err = includeDeleted();
		MojErrCheck(err);
	}
	return MojErrNone;
}

void MojDbQuery::clear()
{
    LOG_TRACE("Entering function %s", __FUNCTION__)

	init();
	m_fromType.clear();
	m_selectProps.clear();
	m_whereClauses.clear();
	m_filterClauses.clear();
	m_page.clear();
    m_aggregateMap.clear();
    m_groupByProps.clear();
}

MojErr MojDbQuery::select(const MojChar* propName)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojString str;
	MojErr err = str.assign(propName);
	MojErrCheck(err);
	err = m_selectProps.put(str);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::from(const MojChar* type)
{
	MojErr err = m_fromType.assign(type);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::where(const MojChar* propName, CompOp op, const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = addClause(m_whereClauses, propName, op, val, coll);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::filter(const MojChar* propName, CompOp op, const MojObject& val)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = addClause(m_filterClauses, propName, op, val, MojDbCollationInvalid);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::order(const MojChar* propName)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = m_orderProp.assign(propName);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::distinct(const MojChar* distinct)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = m_distinct.assign(distinct);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::aggregate(const MojObject& aggregate)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojObject array;
    if(aggregate.get(CountKey, array)) {
        err = addAggregate(array, OpCount);
        MojErrCheck(err);
    }
    if(aggregate.get(MinKey, array)) {
        err = addAggregate(array, OpMin);
        MojErrCheck(err);
    }
    if(aggregate.get(MaxKey, array)) {
        err = addAggregate(array, OpMax);
        MojErrCheck(err);
    }
    if(aggregate.get(SumKey, array)) {
        err = addAggregate(array, OpSum);
        MojErrCheck(err);
    }
    if(aggregate.get(AvgKey, array)) {
        err = addAggregate(array, OpAvg);
        MojErrCheck(err);
    }
    if(aggregate.get(FirstKey, array)) {
        err = addAggregate(array, OpFirst);
        MojErrCheck(err);
    }
    if(aggregate.get(LastKey, array)) {
        err = addAggregate(array, OpLast);
        MojErrCheck(err);
    }
    // groupBy property
    if(aggregate.get(GroupByKey, array)) {
        err = groupBy(array);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbQuery::getExtractor(const MojString& propName, MojRefCountedPtr<MojDbPropExtractor>& extractorOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojDbKind* kind = NULL;
    MojErr err = m_kindEngine->getKind(from().data(), kind);
    MojErrCheck(err);

    // Find collation for prop name
    MojDbCollationStrength collation = MojDbCollationInvalid;
    const MojDbKind::IndexVec& indexVec = kind->indexes();
    for (MojDbKind::IndexVec::ConstIterator i = indexVec.begin(); i != indexVec.end(); ++i) {
        MojDbIndex::StringVec propNames = (*i)->props();
        if (propNames.front()== propName.data()) {
            collation = (*i)->collation(0);
        }
    }

    // If collation is valid, create extractor to get sort key
    extractorOut.reset(new MojDbPropExtractor);
    MojAllocCheck(extractorOut.get());
    extractorOut->name(propName);
    err = extractorOut->prop(propName);
    MojErrCheck(err);
    if (collation != MojDbCollationInvalid) {
        MojRefCountedPtr<MojDbTextCollator> collator(new MojDbTextCollator);
        MojAllocCheck(collator.get());
        err = collator->init(m_locale, collation);
        MojErrCheck(err);
        extractorOut->collator(collator.get());
    }

    return MojErrNone;
}

MojErr MojDbQuery::addAggregate(const MojObject& propNames, const MojByte& op) {
	LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojSize i = 0;
    MojObject prop;
    MojString str;
    MojByte opOut = 0;
    while (propNames.at(i++, prop)) {
        err = prop.stringValue(str);
        MojErrCheck(err);
        AggregatePropInfo infoOut;
        bool found;
        found = m_aggregateMap.get(str, infoOut);
        if(found) infoOut.m_op |= op;
        else infoOut.m_op = op;
        // if min or max operation we need to set extractor
        if (!infoOut.m_extractor.get() && ((op & OpMin) || (op & OpMax) || (op & OpFirst) || (op & OpLast))) {
            err = getExtractor(str, infoOut.m_extractor);
            MojErrCheck(err);
        }
        err = m_aggregateMap.put(str, infoOut);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbQuery::groupBy(const MojObject& groupBy)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojObject prop;
    MojSize i = 0;
    MojString str;
    while (groupBy.at(i++, prop)) {
        MojErr err = prop.stringValue(str);
        MojErrCheck(err);
        err = m_groupByProps.put(str);
        MojErrCheck(err);
        break;
    }

    return MojErrNone;
}

MojErr MojDbQuery::includeDeleted(bool val)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (val) {
		MojObject array;
		MojErr err = array.push(false);
		MojErrCheck(err);
		err = array.push(true);
		MojErrCheck(err);
		// TODO: Move string constants to single location
		err = where(DelKey, MojDbQuery::OpEq, array);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbQuery::validate() const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	bool hasInequalityOp = false;
	bool hasArrayVal = false;
	for (WhereMap::ConstIterator i = m_whereClauses.begin(); i != m_whereClauses.end(); ++i) {
		// verify that we only have inequality op on one prop
		if (i->lowerOp() != OpEq) {
			if (hasInequalityOp)
				MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: query contains inequality operations on multiple properties"));
			hasInequalityOp = true;
		}
		// verify that we only have one array val and it's on an = or % op
		if (i->lowerVal().type() == MojObject::TypeArray && i.key() != DelKey) {
			if (hasArrayVal)
				MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: query contains array values on multiple properties"));
			hasArrayVal = true;
		}
	}
	for (WhereMap::ConstIterator i = m_filterClauses.begin(); i != m_filterClauses.end(); ++i) {
		if (i->lowerOp() == OpSearch) {
			MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: query contains search operator in filter"));
		}
	}
	return MojErrNone;
}

MojErr MojDbQuery::validateFind() const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	for (WhereMap::ConstIterator i = m_whereClauses.begin(); i != m_whereClauses.end(); ++i) {
		// if clause is order-defining, verify that it matches the order specified
		if (i->definesOrder() && !(m_orderProp.empty() || m_orderProp == i.key()))
			MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: query order not compatible with where clause"));
		// disallow search operator
		if (i->lowerOp() == OpSearch) {
			MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: search operator not allowed in find"));
		}
	}
	if (!m_filterClauses.empty()) {
		MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: filter not allowed in find"));
	}
	return MojErrNone;
}

bool MojDbQuery::operator==(const MojDbQuery& rhs) const
{
	if (m_fromType != rhs.m_fromType ||
		m_selectProps != rhs.m_selectProps ||
	    m_whereClauses != rhs.m_whereClauses ||
	    m_page.key() != rhs.m_page.key() ||
		m_orderProp != rhs.m_orderProp ||
		m_distinct != rhs.m_distinct ||
		m_limit != rhs.m_limit ||
		m_desc != rhs.m_desc) {
		return false;
	}
	return true;
}

void MojDbQuery::init()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    m_limit = LimitDefault;
    m_immediateReturn = false;
    m_desc = false;
    m_forceIndex = NULL;
    m_ignoreInactiveShards = true;
    m_kindEngine = NULL;
}

MojErr MojDbQuery::addClauses(WhereMap& map, const MojObject& array)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject clause;
	MojSize i = 0;
	while (array.at(i++, clause)) {
		MojString prop;
		MojErr err = clause.getRequired(PropKey, prop);
		MojErrCheck(err);
		MojString opStr;
		err = clause.getRequired(OpKey, opStr);
		MojErrCheck(err);
		CompOp op = OpNone;
		err = stringToOp(opStr, op);
		MojErrCheck(err);
		MojObject val;
		err = clause.getRequired(ValKey, val);
		MojErrCheck(err);
		MojDbCollationStrength coll = MojDbCollationInvalid;
		MojString collateStr;
		bool found = false;
		err = clause.get(CollateKey, collateStr, found);
		MojErrCheck(err);
		if (found) {
			err = MojDbUtils::collationFromString(collateStr, coll);
			MojErrCheck(err);
		}
		err = addClause(map, prop, op, val, coll);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbQuery::addClause(WhereMap& map, const MojChar* propName, CompOp op, const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(propName);

	// only allow valid ops
    if (!(op >= OpEq && op <= OpSubString))
		MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: invalid query op"));
    // only allow array values for = or % or %% ops
    if (val.type() == MojObject::TypeArray && op != OpEq && op != OpPrefix && op != OpSubString)
		MojErrThrowMsg(MojErrDbInvalidQuery, _T("db: query contains array value for non-eq op"));

	// check to see if the prop is referenced in a prior clause
	WhereMap::Iterator iter;
	MojErr err = map.find(propName, iter);
	MojErrCheck(err);
	if (iter == map.end()) {
		// create new clause
		err = createClause(map, propName, op, val, coll);
		MojErrCheck(err);
	} else {
		// add clause to previously referenced prop.
		err = updateClause(iter.value(), op, val, coll);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbQuery::createClause(WhereMap& map, const MojChar* propName, CompOp op, MojObject val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// construct the clause
	MojErr err = MojErrNone;
	WhereClause clause;
	switch (op) {
	case OpLessThan:
	case OpLessThanEq:
		// less-than ops define upper bound
		err = clause.setUpper(op, val, coll);
		MojErrCheck(err);
		break;
	default:
		// everything else is stored as lower bound
		err = clause.setLower(op, val, coll);
		MojErrCheck(err);
		break;
	}
	// add clause to map
	MojString str;
	err = str.assign(propName);
	MojErrCheck(err);
	err = map.put(str, clause);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::updateClause(WhereClause& clause, CompOp op, const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// the only case where we can have two ops for the same prop is a combination
	// of '<' or '<=' with '>' or '>='
	switch (op) {
	case OpLessThan:
	case OpLessThanEq: {
		CompOp lowerOp = clause.lowerOp();
		if (lowerOp != OpNone && lowerOp != OpGreaterThan && lowerOp != OpGreaterThanEq)
			MojErrThrow(MojErrDbInvalidQueryOpCombo);
		MojErr err = clause.setUpper(op, val, coll);
		MojErrCheck(err);
		break;
	}
	case OpGreaterThan:
	case OpGreaterThanEq: {
		CompOp upperOp = clause.upperOp();
		if (upperOp != OpNone && upperOp != OpLessThan && upperOp != OpLessThanEq)
			MojErrThrow(MojErrDbInvalidQueryOpCombo);
		MojErr err = clause.setLower(op, val, coll);
		MojErrCheck(err);
		break;
	}
	default:
		// all other ops invalid in combination
		MojErrThrow(MojErrDbInvalidQueryOpCombo);
	}
	return MojErrNone;
}

const MojDbQuery::StrOp MojDbQuery::s_ops[] = {
	{_T("="), OpEq},
	{_T("!="), OpNotEq},
	{_T("<"), OpLessThan},
	{_T("<="), OpLessThanEq},
	{_T(">"), OpGreaterThan},
	{_T(">="), OpGreaterThanEq},
	{_T("%"), OpPrefix},
	{_T("?"), OpSearch},
    {_T("%%"), OpSubString},
	{NULL, OpNone}
};

MojErr MojDbQuery::stringToOp(const MojChar* str, CompOp& opOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	for (const StrOp* op = s_ops; op->m_str != NULL; ++op) {
		if (MojStrCmp(op->m_str, str) == 0) {
			opOut = op->m_op;
			return MojErrNone;
		}
	}
	MojErrThrow(MojErrDbInvalidQueryOp);
}

const MojChar* MojDbQuery::opToString(CompOp op)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (op == OpNone || op > OpSubString)
		return NULL;
	return s_ops[op-1].m_str;
}

MojErr MojDbQuery::appendClauses(MojObjectVisitor& visitor, const MojChar* propName, const WhereMap& map)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = visitor.propName(propName);
	MojErrCheck(err);
	err = visitor.beginArray();
	MojErrCheck(err);
	for (WhereMap::ConstIterator i = map.begin(); i != map.end(); i++) {
		if (i->lowerOp() != OpNone) {
			err = appendClause(visitor, i.key(), i->lowerOp (), i->lowerVal(), i->collation());
			MojErrCheck(err);
		}
		if (i->upperOp() != OpNone) {
			err = appendClause(visitor, i.key(), i->upperOp (), i->upperVal(), i->collation());
			MojErrCheck(err);
		}
	}
	err = visitor.endArray();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbQuery::appendClause(MojObjectVisitor& visitor, const MojChar* propName, CompOp op,
 							   const MojObject& val, MojDbCollationStrength coll)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(propName);

	MojErr err = visitor.beginObject();
	MojErrCheck(err);

	err = visitor.stringProp(PropKey, propName);
	MojErrCheck(err);
	const MojChar* opStr = opToString(op);
	if (!opStr) {
		MojErrThrow(MojErrDbInvalidQueryOp);
	}
	err = visitor.stringProp(OpKey, opStr);
	MojErrCheck(err);
	err = visitor.objectProp(ValKey, val);
	MojErrCheck(err);
	if (coll != MojDbCollationInvalid) {
		err = visitor.stringProp(CollateKey, MojDbUtils::collationToString(coll));
		MojErrCheck(err);
	}
	err = visitor.endObject();
	MojErrCheck(err);

	return MojErrNone;
}
