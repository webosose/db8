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


#ifndef MOJDBSERVICEHANDLERBASE_H_
#define MOJDBSERVICEHANDLERBASE_H_

#include "db/MojDbDefs.h"
#include "db/MojDb.h"
#include "core/MojService.h"
#include "core/MojServiceMessage.h"

class MojDbServiceHandlerBase : public MojService::CategoryHandler
{
public:
	static const MojUInt32 DeadlockSleepMillis = 20;
	static const MojUInt32 MaxDeadlockRetries = 20;
	static const MojUInt32 MaxIndexlockRetries = 5;
	static const MojUInt32 MinIndexlockRetries = 3;
	static const MojUInt32 MaxBatchRetries = 1000;
    static const MojChar* ProfileKind;

	typedef MojErr (CategoryHandler::* DbCallback)(MojServiceMessage* msg, const MojObject& payload, MojDbReq& req);

	MojDbServiceHandlerBase(MojDb& db, MojReactor& reactor);

	virtual MojErr open() = 0;
	virtual MojErr close() = 0;

protected:
	MojErr invoke(Callback method, MojServiceMessage* msg, MojObject& payload) override;

	MojErr invokeImpl(Callback method, MojServiceMessage* msg, MojObject& payload);
	MojErr invokeCallback(Callback method, MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	MojErr invokeCallbackProfiled(Callback method, MojServiceMessage* msg, MojObject& payload, MojDbReq& req);
	static MojErr formatCount(MojServiceMessage* msg, MojUInt32 count);
	static MojErr formatPut(MojServiceMessage* msg, const MojObject* begin, const MojObject* end);
	static MojErr formatPutAppend(MojObjectVisitor& writer, const MojObject& result);

	MojDb& m_db;
	MojReactor& m_reactor;
};

#endif /* MOJDBSERVICEHANDLERBASE_H_ */
