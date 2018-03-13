// Copyright (c) 2009-2018 LG Electronics, Inc.
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


#ifndef MOJDBQUERY_H_
#define MOJDBQUERY_H_

#include "db/MojDbDefs.h"
#include "db/MojDbKey.h"
#include "core/MojHashMap.h"
#include "core/MojObject.h"
#include "core/MojSet.h"
#include "core/MojString.h"
#include "db/MojDbExtractor.h"

class MojDbQuery
{
public:
	static const MojUInt32 LimitDefault = MojUInt32Max;

	typedef enum {
		OpNone,
		OpEq,
		OpNotEq,
		OpLessThan,
		OpLessThanEq,
		OpGreaterThan,
		OpGreaterThanEq,
		OpPrefix,
        OpSearch,
        OpSubString
	} CompOp;

    enum AggregateOp {
        OpCount = (1 << 0),
        OpMin   = (1 << 1),
        OpMax   = (1 << 2),
        OpSum   = (1 << 3),
        OpAvg   = (1 << 4),
        OpFirst = (1 << 5),
        OpLast  = (1 << 6)
    };

	class WhereClause
	{
	public:
		WhereClause();

		MojErr setLower(CompOp op, const MojObject& val, MojDbCollationStrength coll);
		MojErr setUpper(CompOp op, const MojObject& val, MojDbCollationStrength coll);

		MojDbCollationStrength collation() const { return m_collation; }
		CompOp lowerOp() const { return m_lowerOp; }
		CompOp upperOp() const { return m_upperOp; }
		const MojObject& lowerVal() const { return m_lowerVal; }
		const MojObject& upperVal() const { return m_upperVal; }
		bool definesOrder() const { return m_lowerOp != OpEq || m_lowerVal.type() == MojObject::TypeArray; }
		bool operator==(const WhereClause& rhs) const;

	private:
		MojErr collation(MojDbCollationStrength collation);

		CompOp m_lowerOp;
		CompOp m_upperOp;
		MojObject m_lowerVal;
		MojObject m_upperVal;
		MojDbCollationStrength m_collation;
	};

	class Page
	{
	public:
		void clear() { m_key.clear(); }
		MojErr fromObject(const MojObject& obj);
		MojErr toObject(MojObject& objOut) const;
		bool empty() const { return m_key.empty(); }

		const MojDbKey& key() const { return m_key; }
		MojDbKey& key() { return m_key; }

	private:
		MojDbKey m_key;
	};

    struct AggregatePropInfo
    {
        bool operator<(const AggregatePropInfo& rhs) const { return m_op < rhs.m_op; }

        MojByte m_op;
        MojRefCountedPtr<MojDbPropExtractor> m_extractor;
    };

	typedef MojHashMap<MojString, WhereClause, const MojChar*> WhereMap;
	typedef MojSet<MojString> StringSet;
    typedef MojMap<MojString, AggregatePropInfo> AggregateMap;

	static const MojChar* const SelectKey;
	static const MojChar* const FromKey;
	static const MojChar* const WhereKey;
	static const MojChar* const FilterKey;
	static const MojChar* const OrderByKey;
	static const MojChar* const DistinctKey;
	static const MojChar* const DescKey;
	static const MojChar* const IncludeDeletedKey;
	static const MojChar* const LimitKey;
        static const MojChar* const ImmediateReturnKey;
	static const MojChar* const PageKey;
    static const MojChar* const AggregateKey;
    static const MojChar* const GroupByKey;
    static const MojChar* const CountKey;
    static const MojChar* const MinKey;
    static const MojChar* const MaxKey;
    static const MojChar* const SumKey;
    static const MojChar* const AvgKey;
    static const MojChar* const FirstKey;
    static const MojChar* const LastKey;
	static const MojChar* const PropKey;
	static const MojChar* const OpKey;
	static const MojChar* const ValKey;
	static const MojChar* const CollateKey;
	static const MojChar* const DelKey;
	static const MojUInt32 MaxQueryLimit = 500;

	MojDbQuery();
	~MojDbQuery();

	MojErr fromObject(const MojObject& obj);
	MojErr toObject(MojObject& objOut) const;
	MojErr toObject(MojObjectVisitor& visitor) const;

	void clear();
	MojErr select(const MojChar* propName);
	MojErr from(const MojChar* type);
	MojErr where(const MojChar* propName, CompOp op, const MojObject& val, MojDbCollationStrength coll = MojDbCollationInvalid);
	MojErr filter(const MojChar* propName, CompOp op, const MojObject& val);
	MojErr order(const MojChar* propName);
	MojErr distinct(const MojChar* distinct);
    MojErr aggregate(const MojObject& aggregate);
	MojErr includeDeleted(bool val = true);
	void desc(bool val) { m_desc = val; }
	void immediateReturn(bool val) { m_immediateReturn = val; }
    void setIgnoreInactiveShards(bool val = true) { m_ignoreInactiveShards = val; }
	void limit(MojUInt32 numResults) { m_limit = numResults; }
	void page(const Page& page) { m_page = page; }
    void kindEngine(MojDbKindEngine* kindEngine) { m_kindEngine = kindEngine; }
    void locale(MojString locale) { m_locale = locale; }

	MojErr validate() const;
	MojErr validateFind() const;
	const StringSet& select() const { return m_selectProps; }
	const MojString& from() const { return m_fromType; }
	const WhereMap& where() const { return m_whereClauses; }
	const WhereMap& filter() const { return m_filterClauses; }
	const MojString& order() const { return m_orderProp; }
	const MojString& distinct() const { return m_distinct; }
    const AggregateMap& aggregate() const { return m_aggregateMap; }
    const StringSet& groupBy() const { return m_groupByProps; }
	const Page& page() const { return m_page; }
	bool desc() const { return m_desc; }
	bool immediateReturn() { return m_immediateReturn; }
	MojUInt32 limit() const { return m_limit; }
    bool ignoreInactiveShards() const {return m_ignoreInactiveShards;}
	bool operator==(const MojDbQuery& rhs) const;

	static MojErr stringToOp(const MojChar* str, CompOp& opOut);

private:
	friend class MojDbQueryPlan;
	friend class MojDbKind;
	friend class MojDbIndex;

	struct StrOp
	{
		const MojChar* const m_str;
		const CompOp m_op;
	};
	static const StrOp s_ops[];

	void init();
	static MojErr addClauses(WhereMap& map, const MojObject& array);
	static MojErr addClause(WhereMap& map, const MojChar* propName, CompOp op, const MojObject& val, MojDbCollationStrength coll);
	static MojErr createClause(WhereMap& map, const MojChar* propName, CompOp op, MojObject val, MojDbCollationStrength coll);
	static MojErr updateClause(WhereClause& clause, CompOp op, const MojObject& val, MojDbCollationStrength coll);
	static const MojChar* opToString(CompOp op);
	static MojErr appendClauses(MojObjectVisitor& visitor, const MojChar* propName, const WhereMap& map);
	static MojErr appendClause(MojObjectVisitor& visitor, const MojChar* propName, CompOp op,
							  const MojObject& val, MojDbCollationStrength coll);
    MojErr addAggregate(const MojObject& propName, const MojByte& op);
    MojErr groupBy(const MojObject& groupBy);
    MojErr getExtractor(const MojString& propName, MojRefCountedPtr<MojDbPropExtractor>& extractorOut);

	MojString m_fromType;
	StringSet m_selectProps;
	WhereMap m_whereClauses;
	WhereMap m_filterClauses;
	Page m_page;
	MojString m_orderProp;
	MojString m_distinct;
	MojUInt32 m_limit;
        bool m_immediateReturn;
	bool m_desc;
    bool m_ignoreInactiveShards;
    AggregateMap m_aggregateMap;
    StringSet m_groupByProps;
    MojDbKindEngine* m_kindEngine;
    MojString m_locale;

	MojDbIndex *m_forceIndex;		// for debugging in stats
};

#endif /* MOJDBQUERY_H_ */
