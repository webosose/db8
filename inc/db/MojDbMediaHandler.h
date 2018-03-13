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

#ifndef MOJDBMEDIAHANDLER_H
#define MOJDBMEDIAHANDLER_H

#include "core/MojSignal.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "db/MojDbDefs.h"
#include "db/MojDbShardInfo.h"

#include <map>
#include <set>
#include <mutex>

class MojDbMediaHandler : public MojSignalHandler
{
    typedef std::map<MojString, MojDbShardInfo> shard_cache_t;

    typedef MojServiceRequest::ReplySignal::Slot<MojDbMediaHandler> Slot;
    typedef MojRefCountedPtr<MojServiceRequest> ServiceRequest;

    struct LunaRequest
    {
        LunaRequest(MojDbMediaHandler *parentClass, Slot::MethodType method);

        MojErr subscribe(MojService *service, const MojChar *serviceName, const MojChar *methodName);
        MojErr subscribe(MojService *service, const MojChar *serviceName, const MojChar *methodName, MojObject *payload);
        MojErr send(MojService *service, const MojChar *serviceName, const MojChar *methodName, MojObject *payload, MojUInt32 numReplies = 1);

        Slot m_slot;
        ServiceRequest m_subscription;
    };

public:
    MojDbMediaHandler(MojDb &db);

    MojErr configure(const MojObject& configure);
    MojErr subscribe(MojService &service);
    MojErr close();

    MojErr handleDeviceListResponse(MojObject &payload, MojErr errCode);
private:
    MojErr sendRelease(const MojString &deviceId, const MojString &subDeviceId, MojErr errCode);
    MojErr sendUnRegister();

    MojErr handleRegister(MojObject &payload, MojErr errCode);
    MojErr handleUnRegister(MojObject &payload, MojErr errCode);
    MojErr handleRelease(MojObject &payload, MojErr errCode);

    MojErr cbDeviceListResponse(MojObject &payload, MojErr errCode);

    MojErr convert(const MojObject &object, MojDbShardInfo &shardInfo);
    bool existInCache(const MojString &id);
    void copyShardCache(std::set<MojString> *);

    LunaRequest m_deviceList;
    LunaRequest m_deviceRegister;
    LunaRequest m_deviceRelease;
    LunaRequest m_deviceUnregister;

    MojString m_serviceName;
    MojService *m_service;
    MojDb &m_db;

    shard_cache_t m_shardCache;
    std::mutex m_lock;
};

#endif
