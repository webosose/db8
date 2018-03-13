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


#include "MojDbAggregateTest.h"
#include "db/MojDb.h"
#include "db/MojDbCursor.h"
#include "core/MojObjectBuilder.h"

static const MojChar* const MojAggregateKindStr =
    _T("{\"id\":\"AggregationTest:1\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[")
        _T("{\"name\":\"idx3\",\"props\":[{\"name\":\"age\"}]}")
    _T("]}");

static const MojChar* const MojAggregateTestObjects[] = {
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"HR\"}, \"name\":{\"first\":\"kim\",\"last\":\"david\"}, \"age\":39, \"position\":3, \"salary\":500, \"penalty\":10, \"tall\":180}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"SWP\"}, \"name\":{\"first\":\"park\",\"last\":\"james\"}, \"age\":25, \"position\":4, \"salary\":310, \"tall\":167.8}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"SWP\"}, \"name\":{\"first\":\"min\",\"last\":\"daniel\"}, \"age\":45, \"position\":1, \"salary\":400, \"penalty\":0, \"tall\":175.0}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"HR\"}, \"name\":{\"first\":\"hwang\",\"last\":\"esther\"}, \"age\":23, \"position\":4, \"salary\":190, \"penalty\":25, \"tall\":172.3}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"SWP\"}, \"name\":{\"first\":\"lee\",\"last\":\"john\"}, \"age\":34, \"position\":2, \"salary\":600, \"penalty\":10, \"tall\":155.9}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"HR\"}, \"name\":{\"first\":\"kang\",\"last\":\"paul\"}, \"age\":29, \"position\":2, \"salary\":450, \"tall\":182.1}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"HR\"}, \"name\":{\"first\":\"hong\",\"last\":\"tomas\"}, \"age\":56, \"position\":1, \"salary\":720, \"penalty\":100, \"tall\":161.6}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"SWP\"}, \"name\":{\"first\":\"choi\",\"last\":\"issac\"}, \"age\":40, \"position\":3, \"salary\":530, \"penalty\":90, \"tall\":179.8}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"SWP\"}, \"name\":{\"first\":\"kim\",\"last\":\"mathew\"}, \"age\":37, \"position\":3, \"salary\":360, \"tall\":159.4}"),
    _T("{\"_kind\":\"AggregationTest:1\", \"department\":{\"head\":\"CTO\",\"branch\":\"HR\"}, \"name\":{\"first\":\"lee\",\"last\":\"luke\"}, \"age\":47, \"position\":1, \"salary\":580, \"penalty\":0, \"tall\":165}")
};

static const MojChar* const MojAggregateQuery1Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"penalty\", \"age\"],")
            _T("\"min\":[\"age\", \"salary\", \"penalty\", \"tall\"],")
            _T("\"max\":[\"age\", \"salary\", \"penalty\", \"tall\"],")
            _T("\"first\":[\"age\", \"salary\", \"penalty\", \"tall\"],")
            _T("\"last\":[\"age\", \"salary\", \"penalty\", \"tall\"],")
            _T("\"sum\":[\"salary\", \"penalty\"],")
            _T("\"avg\":[\"salary\", \"tall\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateExpected1Str =
    _T("[")
        _T("{")
            _T("\"age\": {")
                _T("\"cnt\": 10,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 23,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hwang\",")
                        _T("\"last\": \"esther\"")
                    _T("},")
                    _T("\"penalty\": 25,")
                    _T("\"position\": 4,")
                    _T("\"salary\": 190,")
                    _T("\"tall\": 172.300000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 56,")
                _T("\"min\": 23")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 7,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 45,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"min\",")
                        _T("\"last\": \"daniel\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 400,")
                    _T("\"tall\": 175.000000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 100,")
                _T("\"min\": 0,")
                _T("\"sum\": 235")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 464.000000,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 23,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hwang\",")
                        _T("\"last\": \"esther\"")
                    _T("},")
                    _T("\"penalty\": 25,")
                    _T("\"position\": 4,")
                    _T("\"salary\": 190,")
                    _T("\"tall\": 172.300000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 720,")
                _T("\"min\": 190,")
                _T("\"sum\": 4640")
            _T("},")
            _T("\"tall\": {")
                _T("\"avg\": 169.890000,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 34,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"lee\",")
                        _T("\"last\": \"john\"")
                    _T("},")
                    _T("\"penalty\": 10,")
                    _T("\"position\": 2,")
                    _T("\"salary\": 600,")
                    _T("\"tall\": 155.900000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 39,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"kim\",")
                        _T("\"last\": \"david\"")
                    _T("},")
                    _T("\"penalty\": 10,")
                    _T("\"position\": 3,")
                    _T("\"salary\": 500,")
                    _T("\"tall\": 180")
                _T("},")
                _T("\"max\": 180,")
                _T("\"min\": 155.900000")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery2Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"department\"],")
            _T("\"min\":[\"name.first\"],")
            _T("\"max\":[\"name.first\"],")
            _T("\"first\":[\"name.first\"],")
            _T("\"last\":[\"name.first\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateExpected2Str =
    _T("[")
        _T("{")
            _T("\"department\": {")
                _T("\"cnt\": 10")
            _T("},")
            _T("\"name.first\": {")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 40,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"choi\",")
                        _T("\"last\": \"issac\"")
                    _T("},")
                    _T("\"penalty\": 90,")
                    _T("\"position\": 3,")
                    _T("\"salary\": 530,")
                    _T("\"tall\": 179.800000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 25,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"park\",")
                        _T("\"last\": \"james\"")
                    _T("},")
                    _T("\"position\": 4,")
                    _T("\"salary\": 310,")
                    _T("\"tall\": 167.800000")
                _T("},")
                _T("\"max\": \"park\",")
                _T("\"min\": \"choi\"")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery3Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"min\":[\"penalty\"],")
            _T("\"max\":[\"penalty\"],")
            _T("\"first\":[\"penalty\"],")
            _T("\"last\":[\"penalty\"],")
            _T("\"avg\":[\"salary\"],")
            _T("\"groupBy\":[\"department\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateExpected3Str =
    _T("[")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department\": {")
                    _T("\"branch\": \"HR\",")
                    _T("\"head\": \"CTO\"")
                _T("}")
            _T("},")
            _T("\"penalty\": {")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 47,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"lee\",")
                        _T("\"last\": \"luke\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 580,")
                    _T("\"tall\": 165")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 100,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 488.000000")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department\": {")
                    _T("\"branch\": \"SWP\",")
                    _T("\"head\": \"CTO\"")
                _T("}")
            _T("},")
            _T("\"penalty\": {")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 45,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"min\",")
                        _T("\"last\": \"daniel\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 400,")
                    _T("\"tall\": 175.000000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 40,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"choi\",")
                        _T("\"last\": \"issac\"")
                    _T("},")
                    _T("\"penalty\": 90,")
                    _T("\"position\": 3,")
                    _T("\"salary\": 530,")
                    _T("\"tall\": 179.800000")
                _T("},")
                _T("\"max\": 90,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 440.000000")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery4Str =
    _T("{")
            _T("\"aggregate\":{")
            _T("\"min\":[\"penalty\"],")
            _T("\"max\":[\"penalty\"],")
            _T("\"first\":[\"penalty\"],")
            _T("\"last\":[\"penalty\"],")
            _T("\"avg\":[\"salary\"],")
            _T("\"groupBy\":[\"department.branch\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateExpected4Str =
    _T("[")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department.branch\": \"HR\"")
            _T("},")
            _T("\"penalty\": {")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 47,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"lee\",")
                        _T("\"last\": \"luke\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 580,")
                    _T("\"tall\": 165")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 100,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 488.000000")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department.branch\": \"SWP\"")
            _T("},")
            _T("\"penalty\": {")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 45,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"min\",")
                        _T("\"last\": \"daniel\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 400,")
                    _T("\"tall\": 175.000000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 40,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"choi\",")
                        _T("\"last\": \"issac\"")
                    _T("},")
                    _T("\"penalty\": 90,")
                    _T("\"position\": 3,")
                    _T("\"salary\": 530,")
                    _T("\"tall\": 179.800000")
                _T("},")
                _T("\"max\": 90,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 440.000000")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery5Str =
    _T("{")
            _T("\"aggregate\":{")
            _T("\"cnt\":[\"penalty\"],")
            _T("\"avg\":[\"salary\"],")
            _T("\"groupBy\":[\"position\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateExpected5Str =
    _T("[")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"position\": 1")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 3")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 566.666667")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"position\": 2")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 1")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 525.000000")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"position\": 3")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 2")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 463.333333")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"position\": 4")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 1")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 250.000000")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery6Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"salary\", \"penalty\"],")
            _T("\"min\":[\"penalty\"],")
            _T("\"max\":[\"penalty\"],")
            _T("\"first\":[\"penalty\"],")
            _T("\"last\":[\"penalty\"],")
            _T("\"sum\":[\"salary\"],")
            _T("\"avg\":[\"salary\"],")
            _T("\"groupBy\":[\"department.branch\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\",")
        _T("\"where\":[{\"prop\":\"age\",\"op\":\">\",\"val\":30}]")
    _T("}");

static const MojChar* const MojAggregateExpected6Str =
    _T("[")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department.branch\": \"HR\"")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 3,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 47,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"lee\",")
                        _T("\"last\": \"luke\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 580,")
                    _T("\"tall\": 165")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 56,")
                    _T("\"department\": {")
                        _T("\"branch\": \"HR\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"hong\",")
                        _T("\"last\": \"tomas\"")
                    _T("},")
                    _T("\"penalty\": 100,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 720,")
                    _T("\"tall\": 161.600000")
                _T("},")
                _T("\"max\": 100,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 600.000000,")
                _T("\"cnt\": 3,")
                _T("\"sum\": 1800")
            _T("}")
        _T("},")
        _T("{")
            _T("\"groupBy\": {")
                _T("\"department.branch\": \"SWP\"")
            _T("},")
            _T("\"penalty\": {")
                _T("\"cnt\": 3,")
                _T("\"first\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 45,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"min\",")
                        _T("\"last\": \"daniel\"")
                    _T("},")
                    _T("\"penalty\": 0,")
                    _T("\"position\": 1,")
                    _T("\"salary\": 400,")
                    _T("\"tall\": 175.000000")
                _T("},")
                _T("\"last\": {")
                    _T("\"_kind\": \"AggregationTest:1\",")
                    _T("\"age\": 40,")
                    _T("\"department\": {")
                        _T("\"branch\": \"SWP\",")
                        _T("\"head\": \"CTO\"")
                    _T("},")
                    _T("\"name\": {")
                        _T("\"first\": \"choi\",")
                        _T("\"last\": \"issac\"")
                    _T("},")
                    _T("\"penalty\": 90,")
                    _T("\"position\": 3,")
                    _T("\"salary\": 530,")
                    _T("\"tall\": 179.800000")
                _T("},")
                _T("\"max\": 90,")
                _T("\"min\": 0")
            _T("},")
            _T("\"salary\": {")
                _T("\"avg\": 472.500000,")
                _T("\"cnt\": 4,")
                _T("\"sum\": 1890")
            _T("}")
        _T("}")
    _T("]");

static const MojChar* const MojAggregateQuery7Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"sum\":[\"department.branch\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateQuery8Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"avg\":[\"name\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:1\"")
    _T("}");

static const MojChar* const MojAggregateKindStr2 =
    _T("{\"id\":\"AggregationTest:2\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[")
        _T("{\"name\":\"group\",\"props\":[{\"name\":\"favoriteGroup.user1\"}]}")
    _T("]}");

static const MojChar* const MojAggregateTestObjects2[] = {
    _T("{\"_kind\":\"AggregationTest:2\", \"channel\":11, \"channelName\":\"MBC\", \"favoriteGroup\":{\"user1\":[\"A\"]}}"),
    _T("{\"_kind\":\"AggregationTest:2\", \"channel\":9, \"channelName\":\"KBS\", \"favoriteGroup\":{\"user1\":[\"A\",\"B\"]}}"),
    _T("{\"_kind\":\"AggregationTest:2\", \"channel\":5, \"channelName\":\"SBS\", \"favoriteGroup\":{\"user1\":[\"B\",\"C\"]}}"),
    _T("{\"_kind\":\"AggregationTest:2\", \"channel\":13, \"channelName\":\"EBS\", \"favoriteGroup\":{\"user1\":[\"C\"]}}")
};

static const MojChar* const MojAggregateQuery9Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"favoriteGroup.user1\"],")
            _T("\"min\":[\"channelName\"],")
            _T("\"max\":[\"channelName\"],")
            _T("\"sum\":[\"channel\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:2\"")
    _T("}");

static const MojChar* const MojAggregateExpected9Str =
    _T("[")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 38")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"SBS\",")
                _T("\"min\": \"EBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 4")
            _T("}")
        _T("}")
    _T("]");


static const MojChar* const MojAggregateQuery10Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"favoriteGroup.user1\"],")
            _T("\"min\":[\"channelName\"],")
            _T("\"max\":[\"channelName\"],")
            _T("\"sum\":[\"channel\"]")
        _T("},")
        _T("\"where\":[{\"prop\":\"favoriteGroup.user1\", \"op\":\"=\", \"val\":[\"A\"]} ],")
        _T("\"from\":\"AggregationTest:2\"")
    _T("}");

static const MojChar* const MojAggregateExpected10Str =
    _T("[")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 20")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"MBC\",")
                _T("\"min\": \"KBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 2")
            _T("}")
        _T("}")
    _T("]");


static const MojChar* const MojAggregateQuery11Str =
    _T("{")
        _T("\"aggregate\":{")
            _T("\"cnt\":[\"favoriteGroup.user1\"],")
            _T("\"min\":[\"channelName\"],")
            _T("\"max\":[\"channelName\"],")
            _T("\"sum\":[\"channel\"],")
            _T("\"groupBy\":[\"favoriteGroup.user1\"]")
        _T("},")
        _T("\"from\":\"AggregationTest:2\"")
    _T("}");

static const MojChar* const MojAggregateExpected11Str =
    _T("[")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 20")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"MBC\",")
                _T("\"min\": \"KBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 2")
            _T("},")
            _T("\"groupBy\": {")
                _T("\"favoriteGroup.user1\": \"A\"")
            _T("}")
        _T("},")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 14")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"SBS\",")
                _T("\"min\": \"KBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 2")
            _T("},")
            _T("\"groupBy\": {")
                _T("\"favoriteGroup.user1\": \"B\"")
            _T("}")
        _T("},")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 18")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"SBS\",")
                _T("\"min\": \"EBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 2")
            _T("},")
            _T("\"groupBy\": {")
                _T("\"favoriteGroup.user1\": \"C\"")
            _T("}")
        _T("}")
    _T("]");


static const MojChar* const MojAggregateQuery12Str =
    _T("{")
        _T("\"aggregate\":{")
           _T("\"cnt\":[\"favoriteGroup.user1\"],")
           _T("\"min\":[\"channelName\"],")
           _T("\"max\":[\"channelName\"],")
           _T("\"sum\":[\"channel\"],")
           _T("\"groupBy\":[\"favoriteGroup.user1\"]")
        _T("},")
        _T("\"where\":[{\"prop\":\"favoriteGroup.user1\", \"op\":\"=\", \"val\":[\"A\"]} ],")
        _T("\"from\":\"AggregationTest:2\"")
    _T("}");

static const MojChar* const MojAggregateExpected12Str =
    _T("[")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 20")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"MBC\",")
                _T("\"min\": \"KBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 2")
            _T("},")
            _T("\"groupBy\": {")
                _T("\"favoriteGroup.user1\": \"A\"")
            _T("}")
        _T("},")
        _T("{")
            _T("\"channel\": {")
                _T("\"sum\": 9")
            _T("},")
            _T("\"channelName\": {")
                _T("\"max\": \"KBS\",")
                _T("\"min\": \"KBS\"")
            _T("},")
            _T("\"favoriteGroup.user1\": {")
                _T("\"cnt\": 1")
            _T("},")
            _T("\"groupBy\": {")
                _T("\"favoriteGroup.user1\": \"B\"")
            _T("}")
        _T("}")
    _T("]");

MojDbAggregateTest::MojDbAggregateTest()
: MojTestCase(_T("MojDbAggregate"))
{
}

MojErr MojDbAggregateTest::run()
{
    MojDb db;
    MojErr err = db.open(MojDbTestDir);
    MojTestErrCheck(err);

    // add kind
    MojObject kindObj;
    err = kindObj.fromJson(MojAggregateKindStr);
    MojTestErrCheck(err);
    err = db.putKind(kindObj);
    MojTestErrCheck(err);

    err = kindObj.fromJson(MojAggregateKindStr2);
    MojTestErrCheck(err);
    err = db.putKind(kindObj);
    MojTestErrCheck(err);

    // put test objects
    for (MojSize i = 0; i < sizeof(MojAggregateTestObjects) / sizeof(MojChar*); ++i) {
        MojObject obj;
        err = obj.fromJson(MojAggregateTestObjects[i]);
        MojTestErrCheck(err);
        err = db.put(obj);
        MojTestErrCheck(err);
    }
    for (MojSize i = 0; i < sizeof(MojAggregateTestObjects2) / sizeof(MojChar*); ++i) {
        MojObject obj;
        err = obj.fromJson(MojAggregateTestObjects2[i]);
        MojTestErrCheck(err);
        err = db.put(obj);
        MojTestErrCheck(err);
    }

    err = test(db);
    MojTestErrCheck(err);

    err = db.close();
    MojTestErrCheck(err);

    return MojErrNone;
}

void MojDbAggregateTest::cleanup()
{
    (void) MojRmDirRecursive(MojDbTestDir);
}

MojErr MojDbAggregateTest::test(MojDb& db)
{
    // test1 : basic aggregation (cnt, min, max, sum, avg)
    MojErr err = check(db, MojAggregateQuery1Str, MojAggregateExpected1Str);
    MojTestErrCheck(err);

    // test2 : cnt, min and max for object and string type
    err = check(db, MojAggregateQuery2Str, MojAggregateExpected2Str);
    MojTestErrCheck(err);

    // test3 : groupBy of object type
    err = check(db, MojAggregateQuery3Str, MojAggregateExpected3Str);
    MojTestErrCheck(err);

    // test4 : groupBy of string type
    err = check(db, MojAggregateQuery4Str, MojAggregateExpected4Str);
    MojTestErrCheck(err);

    // test5 : groupBy of number type
    err = check(db, MojAggregateQuery5Str, MojAggregateExpected5Str);
    MojTestErrCheck(err);

    // test6 : aggregate + groupBy + find condition
    err = check(db, MojAggregateQuery6Str, MojAggregateExpected6Str);
    MojTestErrCheck(err);

    // test7 : invalid aggregate type ofsum
    err = checkInvalid(db, MojAggregateQuery7Str, MojErrDbInvalidAggregateType);
    MojTestErrCheck(err);

    // test8 : invalid aggregate type of avg
    err = checkInvalid(db, MojAggregateQuery8Str, MojErrDbInvalidAggregateType);
    MojTestErrCheck(err);

    // test9 : basic aggregate for array type
    err = check(db, MojAggregateQuery9Str, MojAggregateExpected9Str);
    MojTestErrCheck(err);

    // test10 : basic aggregate for array type with where clause
    err = check(db, MojAggregateQuery10Str, MojAggregateExpected10Str);
    MojTestErrCheck(err);

    // test11 : groupBy aggregate for array type
    err = check(db, MojAggregateQuery11Str, MojAggregateExpected11Str);
    MojTestErrCheck(err);

    // test12 : groupBy aggregate for array type with where clause
    err = check(db, MojAggregateQuery12Str, MojAggregateExpected12Str);
    MojTestErrCheck(err);

    return MojErrNone;
}

MojErr MojDbAggregateTest::check(MojDb& db, const MojChar* queryStr, const MojChar* expectedStr)
{
    MojObject queryObj;
    queryObj.fromJson(queryStr);

    MojString localeStr;
    MojDbReq req;
    MojErr err = db.getLocale(localeStr, req);
    MojErrCheck(err);

    MojDbQuery query;
    query.kindEngine(db.kindEngine());
    query.locale(localeStr);
    err = query.fromObject(queryObj);
    MojTestErrCheck(err);
    MojDbCursor cursor;
    err = db.find(query, cursor);
    MojTestErrCheck(err);

    MojObjectBuilder builder;
    err = builder.beginArray();
    MojTestErrCheck(err);
    err = cursor.visit(builder);
    MojTestErrCheck(err);
    err = cursor.close();
    MojTestErrCheck(err);
    err = builder.endArray();
    MojTestErrCheck(err);
    MojObject results = builder.object();

    MojObject expectedObj;
    err = expectedObj.fromJson(expectedStr);
    MojTestErrCheck(err);

    MojTestAssert(expectedObj == results);

    return MojErrNone;
}

MojErr MojDbAggregateTest::checkInvalid(MojDb& db, const MojChar* queryStr, const MojErr expectedErr)
{
   MojObject queryObj;
    queryObj.fromJson(queryStr);
    MojDbQuery query;
    MojErr err = query.fromObject(queryObj);
    MojTestErrCheck(err);
    MojTestErrCheck(err);
    MojDbCursor cursor;
    err = db.find(query, cursor);
    MojTestErrCheck(err);

    MojObjectBuilder builder;
    err = cursor.visit(builder);
    MojTestAssert(expectedErr == err);
    err = cursor.close();
    MojTestErrCheck(err);

    return MojErrNone;
}

