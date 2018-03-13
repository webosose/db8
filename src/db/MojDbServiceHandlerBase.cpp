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


#include "db/MojDbServiceHandlerBase.h"
#include "db/MojDbServiceDefs.h"
#include "core/MojReactor.h"
#include "core/MojTime.h"
#include "core/MojJson.h"

MojDbServiceHandlerBase::MojDbServiceHandlerBase(MojDb& db, MojReactor& reactor)
: m_db(db),
  m_reactor(reactor)
{
}

MojErr MojDbServiceHandlerBase::invoke(Callback method, MojServiceMessage* msg, MojObject& payload)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(method && msg);

	MojUInt32 retries = 0;
	MojErr err;
    for (;;) {

		err = invokeImpl(method, msg, payload);

		MojErrCatch(err, MojErrDbFatal) {
			LOG_CRITICAL(MSGID_DB_SERVICE_ERROR, 0, "db: fatal error; shutting down");
			m_reactor.stop();
			MojErrThrow(MojErrDbFatal);
		}

        if (MojErrDbIO == err || MojErrDbQuotaExceeded == err) {
            MojInt64 bytesUsed = 0l;
            MojInt64 bytesAvailable = 1l;
            MojDbSpaceAlert::AlertLevel alertLevel = MojDbSpaceAlert::NoSpaceAlert;
            MojErr alertErr = m_db.getSpaceAlert().doSpaceCheck(alertLevel, bytesUsed, bytesAvailable);
            MojString serviceName;
            MojErr confReadErr =  m_db.getConf().getRequired("service_name", serviceName);
            MojErrCatchAll(confReadErr);

            // db8 register fault when I/O error occur on main db only
            if (serviceName.compare(MojDbServiceDefs::ServiceName) == 0 && MojErrDbQuotaExceeded != err) {
                LOG_CRITICAL(MSGID_DB_SERVICE_ERROR, 0, "db : I/O error occur on main db process; register fault to faultmanager");
                MojString faultRegisterScriptPath;
                MojErr err = m_db.getConf().getRequired("faultRegisterScriptPath", faultRegisterScriptPath);
                MojErrCatchAll(err);
                system(faultRegisterScriptPath.data());
            } else if (MojErrNone == alertErr && bytesAvailable == 0l && MojDbSpaceAlert::AlertLevelHigh == alertLevel) {
                LOG_CRITICAL(MSGID_DB_SERVICE_ERROR, 0, "db: IO error due to no space; shutting down");
                m_reactor.stop();
                MojErrThrow(err);
            }
        }
		MojErrCatch(err, MojErrDbDeadlock) {
			if (++retries >= MaxDeadlockRetries) {
                LOG_ERROR(MSGID_DB_SERVICE_ERROR, 0, "db: deadlock detected; max retries exceeded");
				MojErrThrow(MojErrDbMaxRetriesExceeded);
			}

			LOG_WARNING(MSGID_MOJ_DB_SERVICE_WARNING, 1,
					PMLOGKFV("retries", "%d", retries),
					"db: deadlock detected; attempting retry");
			err = msg->writer().reset();
			MojErrCheck(err);
			err = MojSleep(DeadlockSleepMillis * 1000);
			MojErrCheck(err);
			continue;
		}
		MojErrCatch(err, MojErrInternalIndexOnDel) {
			if (++retries >= MaxIndexlockRetries) {
                LOG_ERROR(MSGID_DB_SERVICE_ERROR, 0, "db: indexlock_warning max retries exceeded");
				MojErrThrow(MojErrDbInconsistentIndex);
			}

			LOG_WARNING(MSGID_MOJ_DB_SERVICE_WARNING, 1,
					PMLOGKFV("retries", "%d", retries),
					"db: indexlock_conflict; attempting retry");
			err = msg->writer().reset();
			MojErrCheck(err);
			err = MojSleep(DeadlockSleepMillis * 1000);
			MojErrCheck(err);
			continue;
		}
		MojErrCheck(err);
		break;
	}
	return MojErrNone;
}

MojErr MojDbServiceHandlerBase::invokeImpl(MojService::CategoryHandler::Callback method, MojServiceMessage* msg, MojObject& payload)
{
	MojErr err;

	MojDbReq req(false);

	err = req.domain(msg->senderName());
	MojErrCheck(err);

	req.beginBatch();

	if (!m_db.profileEngine()->isEnabled()) {
		err = invokeCallback(method, msg, payload, req);
		MojErrCheck(err);
	} else {
		err = invokeCallbackProfiled(method, msg, payload, req);
		MojErrCheck(err);
	}

	err = req.endBatch();
	MojErrCheck(err);

	return MojErrNone;
}


MojErr MojDbServiceHandlerBase::invokeCallbackProfiled(Callback method, MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err;
	bool profiling;
	MojDbReq profileReq(false);
	err = profileReq.domain(msg->senderName());
	MojErrCheck(err);
	err = m_db.profileEngine()->isProfiled(msg->senderName(), profileReq, &profiling);
	MojErrCheck(err);
	if (!profiling) {
		err = invokeCallback(method, msg, payload, req);
		MojErrCheck(err);
		return MojErrNone;
	}
	MojErr saveStatErr;
	MojString memInfoStr;
	MojDbProfileEngine::clock_t::duration duration;
	{
		MojDbReq lunaReq(false);
		err = lunaReq.domain(msg->senderName());
		MojErrCheck(err);
		lunaReq.beginBatch();
		auto start = MojDbProfileEngine::clock_t::now();
		err = invokeCallback(method, msg, payload, lunaReq);
		auto end = MojDbProfileEngine::clock_t::now();
		saveStatErr = m_db.profileEngine()->updateMemInfo(memInfoStr);
		MojErrCatchAll(saveStatErr);
		duration = end - start;
		lunaReq.endBatch();
	}
	saveStatErr = m_db.profileEngine()->saveStat(msg->senderName(), msg->category(), msg->method(), payload, duration, memInfoStr, profileReq);
	MojErrCatchAll(saveStatErr);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerBase::invokeCallback(Callback method, MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err;

	err = (this->*((DbCallback) method))(msg, payload, req);
	MojErrCheck(err);

#if MOJ_DEBUG
	MojString errStr;
	MojString payloadstr;
	(void) MojErrToString(err, errStr);
	(void) payload.toJson(payloadstr);
    LOG_DEBUG("[db_mojodb] db_method: %s, err: (%d) - %s; sender= %s;\n payload=%s; \n response= %s\n",
        msg->method(), (int)err, errStr.data(), msg->senderName(), payloadstr.data(), ((MojJsonWriter&)(msg->writer())).json().data());
#endif


	return MojErrNone;
}

MojErr MojDbServiceHandlerBase::formatCount(MojServiceMessage* msg, MojUInt32 count)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojObjectVisitor& writer = msg->writer();
	MojErr err = writer.beginObject();
	MojErrCheck(err);
	err = writer.boolProp(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);
	err = writer.intProp(MojDbServiceDefs::CountKey, count);
	MojErrCheck(err);
	err = writer.endObject();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerBase::formatPut(MojServiceMessage* msg, const MojObject* begin, const MojObject* end)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojObjectVisitor& writer = msg->writer();
	MojErr err = writer.beginObject();
	MojErrCheck(err);
	err = writer.boolProp(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);
	err = writer.propName(MojDbServiceDefs::ResultsKey);
	MojErrCheck(err);
	err = writer.beginArray();
	MojErrCheck(err);

	for (const MojObject* i = begin; i != end;  ++i) {
		bool ignoreId = false;
		if (i->get(MojDb::IgnoreIdKey, ignoreId) && ignoreId)
			continue;
		err = formatPutAppend(writer, *i);
		MojErrCheck(err);
	}
	err = writer.endArray();
	MojErrCheck(err);
	err = writer.endObject();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerBase::formatPutAppend(MojObjectVisitor& writer, const MojObject& result)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject id;
	MojErr err = result.getRequired(MojDb::IdKey, id);
	MojErrCheck(err);
	MojObject rev;

	err = writer.beginObject();
	MojErrCheck(err);
	err = writer.objectProp(MojDbServiceDefs::IdKey, id);
	MojErrCheck(err);
	if (result.get(MojDb::RevKey, rev)) {
		err = writer.objectProp(MojDbServiceDefs::RevKey, rev);
		MojErrCheck(err);
	}
	err = writer.endObject();
	MojErrCheck(err);

	return MojErrNone;
}
