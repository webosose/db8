// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#include "db/MojDbMediaHandler.h"
#include "db/MojDbServiceDefs.h"
#include "db/MojDb.h"

#include <algorithm>

MojDbMediaHandler::LunaRequest::LunaRequest(MojDbMediaHandler *parentClass, Slot::MethodType method)
    : m_slot(parentClass, method)
{
}

MojErr MojDbMediaHandler::LunaRequest::subscribe(MojService *service, const MojChar *serviceName, const MojChar *methodName)
{
    MojErr err;
    MojObject payload;

    err = subscribe(service, serviceName, methodName, &payload);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::LunaRequest::subscribe(MojService *service, const MojChar *serviceName, const MojChar *methodName, MojObject *payload)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojAssert(service);
    MojAssert(serviceName);
    MojAssert(methodName);
    MojAssert(payload);

    MojErr err;

    err = payload->putBool(_T("subscribe"), true);
    MojErrCheck(err);

    err = service->createRequest(m_subscription);
    MojErrCheck(err);

    err = m_subscription->send(m_slot, serviceName, methodName, *payload, MojServiceRequest::Unlimited);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::LunaRequest::send(MojService *service, const MojChar *serviceName, const MojChar *methodName, MojObject *payload, MojUInt32 numReplies)
{
    MojAssert(service);
    MojAssert(serviceName);
    MojAssert(methodName);
    MojAssert(payload);

    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    err = service->createRequest(m_subscription);
    MojErrCheck(err);

    err = m_subscription->send(m_slot, serviceName, methodName, *payload, numReplies);
    MojErrCheck(err);

    return MojErrNone;
}

MojDbMediaHandler::MojDbMediaHandler(MojDb &db)
    : m_deviceList(this, &MojDbMediaHandler::cbDeviceListResponse),
      m_deviceRegister(this, &MojDbMediaHandler::handleRegister),
      m_deviceRelease(this, &MojDbMediaHandler::handleRelease),
      m_deviceUnregister(this, &MojDbMediaHandler::handleUnRegister),
      m_service(nullptr),
      m_db(db)
{
}

MojErr MojDbMediaHandler::configure(const MojObject& conf)
{
    MojErr err;

    MojObject dbConf;
    err = conf.getRequired("db", dbConf);
    MojErrCheck(err);

    err = dbConf.getRequired("service_name", m_serviceName);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::subscribe(MojService &service)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojAssert(!m_serviceName.empty());

    MojErr err;
    m_service = &service;

    err = sendUnRegister();
    MojErrCatchAll(err) {
        LOG_DEBUG("[db-luna_shard] sendUnRegister return error. Ignore it");
    }

    err = m_deviceList.subscribe(m_service, MojDbServiceDefs::ASMServiceName, MojDbServiceDefs::ASMListDeviceMethod);
    MojErrCheck(err);

    MojObject payload;
    err = payload.putString(_T("name"), m_serviceName);
    MojErrCheck(err);

    MojObject deviceTypes;
    err = deviceTypes.pushString(_T("usb"));
    MojErrCheck(err);

    err = payload.put(_T("deviceType"), deviceTypes);
    MojErrCheck(err);

    err = m_deviceRegister.subscribe(m_service, MojDbServiceDefs::ASMServiceName, MojDbServiceDefs::ASMRegisterDeviceStatusMethod, &payload);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::close()
{
    MojErr err;

    err = sendUnRegister();
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::handleDeviceListResponse(MojObject &payload, MojErr errCode)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    std::lock_guard<std::mutex>  guard(m_lock);

    MojErr err;
    if (errCode != MojErrNone) {
        MojString payloadStr;
        err = payload.stringValue(payloadStr);
        MojErrCheck(err);

        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 2,
                  PMLOGKFV("error", "%d", errCode),
                  PMLOGKS("payload", payloadStr.data()),
                  "error attempting to get list of devices");

        MojErrThrow(errCode);
    }

    MojErr accErr = MojErrNone;

    MojObject deviceList;
    err = payload.getRequired("devices", deviceList);
    MojErrCheck(err);

    std::set<MojString> shardIds;           // list of not processed ids of already know
    copyShardCache(&shardIds);

    for (MojObject::ConstArrayIterator i = deviceList.arrayBegin(); i != deviceList.arrayEnd(); ++i) {
        MojString deviceType;
        MojObject subDevices;

        err = i->getRequired("deviceType", deviceType);
        MojErrCheck(err);

        if (deviceType != "usb") {
            LOG_DEBUG("[db-luna_shard] Got from PDM device, but device not usb media. Ignore it");
            // not usb media. PDM returns ALL list of media, like USB, Internal storage and other.
            // db8 intresting only in usb sticks
            continue;
        }

        MojString parentDeviceId;
        err = i->getRequired(_T("deviceId"), parentDeviceId);
        MojErrCheck(err);

        err = i->getRequired("subDevices", subDevices);
        MojErrCheck(err);

        for (MojObject::ConstArrayIterator it = subDevices.arrayBegin(); it != subDevices.arrayEnd(); ++it) {
            MojDbShardInfo shardInfo;
            err = convert(*it, shardInfo);
            MojErrCheck(err);

            if (shardInfo.deviceId.empty()) {
                LOG_WARNING(MSGID_LUNA_SERVICE_WARNING, 0, "Device id is empty, ignore it");
                continue;
            }

            shardInfo.parentDeviceId = parentDeviceId;
            shardInfo.active = true;

            shardIds.erase(shardInfo.deviceId);  // mark it as processed

            if (!existInCache(shardInfo.deviceId)) {
                LOG_DEBUG("[db-luna_shard] Found new device %s. Add to device cache and send notification to shard engine", shardInfo.deviceId.data());

                m_shardCache[shardInfo.deviceId] = shardInfo;
                err = m_db.shardEngine()->processShardInfo(shardInfo);
                MojErrAccumulate(accErr, err);
            } else {
                LOG_DEBUG("[db-luna_shard] Device uuid cached, it looks like it doesn't changed");
            }
        } // end subDevices loop
    }   // end main list of devices loop

    // notify shard engine about all inactive shards
    for (std::set<MojString>::const_iterator i = shardIds.begin(); i != shardIds.end(); ++i) {
        shard_cache_t::iterator shardCacheIterator = m_shardCache.find(*i);
        if (shardCacheIterator != m_shardCache.end()) {
            LOG_DEBUG("[db-luna_shard] Device %s not found in cache. Notify shard engine that shard not active", shardCacheIterator->second.deviceId.data());

            shardCacheIterator->second.active = false;
            err = m_db.shardEngine()->processShardInfo(shardCacheIterator->second);
            MojErrAccumulate(accErr, err);
            m_shardCache.erase(shardCacheIterator);
        }
    }

    /*bool finished = false;
    MojErr err = payload.getRequired(_T("finished"), finished);
    MojErrCheck(err);
    if (finished) {
        m_subscription.reset();
        if (m_service) {
            MojRefCountedPtr<MojServiceRequest> req;
            err = m_service->createRequest(req);
            MojErrCheck(err);
            err = req->send(m_alertSlot, _T("com.palm.systemmanager"), _T("publishToSystemUI"), m_payload);
            MojErrCheck(err);
        }
    }*/

    // propagate accumulated error
    MojErrCheck(accErr);

    return MojErrNone;
}

MojErr MojDbMediaHandler::handleRegister(MojObject &payload, MojErr errCode)
{
    std::lock_guard<std::mutex>  guard(m_lock);

    MojErr err;

    MojString deviceId;
    MojString deviceStatus;

    bool found;

    err = payload.get(_T("deviceId"), deviceId, found);
    MojErrCheck(err);

    if (!found) {
        LOG_DEBUG("[db-luna_shard] No Payload, ignore call");

        return MojErrNone;
    }

    err = payload.getRequired(_T("status"), deviceStatus);
    MojErrCheck(err);

    bool isEjecting = (deviceStatus.compare(MojDbServiceDefs::ASMDeviceStatusEjecting) == 0);
    bool isFormatting = (deviceStatus.compare(MojDbServiceDefs::ASMDeviceStatusFormatting) == 0);

    if (!isEjecting && !isFormatting) {
        return MojErrNone;
    }

    MojErr errProcess = MojErrNone;
    std::list<MojString> erase;

    std::for_each(m_shardCache.begin(), m_shardCache.end(), [&] (shard_cache_t::reference ref) {
        MojDbShardInfo& shardInfo = ref.second;

        if (shardInfo.parentDeviceId == deviceId) {
            shardInfo.active = false;
            err = m_db.shardEngine()->processShardInfo(shardInfo);
            MojErrAccumulate(errProcess, err);

            erase.push_back(ref.first);
        }
    });

    std::for_each(erase.begin(), erase.end(), [&] (const MojString& uuid) {
        m_shardCache.erase(uuid);
    });

    // notify ASM about detaching
    MojString subDeviceId;
    err = sendRelease(deviceId, subDeviceId, err);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::sendRelease(const MojString &deviceId, const MojString &subDeviceId, MojErr errCode)
{
    MojAssert(!m_serviceName.empty());

    MojErr err;
    MojObject payload;

    err = payload.putString(_T("deviceId"), deviceId);
    MojErrCheck(err);

    /*err = payload.putString(_T("subDeviceId"), subDeviceId);
    MojErrCheck(err);*/

    err = payload.putString(_T("name"), m_serviceName);
    MojErrCheck(err);

    MojString errStr;

    if (errCode != MojErrNone) {
        err = MojErrToString(err, errStr);
        MojErrCheck(err);
    }

    err = payload.putString(_T("reason"), errStr);
    MojErrCheck(err);

    err = payload.putBool(_T("isUsing"), (errCode == MojErrNone) ? false : true);
    MojErrCheck(err);

    err = m_deviceRelease.send(m_service, MojDbServiceDefs::ASMServiceName, MojDbServiceDefs::ASMResponseDeviceStatusMethod, &payload);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::sendUnRegister()
{
    MojErr err;
    MojObject payload;

    MojAssert(!m_serviceName.empty());

    err = payload.putString(_T("name"), m_serviceName);
    MojErrCheck(err);

    err = m_deviceUnregister.send(m_service, MojDbServiceDefs::ASMServiceName, MojDbServiceDefs::ASMUnRegisterDeviceStatusMethod, &payload);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbMediaHandler::handleRelease(MojObject &payload, MojErr errCode)
{
#ifdef MOJ_DEBUG
    MojString str;
    MojErr err = payload.stringValue(str);
    MojErrCheck(err);

    LOG_DEBUG("[db-luna_shard] handle release response: ASM payload %s", str.data());
#endif
    return MojErrNone;
}

MojErr MojDbMediaHandler::handleUnRegister(MojObject &payload, MojErr errCode)
{
#ifdef MOJ_DEBUG
    MojString str;

    MojErr err = payload.stringValue(str);
    MojErrCheck(err);

    LOG_DEBUG("[db-luna_shard] handleUnRegister PAYLOAD %s", str.data());
#endif
    return MojErrNone;
}

MojErr MojDbMediaHandler::cbDeviceListResponse(MojObject &payload, MojErr errCode)
{
    MojErr err = handleDeviceListResponse(payload, errCode);
    if (err != MojErrNone) {
        MojString payloadStr;
        MojErr jsonErr = payload.stringValue(payloadStr);
        MojString desc;
        MojErr formatErr = MojErrToString(err, desc);
        LOG_ERROR(MSGID_DB_SERVICE_ERROR, 2,
                  PMLOGKFV("error", "%d", err),
                  PMLOGJSON("payload", jsonErr == MojErrNone ? payloadStr.data() : "\"(no-json)\""),
                  "error during handling list of devices: %s",
                  formatErr == MojErrNone ? desc.data() : "(no-repr)"
                 );
    }
    return MojErrNone;
}

MojErr MojDbMediaHandler::convert(const MojObject &object, MojDbShardInfo &shardInfo)
{
    MojErr err;
    err = object.getRequired("deviceId", shardInfo.deviceId);
    MojErrCheck(err);

    err = object.getRequired("deviceName", shardInfo.deviceName);
    MojErrCheck(err);

    err = object.getRequired("deviceUri", shardInfo.deviceUri);
    MojErrCheck(err);

    return MojErrNone;
}

bool MojDbMediaHandler::existInCache(const MojString &id)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    shard_cache_t::iterator i = m_shardCache.find(id);
    return (i != m_shardCache.end());
}

void MojDbMediaHandler::copyShardCache(std::set<MojString> *shardIdSet)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    for (shard_cache_t::const_iterator i = m_shardCache.begin(); i != m_shardCache.end(); ++i) {
        shardIdSet->insert(i->first);
    }
}
