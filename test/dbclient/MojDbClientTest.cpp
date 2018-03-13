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

#include "db/MojDbServiceClient.h"
#include "core/MojGmainReactor.h"
#include "luna/MojLunaService.h"
#include "MojDbClientTest.h"
#include "MojDbClientTestResultHandler.h"
#include "MojDbClientTestWatcher.h"

void MojDbClientTest::SetUp()
{
	if (!m_dbClient.get()) {
		MojAutoPtr<MojGmainReactor> reactor(new MojGmainReactor);
		MojAssert(reactor.get());
		MojErr err = reactor->init();
		MojAssertNoErr(err);

		MojAutoPtr<MojLunaService> svc(new MojLunaService);
		MojAssert(svc.get());
		err = svc->open(_T("com.webos.db8.test.client"));
		MojAssertNoErr(err);
		err = svc->attach(reactor->impl());
		MojAssertNoErr(err);

		m_reactor = reactor;
		m_service = svc;
		m_dbClient.reset(new MojDbServiceClient(m_service.get()));
		MojAssert(m_dbClient.get());
	}

	registerType();
}

void MojDbClientTest::TearDown()
{
	MojErr err;

	err = delTestData();
	MojAssertNoErr(err);
	err=err; //Suppress "not used" warning when asserts compiled out

}

MojErr MojDbClientTest::delTestData()
{
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());
	MojErr err = m_dbClient->delKind(handler->slot(), _T("LunaDbClientTest:1"));
	MojExpectNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojExpectNoErr(err);
	MojAssert(handler->database_error() == MojErrDbKindNotRegistered || handler->database_error() == MojErrNone);

	return MojErrNone;
}

MojErr MojDbClientTest::registerType()
{
	// register type
	MojErr err = writeTestObj(_T("{\"id\":\"LunaDbClientTest:1\",\"owner\":\"com.webos.db8.test.client\",")
							  _T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},{\"name\":\"barfoo\",\"props\":[{\"name\":\"bar\"},{\"name\":\"foo\"}]}]}"), true);
	MojExpectNoErr(err);

	return MojErrNone;
}

MojErr MojDbClientTest::updateType()
{
	// update type - add a new index
	MojErr err = writeTestObj(_T("{\"id\":\"LunaDbClientTest:1\",\"owner\":\"com.webos.db8.test.client\",")
							  _T("\"indexes\":[{\"name\":\"foo\",\"props\":[{\"name\":\"foo\"}]},{\"name\":\"barfoo\",\"props\":[{\"name\":\"bar\"},{\"name\":\"foo\"}]},{\"name\":\"foobar\",\"props\":[{\"name\":\"foobar\"}]}]}"), true);
	MojExpectNoErr(err);

	return MojErrNone;
}

MojErr MojDbClientTest::writeTestObj(const MojChar* json, MojObject* idOut)
{
	return writeTestObj(json, false, idOut);
}

MojErr MojDbClientTest::writeTestObj(const MojChar* json, bool kind, MojObject* idOut)
{
	// not a test, per say, but a utility to populate the datastore for performing specific tests
	bool retb;
	MojObject obj;
	MojErr err = obj.fromJson(json);
	MojExpectNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	if (kind) {
		err = m_dbClient->putKind(handler->slot(), obj);
		MojExpectNoErr(err);
	} else {
		err = m_dbClient->put(handler->slot(), obj);
		MojExpectNoErr(err);
	}

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojExpectNoErr(err);
	// verify result
	MojExpectNoErr(handler->database_error());
	MojObject results;
        MojString s;
        handler->result().toJson(s);
	if (!kind) {
		retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
		MojAssert(retb);

		MojObject::ConstArrayIterator iter = results.arrayBegin();
		MojAssert(iter != results.arrayEnd());

		MojObject id;
		err = iter->getRequired(MojDbServiceDefs::IdKey, id);
		MojExpectNoErr(err);
		MojAssert(!id.undefined());
		if (idOut) {
			*idOut = id;
		}
	}

	return MojErrNone;
}

MojErr MojDbClientTest::getTestObj(MojObject id, MojObject& objOut)
{
	bool retb;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	MojObject idObj(id);
	MojErr err = m_dbClient->get(handler->slot(), idObj);
	MojExpectNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojExpectNoErr(err);

	// verify result
	MojExpectNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);
	MojObject::ConstArrayIterator iter = results.arrayBegin();
	MojAssert(iter != results.arrayEnd());
	objOut = *iter;

	return MojErrNone;
}

MojErr MojDbClientTest::verifyFindResults(const MojDbClientTestResultHandler* handler,
		MojObject idExpected, const MojChar* barExpected)
{
	// verify that all results of a query have the same "bar" value, and that the specified
	// if is present in one result

	MojExpectNoErr(handler->database_error());
	MojObject results;
	MojAssert(const_cast<MojDbClientTestResultHandler *>(handler)->result().get(MojDbServiceDefs::ResultsKey, results));

	bool foundId = false;
	for (MojObject::ConstArrayIterator i = results.arrayBegin(); i != results.arrayEnd(); i++) {
		MojString bar;
		MojErr err = i->getRequired(_T("bar"), bar);
		MojExpectNoErr(err);
		MojAssert(bar == barExpected);
		MojObject id;
		err = i->getRequired(MojDb::IdKey, id);
		MojExpectNoErr(err);
		if (id == idExpected) {
			foundId = true;
		}
	}
	MojAssert(foundId);

	return MojErrNone;
}

MojErr MojDbClientTest::testErrorHandling(bool multiResponse)
{
	// error handling is implemented identically for all
	// single-response calls, and for all mult-response calls.
	// we therefore test find() as a representative example,
	// with and without watch semantics (for multi and  single
	// repsonses, respectively)

	// query on un-indexed field
	MojDbQuery q;
	MojErr err = q.from(_T("LunaDbClientTest:1"));
	MojExpectNoErr(err);
	MojString baz;
	baz.assign(_T("baz_invalid"));
	MojObject whereObj(baz);
	err = q.where(_T("baz"), MojDbQuery::OpEq, whereObj);
	MojExpectNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	bool watch = multiResponse;
	err = m_dbClient->find(handler->slot(), q, watch);
	MojExpectNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojExpectNoErr(err);

	MojAssert(handler->database_error() == MojErrDbNoIndexForQuery);

	// verify that successful callback dispatch on a new handler
	// still works after an error
	err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":1,\"bar\":\"1\"})"));
	MojExpectNoErr(err);

	return MojErrNone;
}

TEST_F(MojDbClientTest, testPut)
{
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	MojObject obj;
	MojErr err;
	bool retb;

	err = obj.putString(MojDb::KindKey, _T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	err = obj.putString(_T("foo"), _T("test_put_foo"));
	MojAssertNoErr(err);
	err = obj.putString(_T("bar"), _T("test_put_bar"));
	MojAssertNoErr(err);

	err = m_dbClient->put(handler->slot(), obj);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	retb = (iter != results.arrayEnd());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
	MojAssert(retb);

	// verify only 1 result
	iter++;
	retb = (iter == results.arrayEnd());
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testMultiPut)
{
	MojErr err;
	bool retb;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	MojUInt32 count = 10;
	MojObject::ObjectVec array;

	for (MojSize i = 0; i < count; i++) {
		MojObject obj;

		err = obj.putString(MojDb::KindKey, _T("LunaDbClientTest:1"));
		MojAssertNoErr(err);
		err = obj.putString(_T("foo"), _T("test_multiput_foo"));
		MojAssertNoErr(err);
		err = obj.putString(_T("bar"), _T("test_multiput_bar"));
		MojAssertNoErr(err);
		err = array.push(obj);
		MojAssertNoErr(err);
	}

	err = m_dbClient->put(handler->slot(), array.begin(), array.end());
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	for (MojSize i = 0; i < count; i++) {
		retb = (iter != results.arrayEnd());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
		MojAssert(retb);
		iter++;
	}
	// verify no extra results
	retb = (iter == results.arrayEnd());
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testGet)
{
	// write an object
	MojObject id;
	bool retb;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_get_foo\",\"bar\":\"test_get_bar\"})"), &id);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	MojObject idObj(id);
	err = m_dbClient->get(handler->slot(), idObj);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	retb = (iter != results.arrayEnd());
	MojAssert(retb);
	MojObject idReturned = -1;
	err = iter->getRequired(MojDb::IdKey, idReturned);

	MojAssertNoErr(err);
	retb = (idReturned == id);
	MojAssert(retb);
	retb = (iter->find(MojDb::RevKey) != iter->end());
	MojAssert(retb);

	MojString foo;
	err = iter->getRequired(_T("foo"), foo);
	MojAssertNoErr(err);
	retb = (foo == _T("test_get_foo"));
	MojAssert(retb);
	MojString bar;
	err = iter->getRequired(_T("bar"), bar);
	MojAssertNoErr(err);
	retb = (bar == _T("test_get_bar"));
	MojAssert(retb);

	// verify only 1 result
	iter++;
	retb = (iter == results.arrayEnd());
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testMultiGet)
{
	MojUInt32 itemCount = 10;
	MojErr err = MojErrNone;
	MojVector<MojObject> ids;
	bool retb;

	for (MojUInt32 i = 0; i < itemCount; i++) {
		MojObject id;
		err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_multiget_foo\",\"bar\":\"test_multiget_bar\"})"), &id);
		MojAssertNoErr(err);
		err = ids.push(id);
		MojAssertNoErr(err);
	}

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	MojObject::ObjectVec array;
	for (MojUInt32 i = 0; i < itemCount; i++)
	{
		err = array.push(ids.at(i));
		MojAssertNoErr(err);
	}

	err = m_dbClient->get(handler->slot(), array.begin(), array.end());
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojUInt32 cReceived = 0;
	for (MojObject::ConstArrayIterator i = results.arrayBegin(); i != results.arrayEnd(); i++) {
		MojObject id = -1;
		retb = (i->get(MojDb::IdKey, id));
		MojAssert(retb);
		retb = (id != -1);
		MojAssert(retb);
		retb = (id == ids.at(cReceived++));
		MojAssert(retb);
	}
	retb = (cReceived == itemCount);
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testDel)
{
	// write an object
	MojObject id;
	bool retb;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_del_foo\",\"bar\":\"test_del_bar\"})"), &id);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	// delete
	MojObject idObj(id);
	err = m_dbClient->del(handler->slot(), idObj, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	retb = (iter != results.arrayEnd());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
	MojAssert(retb);

	// verify only 1 result
	iter++;
	retb = (iter == results.arrayEnd());
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testQueryDel)
{
	bool retb;
	// write an object
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_qdel_foo\",\"bar\":\"test_qdel_bar\"})"));
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_qdel_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	// delete
	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	err = m_dbClient->del(handler->slot(), q, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojInt64 count = -1;
	retb = (handler->result().get(MojDbServiceDefs::CountKey, count));
	MojAssert(retb);
	retb = (count == 1);
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testMultiDel)
{
	MojUInt32 itemCount = 10;
	MojErr err = MojErrNone;
	MojVector<MojObject> ids;
	bool retb;

	for (MojUInt32 i = 0; i < itemCount; i++) {
		MojObject id;
		err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_multidel_foo\",\"bar\":\"test_multidel_\"})"), &id);
		MojAssertNoErr(err);
		err = ids.push(id);
		MojAssertNoErr(err);
	}

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	MojObject::ObjectVec array;
	for (MojUInt32 i = 0; i < itemCount; i++)
	{
		MojObject idObj(ids.at(i));
		err = array.push(idObj);
		MojAssertNoErr(err);
	}

	err = m_dbClient->del(handler->slot(), array.begin(), array.end(), MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	for (MojSize i = 0; i < itemCount; i++) {
		retb = (iter != results.arrayEnd());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
		MojAssert(retb);
		iter++;
	}
	// verify no extra results
	retb = (iter == results.arrayEnd());
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testMerge)
{
	bool retb;
	// write an object
	MojObject id;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_merge_foo\",\"bar\":\"test_merge_\"})"), &id);
	MojAssertNoErr(err);

	// modify select field
	MojObject obj2;
	err = obj2.put(MojDb::IdKey, id);
	MojAssertNoErr(err);
	err = obj2.putString(_T("bar"), _T("test_merge_bar_post"));
	MojAssertNoErr(err);

	// merge object
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());;

	err = m_dbClient->merge(handler->slot(), obj2);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result count
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	retb = (iter != results.arrayEnd());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
	MojAssert(retb);

	// verify only 1 result
	iter++;
	retb = (iter == results.arrayEnd());
	MojAssert(retb);

	// verify field modified
	MojObject objRetrieved;
	err = getTestObj(id, objRetrieved);
	MojAssertNoErr(err);
	MojString barRetrieved;
	err = objRetrieved.getRequired(_T("bar"), barRetrieved);
	MojAssertNoErr(err);
	retb = (barRetrieved == _T("test_merge_bar_post"));
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testQueryMerge)
{
	bool retb;
	// write an object
	MojObject id;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_qmerge_foo\",\"bar\":\"test_qmerge_bar\"})"), &id);
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_qmerge_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	// modify "bar" property
	MojObject props;
	err = props.putString(_T("bar"), _T("test_qmerge_bar_post"));
	MojAssertNoErr(err);

	// merge
	err = m_dbClient->merge(handler->slot(), q, props);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result count
	MojAssertNoErr(handler->database_error());
	MojInt64 count = -1;
	retb = (handler->result().get(MojDbServiceDefs::CountKey, count));
	MojAssert(retb);
	retb = (count == 1);
	MojAssert(retb);

	// verify retrieved obj is merged
	MojObject objRetrieved;
	err = getTestObj(id, objRetrieved);
	MojAssertNoErr(err);
	MojString barRetrieved;
	err = objRetrieved.getRequired(_T("bar"), barRetrieved);
	MojAssertNoErr(err);
	retb = (barRetrieved == _T("test_qmerge_bar_post"));
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testMultiMerge)
{
	MojUInt32 itemCount = 10;
	MojErr err = MojErrNone;
	MojVector<MojObject> ids;
	bool retb;

	for (MojUInt32 i = 0; i < itemCount; i++) {
		MojObject id;
		err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_multimerge_foo\",\"bar\":\"test_multimerge_bar\"})"), &id);
		MojAssertNoErr(err);
		err = ids.push(id);
		MojAssertNoErr(err);
	}

	// modify select field in each object
	MojObject::ObjectVec array;
	for (MojUInt32 i = 0; i < itemCount; i++) {
		MojObject obj;
		err = obj.put(MojDb::IdKey, ids.at(i));
		MojAssertNoErr(err);
		err = obj.putString(_T("bar"), _T("test_multimerge_bar_post"));
		MojAssertNoErr(err);
		err = array.push(obj);
		MojAssertNoErr(err);
	}

	// merge objects
	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());
	err = m_dbClient->merge(handler->slot(), array.begin(), array.end());
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result count
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	for (MojSize i = 0; i < itemCount; i++) {
		retb = (iter != results.arrayEnd());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
		MojAssert(retb);
		iter++;
	}
	// verify no extra results
	retb = (iter == results.arrayEnd());
	MojAssert(retb);

	// verify objects merged
	for (MojUInt32 i = 0; i < itemCount; i++) {
		MojObject objRetrieved;
		err = getTestObj(ids.at(i), objRetrieved);
		MojAssertNoErr(err);
		MojString barRetrieved;
		err = objRetrieved.getRequired(_T("bar"), barRetrieved);
		MojAssertNoErr(err);
		retb = (barRetrieved == _T("test_multimerge_bar_post"));
		MojAssert(retb);
	}
}

TEST_F(MojDbClientTest, testBasicFind)
{
	MojObject id;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_basic_find_foo\",\"bar\":\"test_basic_find_bar\"})"), &id);
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_basic_find_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	err = m_dbClient->find(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	err = verifyFindResults(handler.get(), id, _T("test_basic_find_bar"));
	MojAssertNoErr(err);

	handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	q.limit(MojDbServiceHandler::MaxQueryLimit*2);

	err = m_dbClient->find(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssert(handler->database_error() != MojErrNone);
}

TEST_F(MojDbClientTest, testBasicSearch)
{
	MojObject id;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_basic_search_foo\",\"bar\":\"test_basic_search_bar\"})"), &id);
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_basic_search_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	err = m_dbClient->search(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	err = verifyFindResults(handler.get(), id, _T("test_basic_search_bar"));
	MojAssertNoErr(err);
}

TEST_F(MojDbClientTest, testUpdatedFind)
{
	MojErr err = updateType();
	MojAssertNoErr(err);

	MojObject id;
	err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"bar\":\"test_updated_find_bar\",\"foobar\":\"test_updated_find_foobar\"})"), &id);
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString baz;
	baz.assign(_T("test_updated_find_foobar"));
	MojObject whereObj(baz);
	err = q.where(_T("foobar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestResultHandler> handler = new MojDbClientTestResultHandler;
	MojAssert(handler.get());

	err = m_dbClient->find(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	err = verifyFindResults(handler.get(), id, _T("test_updated_find_bar"));
	MojAssertNoErr(err);
}

TEST_F(MojDbClientTest, testWatchFind)
{
	MojObject id;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_watchfind_foo\",\"bar\":\"test_watchfind_bar\"})"), &id);
	MojAssertNoErr(err);

	// select obj via query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_watchfind_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestWatcher> handler = new MojDbClientTestWatcher;
	MojAssert(handler.get());

	err = m_dbClient->find(handler->slot(), q, true);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	err = verifyFindResults(handler.get(), id, _T("test_watchfind_bar"));
	MojAssertNoErr(err);

	// add obj that matches query
	err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":1,\"bar\":\"test_watchfind_bar\"})"));
	MojAssertNoErr(err);

	// check if callback fired via dispatchfrom writeTestObj()
	if (handler->fired())
		return;

	// block until watch CB received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);
	MojAssert(handler->fired());
}

TEST_F(MojDbClientTest, testWatch)
{
	bool retb;
	MojErr err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_watch_foo\",\"bar\":\"test_watch_bar\"})"));
	MojAssertNoErr(err);

	// format query
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_watch_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestWatcher> handler = new MojDbClientTestWatcher;
	MojAssert(handler.get());

	err = m_dbClient->watch(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	// make sure no results returned
	retb = !handler->result().contains(MojDbServiceDefs::ResultsKey);
	MojAssert(retb);
	// but the watch should have fired because something already exists in the db that matches our watch
	MojAssert(handler->fired());

	// create a new query for a different value of bar
	MojDbQuery q2;
	err = q2.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar2;
	bar2.assign(_T("test_watch_bar2"));
	MojObject whereObj2(bar2);
	err = q2.where(_T("bar"), MojDbQuery::OpEq, whereObj2);
	MojAssertNoErr(err);

	// and watch that query
	handler.reset(new MojDbClientTestWatcher);
	MojAssert(handler.get());

	err = m_dbClient->watch(handler->slot(), q2);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	// make sure no results returned
	retb = !handler->result().contains(MojDbServiceDefs::ResultsKey);
	MojAssert(retb);
	// and that the watch has NOT fired
	MojAssert(!handler->fired());

	// add obj that matches query
	err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":1,\"bar\":\"test_watch_bar2\"})"));
	MojAssertNoErr(err);

	// if callback hasn't fired yet via dispatch from writeTestObj(), wait for it now
	if (!handler->fired()) {
		// block until watch CB received
		err = handler->wait(m_dbClient->service());
		MojAssertNoErr(err);
	}

	MojAssert(handler->fired());
	// cleanup afterwards so that subsequent runs will work
	MojRefCountedPtr<MojDbClientTestResultHandler> resultHandler = new MojDbClientTestResultHandler;
	MojAssert(resultHandler.get());

	err = m_dbClient->del(resultHandler->slot(), q2, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssert(resultHandler->database_error() == MojErrNone);
}

TEST_F(MojDbClientTest, testErrorHandlingWithMultiResponse)
{
	MojAssertNoErr(testErrorHandling(true));
}

TEST_F(MojDbClientTest, testErrorHandlingWithoutMultiResponse)
{
	MojAssertNoErr(testErrorHandling(false));
}

TEST_F(MojDbClientTest, testHdlrCancel)
{
	// issue a watch command, cancel handler, and
	// trigger a watch event - verify that handler callback
	// is not invoked

	// format query
	MojDbQuery q;
	MojErr err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString bar;
	bar.assign(_T("test_hdlr_cancel_bar"));
	MojObject whereObj(bar);
	err = q.where(_T("bar"), MojDbQuery::OpEq, whereObj);
	MojAssertNoErr(err);

	MojRefCountedPtr<MojDbClientTestWatcher> handler = new MojDbClientTestWatcher;
	MojAssert(handler.get());

	err = m_dbClient->watch(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received, cancel handler
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);
	MojAssertNoErr(handler->database_error());
	handler->slot().cancel();

	// add obj that matches query
	err = writeTestObj(_T("{\"_kind\":\"LunaDbClientTest:1\",\"foo\":1,\"bar\":\"test_hdlr_cancel_bar\"})"));
	MojAssertNoErr(err);

	// without cancel, the callback is fired via a dispatch from writeTestObj()
	// verify that the callback was not invoked
	MojAssert(!handler->fired());

	// cleanup afterwards so that subsequent runs will work
	MojRefCountedPtr<MojDbClientTestResultHandler> resultHandler = new MojDbClientTestResultHandler;
	MojAssert(resultHandler.get());

	err = m_dbClient->del(resultHandler->slot(), q, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	err = resultHandler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(resultHandler->database_error());
}

TEST_F(MojDbClientTest, testCompact)
{
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//compact
	err = m_dbClient->compact(handler->slot());
	MojAssertNoErr(err);

	//block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify no error
	MojAssertNoErr(handler->database_error());
}

// testPurge was originally disabled
#if 0
TEST_F(MojDbClientTest, testPurge)
{
	bool retb;
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//purge everything in the dB
	err = m_dbClient->purge(handler->slot(), 0);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result has a count attribute
	MojAssertNoErr(handler->database_error());
	MojInt64 count = -1;
	retb = handler->result().get(MojDbServiceDefs::CountKey, count);
	MojAssert(retb);

	// put 10 objects into the database
	MojObject::ObjectVec array;
	MojSize numObjects = 10;
	for (MojSize i = 0; i < numObjects; i++) {
		MojObject obj;

		err = obj.putString(MojDb::KindKey, _T("LunaDbClientTest:1"));
		MojAssertNoErr(err);
		err = obj.putString(_T("foo"), _T("test_purge_foo"));
		MojAssertNoErr(err);
		err = obj.putString(_T("bar"), _T("test_purge_bar"));
		MojAssertNoErr(err);
		err = array.push(obj);
		MojAssertNoErr(err);
	}

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());
	err = m_dbClient->put(handler->slot(), array.begin(), array.end());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = handler->result().get(MojDbServiceDefs::ResultsKey, results);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	for (MojSize i = 0; i < numObjects; i++) {
		retb = (iter != results.arrayEnd());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
		MojAssert(retb);
		retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
		MojAssert(retb);
		iter++;
	}
	// verify no extra results
	retb = (iter == results.arrayEnd());
	MojAssert(retb);

	//delete numToDelete of the objects
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());
	MojUInt32 numToDelete = 6;
	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	q.limit(numToDelete);

	// delete
	err = m_dbClient->del(handler->slot(), q, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result
	MojAssertNoErr(handler->database_error());
	count = -1;
	retb = (handler->result().get(MojDbServiceDefs::CountKey, count));
	MojAssert(retb);
	retb = (count == numToDelete);
	MojAssert(retb);

	//purge all deleted objects
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());
	err = m_dbClient->purge(handler->slot(), 0);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify purge count
	MojAssertNoErr(handler->database_error());
	count = -1;
	retb = handler->result().get(MojDbServiceDefs::CountKey, count);
	MojAssert(retb);
	retb = (count == numToDelete);
	MojAssert(retb);

	//purge again - shouldn't purge anything
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());
	err = m_dbClient->purge(handler->slot(), 0);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify purge count
	MojAssertNoErr(handler->database_error());
	count = -1;
	retb = handler->result().get(MojDbServiceDefs::CountKey, count);
	MojAssert(retb);
	retb = (count == 0);
	MojAssert(retb);
}
#endif
// testPurgeStatus was originally disabled
#if 0
TEST_F(MojDbClientTest, testPurgeStatus)
{
	bool retb;
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//getPurgeStatus
	err = m_dbClient->purgeStatus(handler->slot());
	MojAssertNoErr(err);

	//block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify result has a revNum attribute
	MojAssertNoErr(handler->database_error());
	MojObject revNum;
	retb = (handler->result().get(MojDbServiceDefs::RevKey, revNum));
	MojAssert(retb);

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	// put 1 object into the database
	MojObject obj;
	err = obj.putString(MojDb::KindKey, _T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	err = obj.putString(_T("foo"), _T("test_purgeStatus_foo"));
	MojAssertNoErr(err);
	err = obj.putString(_T("bar"), _T("test_purgeStatus_bar"));
	MojAssertNoErr(err);

	err = m_dbClient->put(handler->slot(), obj);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// get revNum of the put
	MojAssertNoErr(handler->database_error());
	MojObject results;
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojObject firstResult;
	retb = (results.at(0, firstResult));
	MojAssert(retb);
	MojObject rev;
	retb = (firstResult.get(MojDbServiceDefs::RevKey, rev));
	MojAssert(retb);
	MojObject id;
	retb = (firstResult.get(MojDbServiceDefs::IdKey, id));
	MojAssert(retb);

	//delete the object
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	err = m_dbClient->del(handler->slot(), id, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// make sure one object was deleted
	MojAssertNoErr(handler->database_error());
	retb = (handler->result().get(MojDbServiceDefs::ResultsKey, results));
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = results.arrayBegin();
	retb = (iter != results.arrayEnd());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(retb);
	retb = (iter->find(MojDbServiceDefs::RevKey) != iter->end());
	MojAssert(retb);

	// verify only 1 result
	iter++;
	retb = (iter == results.arrayEnd());
	MojAssert(retb);

	//purge
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	err = m_dbClient->purge(handler->slot(), 0);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify purge count
	MojAssertNoErr(handler->database_error());
	MojInt64 count = -1;
	retb = (handler->result().get(MojDbServiceDefs::CountKey, count));
	MojAssert(retb);
	retb = (count == 1);
	MojAssert(retb);

	//get the purge status
	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	err = m_dbClient->purgeStatus(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// check the last purge rev num
	MojAssertNoErr(handler->database_error());
	MojObject lastRevNum;
	retb = (handler->result().get(_T(MojDbServiceDefs::RevKey), lastRevNum));
	MojAssert(retb);
	retb = (lastRevNum.intValue() > rev.intValue());
	MojAssert(retb);
}
#endif

TEST_F(MojDbClientTest, testDeleteKind)
{
	bool retb;
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//delete LunaDbClientTest kind
	MojString str;
	str.assign("LunaDbClientTest:1");
	err = m_dbClient->delKind(handler->slot(), str);
	MojAssertNoErr(err);

	//block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	// verify there was no error
	MojAssertNoErr(handler->database_error());

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	MojDbQuery q;
	err = q.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);

	err = m_dbClient->find(handler->slot(), q);
	MojAssertNoErr(err);

	// block until repsonse received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	retb = (handler->database_error() == MojErrDbKindNotRegistered);
	MojAssert(retb);
}

TEST_F(MojDbClientTest, testBatch)
{
	bool retb;
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//first put the kind outside a batch
	err = registerType();
	MojAssertNoErr(err);

	//now create a batch and put one object
	MojAutoPtr<MojDbBatch> batch;
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	MojObject obj;

	err = obj.putString(_T("_kind"), _T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	err = obj.putString(_T("foo"), _T("test_batch_foo"));
	MojAssertNoErr(err);
	err = obj.putString(_T("bar"), _T("test_batch_bar"));
	MojAssertNoErr(err);

	err = batch->put(obj);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	MojObject responses;
	retb = handler->result().get(MojDbServiceDefs::ResponsesKey, responses);
	MojAssert(retb);

	MojObject::ConstArrayIterator iter = responses.arrayBegin();
	MojAssert(iter != responses.arrayEnd());
	MojObject results;
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	MojObject::ConstArrayIterator putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojAssert(putIter->find(MojDbServiceDefs::IdKey) != putIter->end());
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != putIter->end());

	// verify only 1 result
	putIter++;
	MojAssert(putIter == results.arrayEnd());
	iter++;
	MojAssert(iter == responses.arrayEnd());

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//create another batch with two puts, one that will fail
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	MojObject obj2;

	err = obj2.putString(_T("_kind"), _T("RandomKind:1"));
	MojAssertNoErr(err);
	err = obj2.putString(_T("foo"), _T("test_batch_foo"));
	MojAssertNoErr(err);
	err = obj2.putString(_T("bar"), _T("test_batch_bar"));
	MojAssertNoErr(err);

	err = obj.putString(_T("foo"), _T("test_batch_foo2"));
	MojAssertNoErr(err);
	err = obj.putString(_T("bar"), _T("test_batch_bar2"));
	MojAssertNoErr(err);

	err = batch->put(obj);
	MojAssertNoErr(err);

	err = batch->put(obj2);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssert(handler->database_error() == MojErrDbKindNotRegistered);
	MojObject errorCode;
	MojAssert(handler->result().get(MojServiceMessage::ErrorCodeKey, errorCode));
	MojObject errorText;
	MojAssert(handler->result().get(MojServiceMessage::ErrorTextKey, errorText));

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	//do a find in a batch and verify that the last transaction was aborted - there should only be one result
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	MojDbQuery query;
	err = query.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);

	err = batch->find(query, true);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	retb = handler->result().get(MojDbServiceDefs::ResponsesKey, responses);
	MojAssert(retb);

	iter = responses.arrayBegin();
	MojAssert(iter != responses.arrayEnd());
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojObject objId;
	err = putIter->getRequired(MojDb::IdKey, objId);
	MojAssertNoErr(err);
	MojInt64 rev;
	err = putIter->getRequired(MojDb::RevKey, rev);
	MojAssertNoErr(err);
	MojInt64 count;
	err = iter->getRequired(MojDbServiceDefs::CountKey, count);
	MojAssertNoErr(err);
	MojAssert(count == 1);

	// verify only 1 result
	putIter++;
	MojAssert(putIter == results.arrayEnd());
	iter++;
	MojAssert(iter == responses.arrayEnd());

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	// test what happens if you modify an object and then query for it in the same batch
	// (the query should find the modified object even though the txn hasn't committed)
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	err = obj.put(MojDb::IdKey, objId);
	MojAssertNoErr(err);
	err = obj.putString(_T("foo"), _T("test_batch_modifiedFoo"));
	MojAssertNoErr(err);
	err = obj.put(MojDb::RevKey, rev);
	MojAssertNoErr(err);

	err = batch->put(obj);
	MojAssertNoErr(err);

	MojDbQuery fooQuery;
	err = fooQuery.from(_T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	MojString whereStr;
	err = whereStr.assign( _T("test_batch_modifiedFoo"));
	MojAssertNoErr(err);
	err = fooQuery.where(_T("foo"), MojDbQuery::OpEq, whereStr);
	MojAssertNoErr(err);

	err = batch->find(fooQuery, true);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	retb = handler->result().get(MojDbServiceDefs::ResponsesKey, responses);
	MojAssert(retb);

	iter = responses.arrayBegin();
	MojAssert(iter != responses.arrayEnd());
	//put result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojAssert(putIter->find(MojDbServiceDefs::IdKey) != putIter->end());
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != putIter->end());

	iter++;
	//find result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojObject dbId;
	err = putIter->getRequired(MojDb::IdKey, dbId);
	MojAssertNoErr(err);
	MojAssert(dbId == objId);
	MojAssert(putIter->find(MojDb::RevKey) != putIter->end());
	err = iter->getRequired(MojDbServiceDefs::CountKey, count);
	MojAssertNoErr(err);
	MojAssert(count == 1);

	//verify those were the only two results
	iter++;
	MojAssert(iter == responses.arrayEnd());

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	// test a delete and then a get in a batch
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	err = batch->del(objId, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	err = batch->get(objId);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	retb = handler->result().get(MojDbServiceDefs::ResponsesKey, responses);
	MojAssert(retb);

	iter = responses.arrayBegin();
	MojAssert(iter != responses.arrayEnd());
	//del result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojAssert(putIter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != iter->end());

	// verify only 1 result
	putIter++;
	MojAssert(putIter == results.arrayEnd());

	iter++;
	//get result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	err = putIter->getRequired(MojDb::IdKey, dbId);
	MojAssertNoErr(err);
	MojAssert(dbId == objId);
	bool deleted;
	err = putIter->getRequired(MojDb::DelKey, deleted);
	MojAssertNoErr(err);
	MojAssert(deleted);

	//verify those were the only two results
	iter++;
	MojAssert(iter == responses.arrayEnd());

	handler.reset(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	// test a put, get, merge, get, del in a batch
	err = m_dbClient->createBatch(batch);
	MojAssertNoErr(err);
	MojAssert(batch.get());

	MojObject object;
	MojString myId;
	err = myId.assign(_T("myId"));
	MojAssertNoErr(err);
	err = object.putString(_T("_kind"), _T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	err = object.putString(_T("foo"), _T("test_batch_myFoo"));
	MojAssertNoErr(err);
	err = object.putString(MojDb::IdKey, myId);
	MojAssertNoErr(err);

	err = batch->put(object);
	MojAssertNoErr(err);

	err = batch->get(myId);
	MojAssertNoErr(err);

	MojObject object2;
	err = object2.putString(_T("_kind"), _T("LunaDbClientTest:1"));
	MojAssertNoErr(err);
	err = object2.putString(_T("bar"), _T("test_batch_myBar"));
	MojAssertNoErr(err);
	err = object2.putString(MojDb::IdKey, myId);
	MojAssertNoErr(err);

	err = batch->merge(object2);
	MojAssertNoErr(err);

	err = batch->get(myId);
	MojAssertNoErr(err);

	err = batch->del(myId, MojDbFlagForceNoPurge);
	MojAssertNoErr(err);

	err = batch->execute(handler->slot());
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);

	MojAssertNoErr(handler->database_error());
	retb = handler->result().get(MojDbServiceDefs::ResponsesKey, responses);
	MojAssert(retb);

	iter = responses.arrayBegin();
	MojAssert(iter != responses.arrayEnd());
	//put result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	err = putIter->getRequired(MojDbServiceDefs::IdKey, dbId);
	MojAssert(dbId == myId);
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != putIter->end());

	iter++;
	//get result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	err = putIter->getRequired(MojDb::IdKey, dbId);
	MojAssertNoErr(err);
	MojAssert(dbId == myId);
	MojAssert(putIter->find(_T("foo")) != putIter->end());
	MojAssert(putIter->find(_T("bar")) == putIter->end());

	iter++;
	//merge result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter->find(MojDbServiceDefs::IdKey) != putIter->end());
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != putIter->end());
	// verify only 1 result
	putIter++;
	MojAssert(putIter == results.arrayEnd());


	iter++;
	//get result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	err = putIter->getRequired(MojDb::IdKey, dbId);
	MojAssertNoErr(err);
	MojAssert(dbId == myId);
	MojAssert(putIter->find(_T("foo")) != putIter->end());
	MojAssert(putIter->find(_T("bar")) != putIter->end());

	iter++;
	//del result
	err = iter->getRequired(MojDbServiceDefs::ResultsKey, results);
	MojAssertNoErr(err);
	putIter = results.arrayBegin();
	MojAssert(putIter != results.arrayEnd());
	MojAssert(putIter->find(MojDbServiceDefs::IdKey) != iter->end());
	MojAssert(putIter->find(MojDbServiceDefs::RevKey) != iter->end());

	// verify only 1 result
	putIter++;
	MojAssert(putIter == results.arrayEnd());

	//verify there were only four results
	iter++;
	MojAssert(iter == responses.arrayEnd());
}

TEST_F(MojDbClientTest, testPermissions)
{
	MojErr err;
	MojRefCountedPtr<MojDbClientTestResultHandler> handler(new MojDbClientTestResultHandler);
	MojAssert(handler.get());

	MojObject perm;
	err = perm.fromJson(
			_T("{\"type\":\"foo\",\"caller\":\"com.test.foo\",\"operations\":{\"create\":\"allow\"},\"object\":\"permtest:1\"}")
			);
	MojAssertNoErr(err);
	err = m_dbClient->putPermission(handler->slot(), perm);
	MojAssertNoErr(err);

	// block until response received
	err = handler->wait(m_dbClient->service());
	MojAssertNoErr(err);
	MojAssertNoErr(handler->database_error());
}
