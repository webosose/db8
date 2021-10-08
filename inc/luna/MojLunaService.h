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


#ifndef MOJLUNASERVICE_H_
#define MOJLUNASERVICE_H_

#include <glib.h>
#include "luna/MojLunaDefs.h"
#include "core/MojService.h"
#include "core/MojString.h"
#include "core/MojLogDb8.h"

class MojLunaService : public MojService, public MojSignalHandler
{
public:
    typedef MojSignal<> IdleTimeoutSignal;
    MojLunaService(bool allowPublicMethods = false/*keep flag to avoid activitymanager compilation error*/, MojMessageDispatcher* queue = NULL);
	virtual ~MojLunaService();

	virtual MojErr open(const MojChar* serviceName);
	virtual MojErr close();
	virtual MojErr dispatch();
	virtual MojErr addCategory(const MojChar* name, CategoryHandler* handler);
	virtual MojErr addSignalCategory(const MojChar* name, LSSignal* signal);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const MojString& proxyRequester);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const char *proxyRequester);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut, const char *originExe, const char *originId, const char *originName);
	MojErr attach(GMainLoop* loop);
	void connectIdleTimeoutSignal(IdleTimeoutSignal::SlotRef slot);
	LSHandle* getService();
	void setIdleTimeout(MojUInt32 timeout);
	LSHandle* getHandle() { return m_service; }
	void sendSignal(const MojChar* category, const MojChar* method, const MojChar* signalBody);

private:
	friend class MojLunaMessage;
	typedef MojVector<LSMethod> MethodVec;

	class LunaCategory : public Category
	{
	public:
        LunaCategory(MojService* service, CategoryHandler* handler, const MethodVec& methods)
        : Category(service, handler)
        , m_methods(methods)
        {}

        MethodVec m_methods;
	};

	static const MojChar* const UriScheme;

	virtual MojErr sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut);
	virtual MojErr cancelImpl(MojServiceRequest* req);
	virtual MojErr dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode);
	virtual MojErr enableSubscriptionImpl(MojServiceMessage* msg);
	virtual MojErr removeSubscriptionImpl(MojServiceMessage* msg);

	static bool handleCancel(LSHandle* sh, LSMessage* msg, void* ctx);
	static bool handleRequest(LSHandle* sh, LSMessage* msg, void* ctx);
	static bool handleResponse(LSHandle* sh, LSMessage* msg, void* ctx);
	static void idleTimeoutCallback(void* userdata);

	void Statistic(LSMessage* msg);
	LSHandle* m_service;
	GMainLoop* m_loop;
	MojUInt32 m_idleTimeout;
	IdleTimeoutSignal m_idleTimeoutSignal;
};

#endif /* MOJLUNASERVICE_H_ */
