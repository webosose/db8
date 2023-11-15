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


#include "luna/MojLunaService.h"
#include "luna/MojLunaErr.h"
#include "luna/MojLunaMessage.h"
#include "luna/MojLunaRequest.h"
#include "core/MojObject.h"
#include <map>
#include <string>
#include <luna-service2/lunaservice.h>

const MojChar* const MojLunaService::UriScheme = _T("palm");

MojLunaService::MojLunaService(bool allowPublicMethods/*TODO:keep flag to avoid activitymanager compilation error*/, MojMessageDispatcher* queue)
: MojService(queue),
  m_service(NULL),
  m_loop(NULL),
  m_idleTimeout(0),
  m_idleTimeoutSignal(this)
{
    (void)allowPublicMethods; // to avoid coverity issue, "unused variable"
}

MojLunaService::~MojLunaService()
{
	MojErr err = close();
	MojErrCatchAll(err);
}

MojErr MojLunaService::open(const MojChar* serviceName)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err =  MojService::open(serviceName);
	MojErrCheck(err);

#ifdef WANT_DYNAMIC
    if (m_idleTimeout > 0) {
        LSIdleTimeout(m_idleTimeout, idleTimeoutCallback, this, nullptr);
    }
#endif

	bool retVal;
	MojLunaErr lserr;

    // create service handle
    retVal = LSRegister(serviceName, &m_service, lserr);
    MojLsErrCheck(retVal, lserr);
    retVal = LSSubscriptionSetCancelFunction(m_service, handleCancel, this, lserr);
    MojLsErrCheck(retVal, lserr);

    return MojErrNone;
}

MojErr MojLunaService::close()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = MojErrNone;
	MojErr errClose = MojService::close();
	MojErrAccumulate(err, errClose);

	// destroy service handle
	MojLunaErr lserr;
	bool retVal;
	if (m_service) {
		retVal = LSUnregister(m_service, lserr);
		m_service = NULL;
		MojLsErrAccumulate(err, retVal, lserr);
	}
	return err;
}

// Note: this is a blocking interface exclusively for unit tests.
// It will not deliver cancel callbacks if clients drop off the bus.
MojErr MojLunaService::dispatch()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssertMutexUnlocked(m_mutex);

	MojAssert(m_loop);
	GMainContext* context = g_main_loop_get_context(m_loop);
	g_main_context_iteration(context, true);

	return MojErrNone;
}

MojErr MojLunaService::addCategory(const MojChar* name, CategoryHandler* handler)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(name && handler);
	MojAssertMutexUnlocked(m_mutex);
	MojThreadGuard guard(m_mutex);

	// create array of LSMethods
	MethodVec methods;
	const CategoryHandler::CallbackMap& callbacks = handler->callbacks();
	for (CategoryHandler::CallbackMap::ConstIterator i = callbacks.begin();
		 i != callbacks.end();
		 i++) {
			LSMethod m = {i.key(), &handleRequest, static_cast<LSMethodFlags>(i->m_ls2_method_flags)};
			MojErr err = methods.push(m);
			MojErrCheck(err);
	}
	LSMethod nullMethod = {NULL, NULL, LSMethodFlags()};
    MojErr err = methods.push(nullMethod);
    MojErrCheck(err);

    // create category object to hang on to method array
	MojRefCountedPtr<LunaCategory> cat(new LunaCategory(this, handler, methods));
	MojAllocCheck(cat.get());
	LSMethod* lsMethods = const_cast<LSMethod*>(cat->m_methods.begin());

	MojLunaErr lserr;
    bool retVal;
    	MojAssert(m_service);
    	retVal = LSRegisterCategory(m_service, name, lsMethods, NULL, NULL, lserr);
    	MojLsErrCheck(retVal, lserr);
        retVal = LSCategorySetData(m_service, name, cat.get(), lserr);
        MojLsErrCheck(retVal, lserr);

	MojString categoryStr;
	err = categoryStr.assign(name);
	MojErrCheck(err);
	err = m_categories.put(categoryStr, cat);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojLunaService::addSignalCategory(const MojChar* name, LSSignal* a_signal)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(name && a_signal);
    MojAssertMutexUnlocked(m_mutex);
    MojThreadGuard guard(m_mutex);

    MojLunaErr lserr;
    bool retVal = LSRegisterCategoryAppend(getHandle(), name, NULL, a_signal, lserr);
    MojLsErrCheck(retVal, lserr);
    retVal = LSCategorySetData(getHandle(), name, NULL, lserr);
    MojLsErrCheck(retVal, lserr);

    return MojErrNone;
}

MojErr MojLunaService::createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	reqOut.reset(new MojLunaRequest(this));
	MojAllocCheck(reqOut.get());

	return MojErrNone;
}

MojErr MojLunaService::createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const MojString& proxyRequester)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	reqOut.reset(new MojLunaRequest(this, proxyRequester));
	MojAllocCheck(reqOut.get());

	return MojErrNone;
}


MojErr MojLunaService::createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const char *proxyRequester)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojString proxyRequesterString;
    MojErr err = proxyRequesterString.assign(proxyRequester);
    MojErrCheck(err);

    reqOut.reset(new MojLunaRequest(this, proxyRequesterString));
    MojAllocCheck(reqOut.get());

    return MojErrNone;
}

MojErr MojLunaService::createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const char *originExe, const char *originId, const char *originName)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojString originExeString;
    MojErr err = originExeString.assign(originExe);
    MojErrCheck(err);

    MojString originIdString;
    err = originIdString.assign(originId);
    MojErrCheck(err);

    MojString originNameString;
    err = originNameString.assign(originName);
    MojErrCheck(err);

    reqOut.reset(new MojLunaRequest(this, originExeString, originIdString, originNameString));
    MojAllocCheck(reqOut.get());

    return MojErrNone;
}

MojErr MojLunaService::attach(GMainLoop* loop)
{
	MojAssert(loop);
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojLunaErr lserr;
	bool retVal;
	MojAssert(m_service);
	retVal= LSGmainAttach(m_service, loop, lserr);
	MojLsErrCheck(retVal, lserr);
	m_loop = loop;

	return MojErrNone;
}

LSHandle* MojLunaService::getService()
{
	return m_service;
}

void MojLunaService::setIdleTimeout(MojUInt32 timeout)
{
    m_idleTimeout = timeout;
}

void MojLunaService::connectIdleTimeoutSignal(IdleTimeoutSignal::SlotRef slot)
{
    m_idleTimeoutSignal.connect(slot);
}


MojErr MojLunaService::sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(req && service && method);
	MojAssert(m_service);
	MojAssertMutexLocked(m_mutex);

	MojLunaRequest* lunaReq = static_cast<MojLunaRequest*>(req);
	const MojChar* json = lunaReq->payload();
    LOG_DEBUG("[db_lunaService] request sent: %s", json);

	MojString uri;
	MojErr err = uri.format(_T("%s://%s/%s"), UriScheme, service, method);
	MojErrCheck(err);

	MojLunaErr lserr;
	LSMessageToken lsToken;
	LSHandle* handle = getHandle();
	if (req->numRepliesExpected() > 1) {

		// If proxy requester is present means this is an Call from Application
		if (!lunaReq->isProxyRequest()) {

			// If origin name is present means this is an Indirect luna call
			if (!lunaReq->isOriginRequest()) {
				bool retVal = LSCall(handle, uri, json, &handleResponse, this, &lsToken, lserr);
				MojLsErrCheck(retVal, lserr);
			}
			else{  
				bool retVal = LSCallProxy(handle, lunaReq->getOriginExe(), lunaReq->getOriginId(), lunaReq->getOriginName(), uri, json, &handleResponse, this, &lsToken, lserr);
				MojLsErrCheck(retVal, lserr);
			}
		} else {
			bool retVal = LSCallFromApplication(handle, uri, json, lunaReq->getRequester(), &handleResponse, this, &lsToken, lserr);
			MojLsErrCheck(retVal, lserr);
		}
	} else {
		if (!lunaReq->isProxyRequest()) {

			// If origin name is present means this is an Indirect luna call
			if (!lunaReq->isOriginRequest()) {
				bool retVal = LSCallOneReply(handle, uri, json, &handleResponse, this, &lsToken, lserr);
				MojLsErrCheck(retVal, lserr);
			}
			else{
				bool retVal = LSCallProxyOneReply(handle, lunaReq->getOriginExe(), lunaReq->getOriginId(), lunaReq->getOriginName(), uri, json, &handleResponse, this, &lsToken, lserr);
				MojLsErrCheck(retVal, lserr);
			}
		} else {
			bool retVal = LSCallFromApplicationOneReply(handle, uri, json, lunaReq->getRequester(), &handleResponse, this, &lsToken, lserr);
			MojLsErrCheck(retVal, lserr);
		}
	}
	tokenOut = (Token) lsToken;

	return MojErrNone;
}

MojErr MojLunaService::cancelImpl(MojServiceRequest* req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(req);
	MojAssert(m_service);
	MojAssertMutexUnlocked(m_mutex);

	MojLunaRequest* lunaReq = static_cast<MojLunaRequest*>(req);
	if (req->numRepliesExpected() > 1 && !lunaReq->cancelled()) {
		LSMessageToken lsToken = (LSMessageToken) req->token();
		MojAssert(lsToken != LSMESSAGE_TOKEN_INVALID);
		MojLunaErr lserr;
		LSHandle* handle = getHandle();
		bool cancelled = LSCallCancel(handle, lsToken, lserr);
		MojLsErrCheck(cancelled, lserr);
		lunaReq->cancelled(true);
	}
	return MojErrNone;
}

MojErr MojLunaService::dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(req);
	MojAssertMutexUnlocked(m_mutex);

	// don't automatically cancel a subscription if the payload has "subscribed":true
	bool subscribed = false;
	payload.get(_T("subscribed"), subscribed);

	// remove if there are more responses coming. due to the vagaries of the luna-service protocol,
	// we never really know for sure, but this is a good guess.
	bool remove = (req->numReplies() + 1 >= req->numRepliesExpected()) || (errCode != MojErrNone && !subscribed);
	MojErr err = req->dispatchReply(msg, payload, errCode);
	MojErrCatchAll(err) {
		remove = true;
	}
	if (remove) {
		if (req->numRepliesExpected() == 1) {
			bool found = false;
			err = removeRequest(req, found);
			MojErrCheck(err);
		} else {
			err = cancel(req);
			MojErrCheck(err);
		}
	}
	return MojErrNone;
}

MojErr MojLunaService::enableSubscriptionImpl(MojServiceMessage* msg)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);
	MojAssertMutexUnlocked(m_mutex);

	MojString token;
	MojErr err = token.format(_T("%d"), msg->token());
	MojErrCheck(err);

    MojLunaErr lserr;
    MojLunaMessage* lsMessage = static_cast<MojLunaMessage*>(msg);
    LSHandle* handle = LSMessageGetConnection(lsMessage->impl());
    bool retVal = LSSubscriptionAdd(handle, token, lsMessage->impl(), lserr);
    MojLsErrCheck(retVal, lserr);

    return MojErrNone;
}

MojErr MojLunaService::removeSubscriptionImpl(MojServiceMessage* msg)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);
	MojAssertMutexLocked(m_mutex);

	/*MojString token;
	MojErr err = token.format(_T("%d"), msg->token());
	MojErrCheck(err);

    MojLunaErr lserr;
    MojLunaMessage* lsMessage = static_cast<MojLunaMessage*>(msg);
    LSHandle* handle = LSMessageGetConnection(lsMessage->impl());

    LSSubscriptionIter* iter = NULL;
    bool retVal = LSSubscriptionAcquire(handle, token, &iter, lserr);
    MojLsErrCheck(retVal, lserr);
    while (LSSubscriptionHasNext(iter)) {
    	LSMessage* msg = LSSubscriptionNext(iter);
    	MojAssert(msg);
		MojUnused(msg);
    	LSSubscriptionRemove(iter);
    }
    LSSubscriptionRelease(iter);*/

    return MojErrNone;
}

bool MojLunaService::handleCancel(LSHandle* sh, LSMessage* msg, void* ctx)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(sh && msg && ctx);
	MojLunaService* service = static_cast<MojLunaService*>(ctx);

	MojRefCountedPtr<MojLunaMessage> request(new MojLunaMessage(service, msg));
    if (!request.get())
        return false;

    LOG_DEBUG("[db_lunaService] cancel received: %s", request->payload());
	MojErr err = service->MojService::handleCancel(request.get());
	MojErrCatchAll(err);

    return true;
}

bool MojLunaService::handleRequest(LSHandle* sh, LSMessage* msg, void* ctx)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(sh && msg && ctx);
	MojLunaService::Category* category = static_cast<Category*>(ctx);
	MojLunaService* service = static_cast<MojLunaService*>(category->m_service);

	MojRefCountedPtr<MojLunaMessage> request(new MojLunaMessage(service, msg, category));
    if (!request.get())
        return false;

    if(service)
    {
        service->Statistic(msg);
    }

    LOG_DEBUG("[db_lunaService] request received: %s", request->payload());

	MojErr reqErr;
	MojErr err = reqErr = request->processSubscriptions();
	MojErrCatchAll(err) {
		(void) request->replyError(reqErr);
        return true;
	}

	err = service->MojService::handleRequest(request.get());
	MojErrCatchAll(err);

    return true;
}

bool MojLunaService::handleResponse(LSHandle* sh, LSMessage* msg, void* ctx)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(sh && msg && ctx);
	MojLunaService* service = static_cast<MojLunaService*>(ctx);

	MojRefCountedPtr<MojLunaMessage> request(new MojLunaMessage(service, msg, NULL, true));
    if (!request.get())
        return false;

    LOG_DEBUG("[db_lunaService] response received: %s", request->payload());
	MojErr err = service->handleReply(request.get());
	MojErrCatchAll(err);

    return true;
}

void MojLunaService::idleTimeoutCallback(void* userdata)
{
    LOG_DEBUG("idleTimeoutCallback called");
    MojLunaService* mojLunaService = static_cast<MojLunaService*>(userdata);
    mojLunaService->m_idleTimeoutSignal.fire();
}

void MojLunaService::Statistic(LSMessage* message)
{
    enum mode{ needToCheck, active, inactive};
    //need to check flag
    static mode status = needToCheck;

    //flag checked and statistic is not enabled
    if(status == inactive )
    {
        return;
    }

    if(status == needToCheck)
    {
        struct stat buf;
        std::string path("/var/db/");
        path += m_name.data();
        if(stat(path.c_str(), &buf) == 0)
        {
            status = active;
        }
        else
        {
            status = inactive;
        }
    }

    if(status == active)
    {
        std::string appId_pid;
        if(auto value = LSMessageGetApplicationID(message))
        {
            appId_pid = value;
        }
        std::string callerAppId = appId_pid.substr(0,appId_pid.find(" "));
        std::string senderServiceName;
        if(auto value = LSMessageGetSenderServiceName(message))
        {
            senderServiceName = value;
        }
        std::string messageCategory;
        if(auto value = LSMessageGetCategory(message))
        {
            messageCategory = value;
        }
        std::string messageMethod;
        if(auto value = LSMessageGetMethod(message))
        {
            messageMethod = value;
        }
        std::string messagePayload;
        if(auto value = LSMessageGetPayload(message))
        {
            messagePayload = value;
        }

        static std::map<std::string, size_t> m_bytes;
        static std::map<std::string, int> m_requests;
        int pid = getpid();

        if(senderServiceName.empty())
        {
            if(callerAppId.empty())
            {
                senderServiceName = ("com.palm.configurator");
            }
            else
            {
                senderServiceName = callerAppId;
            }
        }

        m_bytes[senderServiceName] += messagePayload.size();
        m_requests[senderServiceName] += 1;

        char path[100];
        sprintf(path, "/var/db/db8-luna-commands-%d.sh", pid);
        FILE* file = fopen(path, "a+");
        std::string line;
        line = "luna-send -a ";
        line += senderServiceName;
        line += " -n 1 luna://";
        line += m_name.data();
        if(!messageCategory.empty())
        {
            line += messageCategory;
        }
        if(!line.empty() && line[line.size() - 1] != '/')
        {
            line += "/";
        }
        fputs(line.c_str(), file);
        fputs(messageMethod.c_str(), file);
        fputs(" '", file);
        if(!messagePayload.empty())
        {
            fputs(messagePayload.c_str(), file);
        }
        fputs("'\n", file);
        fclose(file);

        sprintf(path, "/var/db/db8-luna-statistic-%d.txt", pid);

        file = fopen(path, "w");
        for(const auto& it : m_bytes)
        {
            fputs("Service: ", file);
            fputs(it.first.c_str(), file);
            fputs("\t\tbytes:", file);
            fprintf(file, "%zd", it.second);
            fputs("\t\tcount:", file);
            fprintf(file, "%d\n", m_requests[it.first]);
        }
        fputs("\n=============================\n", file);
        fclose(file);
    }
}

void MojLunaService::sendSignal(const MojChar* category, const MojChar* method, const MojChar* signalBody)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojLunaErr lserr;
    std::string uri;
    uri = "luna://";
    uri += m_name.data();
    uri += category;
    uri += method;

    bool send;
    send = LSSignalSend(getHandle(), uri.c_str(), signalBody, lserr);
    if (!send) {
        LOG_INFO(MSGID_MESSAGE_CALL, 0, "Failed to send reset signal : %s", lserr.message());
    }
}
