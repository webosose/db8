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


#ifndef MOJSERVICE_H_
#define MOJSERVICE_H_

#include "core/MojCoreDefs.h"
#include "core/MojAutoPtr.h"
#include "core/MojHashMap.h"
#include "core/MojObject.h"
#include "core/MojSchema.h"
#include "core/MojSignal.h"
#include "core/MojString.h"

class MojService : private MojNoCopy
{
public:
	typedef MojUInt32 Token;
	static const MojChar* const DefaultCategory;

	class CategoryHandler : public MojSignalHandler
	{
	public:
		typedef MojErr (CategoryHandler::* Callback)(MojServiceMessage* msg, const MojObject& payload);

		virtual ~CategoryHandler() {}
		MojErr invoke(const MojChar* method, MojServiceMessage* msg, MojObject& payload);

	protected:
		friend class MojLunaService;

		struct Method {
			const MojChar* m_name;
			Callback m_callback;
			uint32_t m_ls2_method_flags;

			Method()
				: m_name(NULL)
				, m_callback(NULL)
				, m_ls2_method_flags(0)
			{
			}

			Method(const MojChar* name, Callback callback)
				: m_name(name)
				, m_callback(callback)
				, m_ls2_method_flags(0)
			{}

			Method(const MojChar* name, Callback callback, uint32_t _m_ls2_method_flags)
				: m_name(name)
				, m_callback(callback)
				, m_ls2_method_flags(_m_ls2_method_flags)
			{}
		};

		struct SchemaMethod {
			const MojChar* m_name;
			Callback m_callback;
			const MojChar* m_schemaJson;
		};

		struct CallbackInfo {
			CallbackInfo() : m_callback(NULL), m_ls2_method_flags(0) {}
			CallbackInfo(Callback cb, MojSchema* schema, uint32_t ls2_method_flags)
				: m_callback(cb)
				, m_schema(schema)
				, m_ls2_method_flags(ls2_method_flags)
			{}
			bool operator==(const CallbackInfo& rhs) const {
				return	m_callback == rhs.m_callback
						&& m_ls2_method_flags == rhs.m_ls2_method_flags;
			}

			Callback m_callback;
			MojRefCountedPtr<MojSchema> m_schema;
			uint32_t m_ls2_method_flags;
		};

		typedef MojHashMap<MojString, CallbackInfo, const MojChar*> CallbackMap;

		CategoryHandler() {}
		const CallbackMap& callbacks() const { return m_callbackMap; }
		MojErr addMethod(const MojChar* methodName, Callback callback, const MojChar* schemaJson = NULL, uint32_t ls2_method_flags = 0);
		MojErr addMethods(const Method* methods);
		MojErr addMethods(const SchemaMethod* methods);
		virtual MojErr invoke(Callback method, MojServiceMessage* msg, MojObject& payload);

        // Deprecated methods. Still used by FileCache
        MojErr addMethods(const Method* methods, bool)
            __attribute__((deprecated("No public/private bus any more. Use addMethods(const Method*) instead")));
        MojErr addMethods(const SchemaMethod* methods, bool)
            __attribute__((deprecated("No public/private bus any more. Use addMethods(const SchemaMethod*) instead")));

		CallbackMap m_callbackMap;
	};

	virtual ~MojService();
	virtual MojErr open(const MojChar* serviceName = NULL);
	virtual MojErr close();

	virtual MojErr addCategory(const MojChar* name, CategoryHandler* handler);
	virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut) = 0;
	virtual MojErr dispatch() = 0; // for test purposes only

protected:
	friend class MojServiceMessage;
	friend class MojServiceRequest;

	class SubscriptionKey : public MojObject
	{
	public:
		MojErr init(const MojServiceMessage* msg);
		MojErr init(const MojChar* sender);
		MojErr sender(MojString& tokenOut) const;
		Token token() const;
	};

	class Category : public MojRefCounted
	{
	public:
		Category(MojService* service, CategoryHandler* handler)
		: m_service(service), m_handler(handler) {}

		MojService* m_service;
		MojRefCountedPtr<CategoryHandler> m_handler;
	};

	typedef MojMap<SubscriptionKey, MojServiceMessage*> SubscriptionMap;
	typedef MojHashMap<Token, MojRefCountedPtr<MojServiceRequest> > RequestMap;
	typedef MojHashMap<MojString, MojRefCountedPtr<Category>, const MojChar*> CategoryMap;
	typedef MojErr (MojService::* DispatchMethod)(MojServiceMessage* msg);

	MojService(MojMessageDispatcher* queue = NULL);
	MojErr send(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut);
	MojErr cancel(MojServiceRequest* req);
	MojErr enableSubscription(MojServiceMessage* msg);

	MojErr handleRequest(MojServiceMessage* msg) { return handleMessage(&MojService::dispatchRequest, msg); }
	MojErr handleReply(MojServiceMessage* msg) { return handleMessage(&MojService::dispatchReply, msg); }
	MojErr handleCancel(MojServiceMessage* msg) { return handleMessage(&MojService::dispatchCancel, msg); }
	MojErr handleMessage(DispatchMethod method, MojServiceMessage* msg);

	MojErr dispatchRequest(MojServiceMessage* msg);
	MojErr dispatchReply(MojServiceMessage* msg);
	MojErr dispatchCancel(MojServiceMessage* msg);
	MojErr dispatchCancel(const SubscriptionKey& key);

	MojErr addRequest(MojServiceRequest* req);
	MojErr getRequest(MojServiceMessage* msg, MojRefCountedPtr<MojServiceRequest>& reqOut);
	MojErr removeRequest(MojServiceRequest* req, bool& foundOut);
	MojErr addSubscription(MojServiceMessage* msg);
	MojErr removeSubscription(MojServiceMessage* msg);
	MojErr getCategory(const MojChar* name, MojRefCountedPtr<Category>& catOut);

	virtual MojErr sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut) = 0;
	virtual MojErr cancelImpl(MojServiceRequest* req) = 0;
	virtual MojErr dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode) = 0;
	virtual MojErr enableSubscriptionImpl(MojServiceMessage* msg) = 0;
	virtual MojErr removeSubscriptionImpl(MojServiceMessage* msg) = 0;

	MojThreadMutex m_mutex;
	SubscriptionMap m_subscriptions;
	CategoryMap m_categories;
	RequestMap m_requests;
	MojString m_name;
	MojMessageDispatcher* m_dispatcher;
};

#endif /* MOJSERVICE_H_ */
