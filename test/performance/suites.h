// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "db/MojDb.h"
#include "db/MojDbSearchCursor.h"
#include "dbhelpers.h"
#include "sample.h"

static const char* RegexpAllExceptNoindexdb = "^(?!noindexdb)([a-zA-Z0-9]+)$";
static const char* RegexpAllDatabases = "([a-zA-Z0-9]+)";
const char* RegexpOnlyAllIndexDb = "^(?=allindexdb)([a-zA-Z0-9]+)$";

enum class Position { First, Middle, Last };

MojErr objectByPosition(MojDb* db, Position position, MojObject *obj)
{
	MojErr err;

	switch (position) {
		case Position::First:
			err = getFirstLastObject(db, obj, true);
			break;
		case Position::Middle:
			err = getMiddleObject(db, obj);
			break;
		case Position::Last:
			err = getFirstLastObject(db, obj, false);
			break;
	}

	return MojErrNone;
}

class GetBase
{
public:
	GetBase(Position position) : m_position(position) {}

	MojErr init(MojDb* db)
	{
		MojErr err;

		MojObject obj;

		err = objectByPosition(db, m_position, &obj);
		MojErrCheck(err);

		err = obj.getRequired(MojDb::IdKey, m_getObjId);
		MojErrCheck(err);

		return MojErrNone;
	}

	MojErr run(MojDb* db) noexcept
	{
		bool found;
		MojErr err;
		err = db->get(m_getObjId, m_resultObj, found);
		MojErrCheck(err);

		assert(found);

		return MojErrNone;
	}

private:
	MojObject m_resultObj;
	MojObject m_getObjId;
	Position m_position;
};

struct GetBegin : public GetBase
{
    GetBegin() : GetBase(Position::First) {}
};

struct GetMiddle : public GetBase
{
	GetMiddle() : GetBase(Position::Middle) {}
};

struct GetEnd : public GetBase
{
	GetEnd() : GetBase(Position::Last) {}
};

class FindBase
{
public:
	FindBase(Position position) : m_position(position) {}

	MojErr init(MojDb* db)
	{
		MojErr err;

		MojObject obj;

		err = objectByPosition(db, m_position, &obj);
		MojErrCheck(err);

		MojUInt32 aValue;
		err = obj.getRequired(_T("a"), aValue);
		MojErrCheck(err);

		err = m_query.from(_T("com.foo.bar:1"));
		MojErrCheck(err);

		err = m_query.where(_T("a"), MojDbQuery::OpEq, aValue);
		MojErrCheck(err);

		m_query.limit(1);

		return MojErrNone;
	}

	MojErr run(MojDb* db) noexcept
	{
		bool found;
		MojErr err;

		MojDbCursor cursor;
		err = db->find(m_query, cursor);
		MojErrCheck(err);

		err = cursor.get(m_resultObj, found);
		MojErrCheck(err);

		assert(found);

		return MojErrNone;
	}

private:
	Position m_position;
	MojObject m_resultObj;
protected:
        MojDbQuery m_query;
};

struct FindBegin : public FindBase
{
    FindBegin() : FindBase(Position::First) {}
};

struct FindMiddle : public FindBase
{
	FindMiddle() : FindBase(Position::Middle) {}
};

struct FindEnd : public FindBase
{
	FindEnd() : FindBase(Position::Last) {}
};
class SearchBase
{
public:
        explicit SearchBase(Position position) : m_position(position) {}

			MojErr init(MojDb* db)
			{
			MojErr err;
			MojObject obj;
			err = objectByPosition(db, m_position, &obj);
			MojErrCheck(err);
			MojUInt32 aValue;
			err = obj.getRequired(_T("a"), aValue);
			MojErrCheck(err);
			err = m_query.from(_T("com.foo.bar:1"));
			MojErrCheck(err);
			err = m_query.where(_T("a"), MojDbQuery::OpLessThan, aValue);
			MojErrCheck(err);
			m_query.limit(1);
			return MojErrNone;
			}

        MojErr run(MojDb* db) noexcept
        {
                bool found;
                MojErr err;

                MojString str;
                MojDbSearchCursor m_cursor(str);
                err = db->find(m_query, m_cursor);
                MojErrCheck(err);

                err = m_cursor.get(m_resultObj, found);
                MojErrCheck(err);

                assert(found);

                return MojErrNone;
        }

private:
        Position m_position;
        MojDbStorageItem* m_resultObj;
protected:
        MojDbQuery m_query;
};
struct SearchBegin : public SearchBase
{
    SearchBegin() : SearchBase(Position::First) {}
};

struct SearchMiddle : public SearchBase
{
    SearchMiddle() : SearchBase(Position::Middle) {}
};

struct SearchEnd : public SearchBase
{
    SearchEnd() : SearchBase(Position::Last) {}
};

struct FindComplexEqualEqual : public FindBase
{
        FindComplexEqualEqual() : FindBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Middle, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpEq, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct FindComplexGreaterEqual : public FindBase
{
        FindComplexGreaterEqual() : FindBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Middle, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                err = objectByPosition(db, Position::Last, &obj);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpGreaterThan, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct FindComplexLessEqual : public FindBase
{
        FindComplexLessEqual() : FindBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Last, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                err = objectByPosition(db, Position::Middle, &obj);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpLessThan, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct SearchComplexEqualEqual : public SearchBase
{
        SearchComplexEqualEqual() : SearchBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Middle, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpEq, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct SearchComplexGreaterEqual : public SearchBase
{
        SearchComplexGreaterEqual() : SearchBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Middle, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                err = objectByPosition(db, Position::Last, &obj);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpGreaterThan, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct SearchComplexLessEqual : public SearchBase
{
        SearchComplexLessEqual() : SearchBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Last, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                err = objectByPosition(db, Position::Middle, &obj);

                MojUInt32 bValue;
                err = obj.getRequired(_T("b"), bValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpLessThan, aValue);
                MojErrCheck(err);

                err = m_query.where(_T("b"), MojDbQuery::OpEq, bValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

struct SearchComplexPrefix : public SearchBase
{
        SearchComplexPrefix() : SearchBase(Position::Middle) {}

        MojErr init(MojDb* db)
        {
                MojErr err;

                MojObject obj;

                err = objectByPosition(db, Position::Middle, &obj);
                MojErrCheck(err);

                MojUInt32 aValue;
                err = obj.getRequired(_T("a"), aValue);
                MojErrCheck(err);

                err = m_query.from(_T("com.foo.bar:1"));
                MojErrCheck(err);

                err = m_query.where(_T("a"), MojDbQuery::OpPrefix, aValue);
                MojErrCheck(err);

                m_query.limit(1);

                return MojErrNone;
        }
};

static const Suite Suites[]
{
	std::make_tuple("getBegin",  [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<GetBegin>    (db, threads, trepeats, diffs); }, RegexpAllDatabases),
	std::make_tuple("getMiddle", [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<GetMiddle>   (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
	std::make_tuple("getEnd",    [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<GetEnd>      (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
	std::make_tuple("findBegin", [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindBegin>   (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
	std::make_tuple("findMiddle",[](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindMiddle>  (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
   std::make_tuple("findEnd",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindEnd>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("findclientBegin",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindBegin>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("findclientMiddle",[](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindMiddle>  (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("findclientEnd",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindEnd>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchBegin", [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchBegin>   (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchMiddle",[](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchMiddle>  (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchEnd",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchEnd>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchclientBegin",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchBegin>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchclientMiddle",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchMiddle>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
    std::make_tuple("searchclientEnd",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchEnd>     (db, threads, trepeats, diffs); }, RegexpAllExceptNoindexdb),
	std::make_tuple("FindComplexEqualEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindComplexEqualEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("FindComplexGreaterEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindComplexGreaterEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("FindComplexLessEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<FindComplexLessEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("SearchComplexEqualEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchComplexEqualEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("SearchComplexGreaterEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchComplexGreaterEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("SearchComplexLessEqual",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchComplexLessEqual>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb),
        std::make_tuple("SearchComplexPrefix",   [](MojDb* db, size_t threads, size_t trepeats, Durations* diffs) { return sampleThreadedAccumulate<SearchComplexPrefix>     (db, threads, trepeats, diffs); }, RegexpOnlyAllIndexDb)
};
