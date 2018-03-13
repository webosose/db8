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


#include "MojDbLocaleTest.h"
#include "db/MojDb.h"
#include "db/MojDbStorageEngine.h"
#include "MojDbTestStorageEngine.h"
#include "db/MojDbServiceDefs.h"

static const MojChar* const MojTestKindStrName = _T("Test:1");
static const MojChar* const MojTestKindStr =
	_T("{\"id\":\"Test:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"collate\":\"secondary\"}]}]}");

static const MojChar* MojTestObjects[] = {
	_T("{\"_id\":1,\"_kind\":\"Test:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":2,\"_kind\":\"Test:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":3,\"_kind\":\"Test:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":4,\"_kind\":\"Test:1\",\"foo\":\"côté\"}"),
	NULL
};

// test for English (en_US / en_GB / en_CA / etc..)
static const MojChar* const MojTestKindStrNameEN = _T("com.webos.test.en:1");
static const MojChar* const MojTestKindStrEN =
	_T("{\"id\":\"com.webos.test.en:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\", \"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsEN[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.en:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

// test for German (de_DE)
static const MojChar* const MojTestKindStrNameDE = _T("com.webos.test.de:1");
static const MojChar* const MojTestKindStrDE =
	_T("{\"id\":\"com.webos.test.de:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsDE[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"Fuße\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"Fluße\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"Flusse\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"flusse\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"fluße\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"flüße\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.de:1\",\"foo\":\"flüsse\"}"),
	NULL
};

// test for Spanish (es_ES / es_PE / etc..)
static const MojChar* const MojTestKindStrNameES = _T("com.webos.test.es:1");
static const MojChar* const MojTestKindStrES =
	_T("{\"id\":\"com.webos.test.es:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsES[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"enero\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"febrero\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"marzo\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"côté\"}"),
	_T("{\"_id\":13, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":14, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":15, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":16, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":17, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":18, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":19, \"_kind\":\"com.webos.test.es:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

// test for Hebrew (he_IL)
static const MojChar* const MojTestKindStrNameHE = _T("com.webos.test.he:1");
static const MojChar* const MojTestKindStrHE =
	_T("{\"id\":\"com.webos.test.he:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsHE[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"יום ראשון\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"יום שני\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"יום שלישי\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"ינואר\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"פברואר\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"מרץ\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":13, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":14, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":15, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"côté\"}"),
	_T("{\"_id\":16, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":17, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":18, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":19, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":20, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":21, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":22, \"_kind\":\"com.webos.test.he:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

// test for Japanese (ja_JP)
static const MojChar* const MojTestKindStrNameJA = _T("com.webos.test.ja:1");
static const MojChar* const MojTestKindStrJA =
	_T("{\"id\":\"com.webos.test.ja:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsJA[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"日曜日\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"月曜日\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"火曜日\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"1月\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"2月\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"3月\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":13, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":14, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":15, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"côté\"}"),
	_T("{\"_id\":16, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":17, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":18, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":19, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":20, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":21, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":22, \"_kind\":\"com.webos.test.ja:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

// test for Macedonian (mk_MK)
static const MojChar* const MojTestKindStrNameMK = _T("com.webos.test.mk:1");
static const MojChar* const MojTestKindStrMK =
	_T("{\"id\":\"com.webos.test.mk:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsMK[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"недела\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"понеделник\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"вторник\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"јануари\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"февруари\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"март\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":13, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":14, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":15, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"côté\"}"),
	_T("{\"_id\":16, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":17, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":18, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":19, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":20, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":21, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":22, \"_kind\":\"com.webos.test.mk:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

// test for Portuguese (pt_BR)
static const MojChar* const MojTestKindStrNamePT = _T("com.webos.test.pt:1");
static const MojChar* const MojTestKindStrPT =
	_T("{\"id\":\"com.webos.test.pt:1\",")
	_T("\"owner\":\"mojodb.admin\",")
	_T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\", \"collate\":\"tertiary\"}]}]}");

static const MojChar* MojTestObjectsPT[] = {
	_T("{\"_id\":1, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"domingo\"}"),
	_T("{\"_id\":2, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"segunda\"}"),
	_T("{\"_id\":3, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"terça\"}"),
	_T("{\"_id\":4, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"janeiro\"}"),
	_T("{\"_id\":5, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"fevereiro\"}"),
	_T("{\"_id\":6, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"março\"}"),
	_T("{\"_id\":7, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"bad\"}"),
	_T("{\"_id\":8, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"Bad\"}"),
	_T("{\"_id\":9, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"Bat\"}"),
	_T("{\"_id\":10, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"bat\"}"),
	_T("{\"_id\":11, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"bäd\"}"),
	_T("{\"_id\":12, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"Bäd\"}"),
	_T("{\"_id\":13, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"bät\"}"),
	_T("{\"_id\":14, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"Bät\"}"),
	_T("{\"_id\":15, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"côté\"}"),
	_T("{\"_id\":16, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"coté\"}"),
	_T("{\"_id\":17, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"côte\"}"),
	_T("{\"_id\":18, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"cote\"}"),
	_T("{\"_id\":19, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"black-bird\"}"),
	_T("{\"_id\":20, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"blackbird\"}"),
	_T("{\"_id\":21, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"black-birds\"}"),
	_T("{\"_id\":22, \"_kind\":\"com.webos.test.pt:1\",\"foo\":\"blackbirds\"}"),
	NULL
};

MojDbLocaleTest::MojDbLocaleTest()
: MojTestCase(_T("MojDbLocale"))
{
}

MojErr MojDbLocaleTest::run()
{
	MojDb db;
	MojErr err = db.open(MojDbTestDir);
	MojTestErrCheck(err);

	// test locales
	err = ICULocaleTestRun(db);
	MojTestErrCheck(err);

	// put kind
	MojObject obj;
	err = obj.fromJson(MojTestKindStr);
	MojTestErrCheck(err);
	err = db.putKind(obj);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("fr_CA"));
	MojTestErrCheck(err);
	err = put(db, MojTestKindStrName, MojTestObjects);
	MojErrCheck(err);

    err = db.updateLocale(_T("en_US"));
    MojTestErrCheck(err);
    err = checkOrder(db, MojTestKindStrName,_T("[1,2,3,4]"));
    MojTestErrCheck(err);

    /*
     * According to: http://userguide.icu-project.org/collation
     *   French requires that letters sorted with accents at the end of the string
     *   be sorted ahead of accents in the beginning of the string. For example,
     *   the word "côte" sorts before "coté" because the acute accent on the final
     *   "e" is more significant than the circumflex on the "o".
     *
     * Note that icu shipped with Ubuntu 12.04 and one from Yocto 1.4 doesn't
     * follow this requirements. But rather follows other what said in other
     * document: http://userguide.icu-project.org/collation/concepts#TOC-Collator-naming-scheme
     *   "Some French dictionary ordering traditions sort strings with
     *    different accents from the back of the string. This attribute is
     *    automatically set to On for the Canadian French locale (fr_CA)."
     */
    err = db.updateLocale(_T("fr_CA"));
    MojTestErrCheck(err);
    err = checkOrder(db, MojTestKindStrName, _T("[1,3,2,4]"));
    MojTestErrCheck(err);

	// close and reopen with test engine
	err = db.close();
	MojTestErrCheck(err);
	MojRefCountedPtr<MojDbStorageEngine> engine;
	err = MojDbStorageEngine::createDefaultEngine(engine);
	MojTestErrCheck(err);
	MojAllocCheck(engine.get());
	MojRefCountedPtr<MojDbTestStorageEngine> testEngine(new MojDbTestStorageEngine(engine.get()));
	MojAllocCheck(testEngine.get());
	err = testEngine->open(MojDbTestDir);
	MojTestErrCheck(err);
	err = db.open(MojDbTestDir, testEngine.get());
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrName, MojTestObjects);
	MojTestErrCheck(err);
	err = checkOrder(db, MojTestKindStrName, _T("[1,3,2,4]"));
	MojTestErrCheck(err);

	// fail txn commit
	err = testEngine->setNextError(_T("txn.commit"), MojErrDbDeadlock);
	MojTestErrCheck(err);
	err = db.updateLocale(_T("en_US"));
	MojTestErrExpected(err, MojErrDbDeadlock);
	// make sure we still have fr ordering
	err = checkOrder(db, MojTestKindStrName, _T("[1,3,2,4]"));
	MojTestErrCheck(err);
	err = put(db, MojTestKindStrName, MojTestObjects);
	MojTestErrCheck(err);
	err = checkOrder(db, MojTestKindStrName, _T("[1,3,2,4]"));
	MojTestErrCheck(err);

	err = db.close();
	MojTestErrCheck(err);

	return MojErrNone;
}

// Test set and sorted result are refer from http://demo.icu-project.org/icu-bin/locexp?d_=en&x=col&_=root
// It provide sample source and sorted result under each locales respectively
MojErr MojDbLocaleTest::ICULocaleTestRun(MojDb& db)
{
	// change locale
	MojErr err = db.updateLocale(_T("en_US"));
	MojTestErrCheck(err);

	// put kind for en_* test
	MojObject enKind;
	err = enKind.fromJson(MojTestKindStrEN);
	MojTestErrCheck(err);
	err = db.putKind(enKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameEN, MojTestObjectsEN);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameEN, _T("[1,2,5,6,4,3,7,8,9,11,10,12]"));
	MojTestErrCheck(err);

	// delete kind to prevent duplicated id
	bool found;
	MojObject delKindId;
	err = enKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);


	err = db.updateLocale(_T("de_DE"));
	MojTestErrCheck(err);

	// put Kind for de_DE test
	MojObject deKind;
	err = deKind.fromJson(MojTestKindStrDE);
	MojTestErrCheck(err);
	err = db.putKind(deKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameDE, MojTestObjectsDE);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameDE, _T("[4,3,5,2,7,6,1]"));
	MojTestErrCheck(err);

	err = deKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("es_AR"));
	MojTestErrCheck(err);

	// put Kind for es_* test
	MojObject esKind;
	err = esKind.fromJson(MojTestKindStrES);
	MojTestErrCheck(err);
	err = db.putKind(esKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameES, MojTestObjectsES);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameES, _T("[4,5,8,9,7,6,10,11,16,18,17,19,15,13,14,12,1,2,3]"));
	MojTestErrCheck(err);

	err = esKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("he_IL"));
	MojTestErrCheck(err);

	// put Kind for he_IL test
	MojObject heKind;
	err = heKind.fromJson(MojTestKindStrHE);
	MojTestErrCheck(err);
	err = db.putKind(heKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameHE, MojTestObjectsHE);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameHE, _T("[1,2,3,4,6,5,7,8,11,12,10,9,13,14,19,21,20,22,18,16,17,15]"));
	MojTestErrCheck(err);

	err = heKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("ja_JP"));
	MojTestErrCheck(err);

	// put Kind for ja_JP test
	MojObject jaKind;
	err = jaKind.fromJson(MojTestKindStrJA);
	MojTestErrCheck(err);
	err = db.putKind(jaKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameJA, MojTestObjectsJA);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameJA, _T("[4,5,6,7,8,11,12,10,9,13,14,19,21,20,22,18,16,17,15,3,2,1]"));
	MojTestErrCheck(err);

	err = jaKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("mk_MK"));
	MojTestErrCheck(err);

	// put Kind for mk_MK test
	MojObject mkKind;
	err = mkKind.fromJson(MojTestKindStrMK);
	MojTestErrCheck(err);
	err = db.putKind(mkKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNameMK, MojTestObjectsMK);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNameMK, _T("[3,4,6,1,2,5,7,8,11,12,10,9,13,14,19,21,20,22,18,16,17,15]"));
	MojTestErrCheck(err);

	err = mkKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	err = db.updateLocale(_T("pt_BR"));
	MojTestErrCheck(err);

	// put Kind for pt_BR test
	MojObject ptKind;
	err = ptKind.fromJson(MojTestKindStrPT);
	MojTestErrCheck(err);
	err = db.putKind(ptKind);
	MojTestErrCheck(err);

	err = put(db, MojTestKindStrNamePT, MojTestObjectsPT);
	MojTestErrCheck(err);

	err = checkOrder(db, MojTestKindStrNamePT, _T("[7,8,11,12,10,9,13,14,19,21,20,22,18,16,17,15,1,5,4,6,2,3]"));
	MojTestErrCheck(err);

	err = ptKind.getRequired(MojDbServiceDefs::IdKey, delKindId);
	MojTestErrCheck(err);
	err = db.delKind(delKindId, found);
	MojTestErrCheck(err);

	return MojErrNone;
}

MojErr MojDbLocaleTest::put(MojDb& db, const MojChar* kindName, const MojChar** objects)
{
	// make sure that we are still consistent by deleting and re-adding
	MojDbQuery query;
	MojErr err = query.from(kindName);
	MojTestErrCheck(err);
	MojUInt32 count = 0;
	err = db.del(query, count);
	MojTestErrCheck(err);

	for (const MojChar** i = objects; *i != NULL; ++i) {
		MojObject obj;
		err = obj.fromJson(*i);
		MojTestErrCheck(err);
		err = db.put(obj);
		MojTestErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbLocaleTest::checkOrder(MojDb& db, const MojChar* kindName, const MojChar* expectedJson)
{
	MojObject expected;
	MojErr err = expected.fromJson(expectedJson);
	MojTestErrCheck(err);

    MojTestAssert (expected.type() == MojObject::TypeArray);

	MojObject::ConstArrayIterator i = expected.arrayBegin();

	MojDbQuery query;
	err = query.from(kindName);
	MojTestErrCheck(err);
	err = query.order(_T("foo"));
	MojTestErrCheck(err);
	MojDbCursor cur;
	err = db.find(query, cur);
	MojTestErrCheck(err);
	for (;;) {
		MojObject obj;
		bool found = false;
		err = cur.get(obj, found);
		MojTestErrCheck(err);
		if (!found)
			break;
		MojTestAssert(i != expected.arrayEnd());
		MojObject id;
		MojTestAssert(obj.get(MojDb::IdKey, id));
		MojInt64 idInt = id.intValue();
		MojUnused(idInt);

		MojTestAssert(id == *i++);
	}
	return MojErrNone;
}

void MojDbLocaleTest::cleanup()
{
	(void) MojRmDirRecursive(MojDbTestDir);
}
