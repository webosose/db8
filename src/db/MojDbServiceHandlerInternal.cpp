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

#include <unicode/uloc.h>

#include "db/MojDbServiceHandler.h"
#include "db/MojDbServiceDefs.h"
#include "db/MojDbServiceHandlerInternal.h"
#include "db/MojDbReq.h"
#include "core/MojServiceRequest.h"
#include "core/MojTime.h"


const MojDbServiceHandlerInternal::SchemaMethod MojDbServiceHandlerInternal::s_methods[] = {
        {MojDbServiceDefs::PostBackupMethod, (Callback) &MojDbServiceHandlerInternal::handlePostBackup, MojDbServiceHandlerInternal::PostBackupSchema},
        {MojDbServiceDefs::PostRestoreMethod, (Callback) &MojDbServiceHandlerInternal::handlePostRestore, MojDbServiceHandlerInternal::PostRestoreSchema},
        {MojDbServiceDefs::PreBackupMethod, (Callback) &MojDbServiceHandlerInternal::handlePreBackup, MojDbServiceHandlerInternal::PreBackupSchema},
        {MojDbServiceDefs::PreRestoreMethod, (Callback) &MojDbServiceHandlerInternal::handlePreRestore,MojDbServiceHandlerInternal::PreRestoreSchema},
        {MojDbServiceDefs::ScheduledPurgeMethod, (Callback) &MojDbServiceHandlerInternal::handleScheduledPurge, MojDbServiceHandlerInternal::ScheduledPurgeSchema},
        {MojDbServiceDefs::SpaceCheckMethod, (Callback) &MojDbServiceHandlerInternal::handleSpaceCheck, MojDbServiceHandlerInternal::SpaceCheckSchema},
        {MojDbServiceDefs::ScheduledSpaceCheckMethod, (Callback) &MojDbServiceHandlerInternal::handleScheduledSpaceCheck, MojDbServiceHandlerInternal::ScheduledSpaceCheckSchema},
        {MojDbServiceDefs::ReleaseAdminProfileMethod, (Callback) &MojDbServiceHandlerInternal::handleReleaseAdminProfile, MojDbServiceHandlerInternal::ReleaseAdminProfileSchema},
        {NULL, NULL, NULL} };

const MojChar* const MojDbServiceHandlerInternal::PostBackupMethod = _T("internal/postBackup");
const MojChar* const MojDbServiceHandlerInternal::PostRestoreMethod = _T("internal/postRestore");
const MojChar* const MojDbServiceHandlerInternal::PreBackupMethod = _T("internal/preBackup");
const MojChar* const MojDbServiceHandlerInternal::PreRestoreMethod = _T("internal/preRestore");
const MojChar* const MojDbServiceHandlerInternal::ScheduledPurgeMethod = _T("internal/scheduledPurge");
const MojChar* const MojDbServiceHandlerInternal::SpaceCheckMethod = _T("internal/spaceCheck");
const MojChar* const MojDbServiceHandlerInternal::ScheduledSpaceCheckMethod = _T("internal/scheduledSpaceCheck");
const MojChar* const MojDbServiceHandlerInternal::ReleaseAdminProfileMethod = _T("internal/releaseAdminProfile");

MojDbServiceHandlerInternal::MojDbServiceHandlerInternal(MojDb& db, MojReactor& reactor, MojService& service)
: MojDbServiceHandlerBase(db, reactor),
  m_service(service)
{
}

MojErr MojDbServiceHandlerInternal::open()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = addMethods(s_methods);
	MojErrCheck(err);

    if (m_db.shardEngine()->enabled()) {
        m_mediaChangeHandler = new MojDbMediaHandler(m_db);
        MojAssert(m_mediaChangeHandler.get());
    }

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::subscribe()
{
    if (m_db.shardEngine()->enabled()) {
        MojAssert(m_mediaChangeHandler.get());

        MojErr err = m_mediaChangeHandler->subscribe(m_service);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::close()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    if (m_db.shardEngine()->enabled()) {
        MojAssert(m_mediaChangeHandler.get());

        err = m_mediaChangeHandler->close();
        MojErrCheck(err);
    }

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::configure(const MojObject& conf, bool fatalError)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
	if (fatalError) {
		err = generateFatalAlert();
		MojErrCheck(err);
	} else {
		err = requestLocale();
		MojErrCheck(err);
	}

	if (m_db.shardEngine()->enabled()) {
        MojAssert(m_mediaChangeHandler.get());

        err = m_mediaChangeHandler->configure(conf);
        MojErrCheck(err);
    }

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handlePostBackup(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// Chance to clean up anything we might have done during backup.
	// Don't need to delete the backup file - BackupService handles that
	MojErr err = msg->replySuccess();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handlePostRestore(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojString dir;
	MojErr err = payload.getRequired(MojDbServiceDefs::DirKey, dir);
	MojErrCheck(err);

	MojObject files;
	err = payload.getRequired(MojDbServiceDefs::FilesKey, files);
	MojErrCheck(err);

	// load each file, in order
	for (MojObject::ConstArrayIterator i = files.arrayBegin(); i != files.arrayEnd(); ++i) {
		MojUInt32 count = 0;
		MojString fileName;
		err = i->stringValue(fileName);
		MojErrCheck(err);
		MojString path;
		err = path.format(_T("%s/%s"), dir.data(), fileName.data());
		MojErrCheck(err);
		err = m_db.load(path, count, MojDbFlagForce, req);
		MojErrCheck(err);
	}

	err = msg->replySuccess();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handlePreBackup(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojErr err;
	MojString dir;
	err = payload.getRequired(MojDbServiceDefs::DirKey, dir);
	MojErrCheck(err);
	MojObject dumpResponse;

	MojObject backupParams;
	MojUInt32 bytes = 0;
	err = payload.getRequired(MojDbServiceDefs::BytesKey, bytes);
	MojErrCheck(err);
	MojObject incrementalKey;
	(void) payload.get(MojDbServiceDefs::IncrementalKey, incrementalKey);

	MojTime curTime;
	err = MojGetCurrentTime(curTime);
	MojErrCheck(err);
	MojString backupFileName;
	err = backupFileName.format(_T("%s-%llu.%s"), _T("backup"), curTime.microsecs(), _T("json"));
	MojErrCheck(err);

	MojUInt32 count = 0;
	MojString backupPath;
	err = backupPath.format(_T("%s/%s"), dir.data(), backupFileName.data());
	MojErrCheck(err);
	err = m_db.dump(backupPath, count, true, req, true, bytes, incrementalKey.empty() ? NULL : &incrementalKey, &dumpResponse);
	MojErrCheck(err);

	err = dumpResponse.put(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);
	MojObject files(MojObject::TypeArray);
	if (count > 0) {
		err = files.pushString(backupFileName);
		MojErrCheck(err);
	}
	err = dumpResponse.put(MojDbServiceDefs::FilesKey, files);
	MojErrCheck(err);
	err = msg->reply(dumpResponse);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handlePreRestore(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojString version;
	bool found = false;
	bool proceed = true;
	MojErr err = payload.get(MojDbServiceDefs::VersionKey, version, found);
	MojErrCheck(err);
	if (found) {
		MojObject versionObj;
		err = versionObj.fromJson(version);
		MojErrCheck(err);
		if (versionObj > m_db.version()) {
			proceed = false;
		}
	}

	MojObject response;
	err = response.put(MojDbServiceDefs::ProceedKey, proceed);
	MojErrCheck(err);
	err = response.put(MojServiceMessage::ReturnValueKey, true);
	MojErrCheck(err);
	err = msg->reply(response);
	MojErrCheck(err);
	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handleScheduledPurge(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojObject activity;
	MojErr err = payload.getRequired(_T("$activity"), activity);
	MojErrCheck(err);
	MojObject activityId;
	err = activity.getRequired(_T("activityId"), activityId);
	MojErrCheck(err);

	MojRefCountedPtr<PurgeHandler> handler(new PurgeHandler(this, activityId));
	MojAllocCheck(handler.get());
	err = handler->init();
	MojErrCheck(err);
	err = msg->replySuccess();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::handleScheduledSpaceCheck(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojObject activity;
	MojErr err = payload.getRequired(_T("$activity"), activity);
	MojErrCheck(err);
	MojObject activityId;
	err = activity.getRequired(_T("activityId"), activityId);
	MojErrCheck(err);

	MojRefCountedPtr<PurgeHandler> handler(new PurgeHandler(this, activityId, true));
	MojAllocCheck(handler.get());
	err = handler->init();
	MojErrCheck(err);
	err = msg->replySuccess();
	MojErrCheck(err);

	return MojErrNone;
}
MojErr MojDbServiceHandlerInternal::handleReleaseAdminProfile(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);
	MojErr err;
	bool isAppFound = false;
	MojString application;
	err = payload.get(MojDbServiceDefs::ProfileApplicationKey, application ,isAppFound);
	MojErrCheck(err);
	if (!isAppFound) {
		err = application.assign(msg->senderName());
		MojErrCheck(err);
	}
	err = m_db.releaseAdminProfile(application.data(), req);
	MojErrCheck(err);

	err = msg->replySuccess();
	MojErrCheck(err);

	return MojErrNone;

}
MojErr MojDbServiceHandlerInternal::handleSpaceCheck(MojServiceMessage* msg, MojObject& payload, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(msg);

	MojErr err = MojErrNone;
    MojDbSpaceAlert& spaceAlert = m_db.getSpaceAlert();

	MojObject response;
    MojDbSpaceAlert::AlertLevel alertLevel;
	MojInt64 bytesUsed;
	MojInt64 bytesAvailable;
	bool subscribed = msg->subscribed();

	if (subscribed) {
        spaceAlert.subscribe(msg);
	}

	err = spaceAlert.doSpaceCheck(alertLevel, bytesUsed, bytesAvailable);
	MojErrCheck(err);

    err = response.putString(_T("severity"), spaceAlert.getAlertName(alertLevel));
	MojErrCheck(err);
	err = response.putInt(_T("bytesUsed"), bytesUsed);
	MojErrCheck(err);
	err = response.putInt(_T("bytesAvailable"), bytesAvailable);
	MojErrCheck(err);
    err = response.putString(_T("path"), spaceAlert.getDatabaseRoot());
    MojErrCheck(err);
	err = response.putBool("subscribed", subscribed);
	MojErrCheck(err);
	err = msg->reply(response);
	MojErrCheck(err);

	return err;
}


MojErr MojDbServiceHandlerInternal::requestLocale()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// register with the settings service for locale changes
	MojString keys;
	(void) keys.assign(MojDbServiceDefs::LocaleKey);
	MojObject localePayload;
	MojErr err = localePayload.put(MojDbServiceDefs::KeysKey, keys);
	MojErrCheck(err);
	err = localePayload.put(MojDbServiceDefs::SubscribeKey, true);
	MojErrCheck(err);

	MojRefCountedPtr<MojServiceRequest> localeReq;
	err = m_service.createRequest(localeReq);
	MojErrCheck(err);
	m_localeChangeHandler.reset(new MojDbServiceHandlerInternal::LocaleHandler(m_db));
	MojAllocCheck(m_localeChangeHandler.get());
	err = localeReq->send(m_localeChangeHandler->m_slot, _T("com.webos.settingsservice"), _T("getSystemSettings"), localePayload, MojServiceRequest::Unlimited);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::generateFatalAlert()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject message;
	MojErr err = message.putString(_T("errorType"), _T("fsckError"));
	MojErrCheck(err);
	err = message.putBool(_T("returnValue"), true);
	MojErrCheck(err);
	err = generateAlert(_T("showTokenError"), message);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::generateAlert(const MojChar* event, const MojObject& msg)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject payload;
	MojErr err = payload.putString(_T("event"), event);
	MojErrCheck(err);
	err = payload.put(_T("message"), msg);
	MojErrCheck(err);

	MojRefCountedPtr<AlertHandler> handler(new AlertHandler(m_service, payload));
	MojAllocCheck(handler.get());
	err = handler->send();
	MojErrCheck(err);

	return MojErrNone;
}

MojDbServiceHandlerInternal::PurgeHandler::PurgeHandler(MojDbServiceHandlerInternal* serviceHandler, const MojObject& activityId, bool doSpaceCheck)
: m_handled(false),
  m_doSpaceCheck(doSpaceCheck),
  m_activityId(activityId),
  m_serviceHandler(serviceHandler),
  m_adoptSlot(this, &PurgeHandler::handleAdopt),
  m_completeSlot(this, &PurgeHandler::handleComplete)
{
}

MojErr MojDbServiceHandlerInternal::PurgeHandler::init()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject payload;
	MojErr err = initPayload(payload);
	MojErrCheck(err);
	err = payload.putBool(_T("wait"), true);
	MojErrCheck(err);
	err = payload.putBool(_T("subscribe"), true);
	MojErrCheck(err);
	err = m_serviceHandler->m_service.createRequest(m_subscription);
	MojErrCheck(err);
	err = m_subscription->send(m_adoptSlot, _T("com.palm.activitymanager"), _T("adopt"), payload, MojServiceRequest::Unlimited);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::PurgeHandler::handleAdopt(MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErrCheck(errCode);
	 MojErr err;

	// do the purge and compact
	if (!m_handled) {
		m_handled = true;
        if( m_doSpaceCheck )
        {
           LOG_DEBUG("Do space check");
           err = m_serviceHandler->m_db.getSpaceAlert().doSpaceCheck();
           MojErrCheck(err);
        }
        else
        {
           MojUInt32 count = 0;
           err = m_serviceHandler->m_db.purge(count);
           MojErrCheck(err);
           err = m_serviceHandler->m_db.compact();
           MojErrCheck(err);
        }

		// complete the activity
		MojObject requestPayload;
		err = initPayload(requestPayload);
		MojErrCheck(err);
		err = requestPayload.putBool(_T("restart"), true);
		MojErrCheck(err);
		MojRefCountedPtr<MojServiceRequest> req;
		err = m_serviceHandler->m_service.createRequest(req);
		MojErrCheck(err);
		err = req->send(m_completeSlot, _T("com.palm.activitymanager"), _T("complete"), requestPayload);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::PurgeHandler::handleComplete(MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// cancel subscription
	m_adoptSlot.cancel();
	if (errCode != MojErrNone) {
        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 1,
       		PMLOGKFV("error", "%d", errCode),
       		"error completing activity: 'error'");
		MojErrThrow(errCode);
	}
	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::PurgeHandler::initPayload(MojObject& payload)
{
	MojErr err = payload.put(_T("activityId"), m_activityId);
	MojErrCheck(err);
	err = payload.putString(_T("activityName"), _T("mojodb scheduled purge"));
	MojErrCheck(err);

	return MojErrNone;
}

MojDbServiceHandlerInternal::LocaleHandler::LocaleHandler(MojDb& db)
:  m_db(db),
   m_slot(this, &LocaleHandler::handleResponse)
{
}

MojErr MojDbServiceHandlerInternal::LocaleHandler::handleResponse(MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (errCode != MojErrNone) {
        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 1,
		PMLOGKFV("error", "%d", errCode),
		"error from setting service, locale query: 'error'");
		MojErrThrow(errCode);
	}

    MojObject settings;

    if (payload.get(_T("settings"), settings)) {
        MojObject localeInfo;
        MojObject locales;
        MojString localeStr;
        MojString updateLocale;

        MojErr err = settings.getRequired(_T("localeInfo"), localeInfo);
        MojErrCheck(err);
        err = localeInfo.getRequired(_T("locales"), locales);
        MojErrCheck(err);
        err = locales.getRequired(_T("UI"), localeStr);
        MojErrCheck(err);

        MojVector<MojString> localeBuf;
        err = localeStr.split('-', localeBuf);
        MojErrCheck(err);

        // Replace "-" to "_" for ICU library locale str format
        // ex> en-US ==> en_US
        // Since, chinese locale string has 3 parts, we need follow iteration
        // ex> zh-Hans-CN ==> zh_Hans_CN
        auto it = localeBuf.begin();
        updateLocale.assign(*it++);
        for ( ; it != localeBuf.end(); ++it) {
            updateLocale.append("_");
            updateLocale.append(*it);
        }

        // Originally when locale string is "zh_CN" then we added collation 'pinyin'
        // In CHN / TWN / HKG, they use the Chineses, but we cannot ensure
        // if country is TWN or HKG, then adding 'pinyin' collation is neccessary
        // Check it lately in TWN and HKG if such collation is needed
        if (updateLocale == "zh_Hans_CN")
            updateLocale.append("@collation=pinyin");

        err = m_db.updateLocale(updateLocale);
        MojErrCheck(err);

        UErrorCode errorU = U_ZERO_ERROR;
        uloc_setDefault(updateLocale.data(), &errorU);

        if (U_FAILURE(errorU)) {
            LOG_WARNING(MSGID_MOJ_DB_SERVICE_WARNING, 1,
                    PMLOGKS("locale", updateLocale.data()),
                    "MojDbServiceHandlerInternal::LocaleHandler::handleResponse. Can't set locale to 'locale'");
        }
    }

    return MojErrNone;
}

MojDbServiceHandlerInternal::AlertHandler::AlertHandler(MojService& service, const MojObject& payload)
: m_bootStatusSlot(this, &AlertHandler::handleBootStatusResponse),
  m_alertSlot(this, &AlertHandler::handleAlertResponse),
  m_service(service),
  m_payload(payload)
{
}

MojErr MojDbServiceHandlerInternal::AlertHandler::send()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject payload;
	MojErr err = payload.put(_T("subscribe"), true);
	MojErrCheck(err);
	err = m_service.createRequest(m_subscription);
	MojErrCheck(err);
	err = m_subscription->send(m_bootStatusSlot, _T("com.palm.systemmanager"), _T("getBootStatus"), payload, MojServiceRequest::Unlimited);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::AlertHandler::handleBootStatusResponse(MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (errCode != MojErrNone) {
        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 1,
            PMLOGKFV("error", "%d", errCode),
            "error attempting to get sysmgr boot status 'error'");
		MojErrThrow(errCode);
	}

	bool finished = false;
	MojErr err = payload.getRequired(_T("finished"), finished);
	MojErrCheck(err);
	if (finished) {
		m_subscription.reset();
		MojRefCountedPtr<MojServiceRequest> req;
		err = m_service.createRequest(req);
		MojErrCheck(err);
		err = req->send(m_alertSlot, _T("com.palm.systemmanager"), _T("publishToSystemUI"), m_payload);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbServiceHandlerInternal::AlertHandler::handleAlertResponse(MojObject& payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (errCode != MojErrNone) {
        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 1,
            PMLOGKFV("error", "%d", errCode),
            "error attempting to display alert: 'error'");
		MojErrThrow(errCode);
	}
	return MojErrNone;
}
