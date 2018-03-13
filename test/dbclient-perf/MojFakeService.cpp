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

#include "luna/MojLunaRequest.h" /* MojServiceRequest */

#include "FakeLSMessage.h"
#include "MojFakeService.h"
#include "MojFakeMessage.h"

MojFakeService::MojFakeService()
{
}

MojFakeService::~MojFakeService()
{
}

MojErr MojFakeService::dispatch()
{
    return MojErrNone;
}

MojErr MojFakeService::sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut)
{
    MojThreadGuard g(m_mutex, false);

    MojRefCountedPtr<Category> category;
    MojObject payloadObj;
    LSMessage msg;
    bool res;
    MojErr err;

    res = m_categories.get("/", category);
    MojAssert(res == true);

    msg.ref = 1;
    msg.transport_msg = 0;
    msg.sh = 0;
    msg.responseToken = 0;
    msg.category = "/";
    msg.method = method;
    msg.payload = ((MojJsonWriter *) &req->writer())->json().data();

    payloadObj.fromJson(msg.payload);

    MojFakeMessage m(this, &msg);

    err = category->m_handler->invoke(method, &m, payloadObj);
    MojAssert(err == MojErrNone || err == MojErrDbKindNotRegistered);

    err = addRequest(req);
    MojAssert(err == MojErrNone);

    m_mutex.unlock();
    err = handleReply(&m);
    MojAssert(err == MojErrNone);
    m_mutex.lock();

    return MojErrNone;
}

MojErr MojFakeService::cancelImpl(MojServiceRequest* req)
{
    return MojErrNone;
}

MojErr MojFakeService::dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode)
{
    MojAssert(req);
    return req->dispatchReply(msg, payload, errCode);
}

MojErr MojFakeService::enableSubscriptionImpl(MojServiceMessage* msg)
{
    return MojErrNone;
}

MojErr MojFakeService::removeSubscriptionImpl(MojServiceMessage* msg)
{
    return MojErrNone;
}

MojErr MojFakeService::createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut)
{
    reqOut.reset(new MojLunaRequest(this));
    return MojErrNone;
}

MojErr MojFakeService::attach(GMainLoop *loop)
{
    return MojErrNone;
}

MojErr MojFakeService::open(const MojChar* serviceName)
{
    m_name.assign(serviceName);
    return MojErrNone;
}

MojErr MojFakeService::close()
{
    // cancel all pending subscriptions
    MojErr err = MojErrNone;
    for (SubscriptionMap::ConstIterator i = m_subscriptions.begin(); i != m_subscriptions.end();) {
        SubscriptionKey key = (i++).key();
        MojErr errClose = dispatchCancel(key);
        MojErrAccumulate(err, errClose);
    }
    // if you're hitting this assertion, you probably aren't releasing your message in your cancel handler
    MojAssert(m_subscriptions.empty());

    return err;
}

MojErr MojFakeService::addCategory(const MojChar* categoryName, CategoryHandler* handler)
{
    MojAssert(categoryName && handler);
    MojThreadGuard guard(m_mutex);

    MojAssert(!m_categories.contains(categoryName));
    MojRefCountedPtr<Category> category(new Category(this, handler));
    MojAllocCheck(category.get());

    MojString categoryStr;
    MojErr err = categoryStr.assign(categoryName);
    MojErrCheck(err);
    err = m_categories.put(categoryStr, category);

    MojErrCheck(err);

    return MojErrNone;
}
