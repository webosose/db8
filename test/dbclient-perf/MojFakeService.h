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

#ifndef MOJFAKESERVICE_H
#define MOJFAKESERVICE_H

#include "core/MojService.h"

class MojFakeService : public MojService
{
public:
    MojFakeService();
    ~MojFakeService() override;
    virtual MojErr attach(GMainLoop *loop);
    virtual MojErr open(const MojChar* serviceName);
    virtual MojErr close();
    virtual MojErr addCategory(const MojChar* name, CategoryHandler* handler);
    virtual MojErr createRequest(MojRefCountedPtr<MojServiceRequest>& reqOut);
    virtual MojErr dispatch(); // for test purposes only

    virtual MojErr sendImpl(MojServiceRequest* req, const MojChar* service, const MojChar* method, Token& tokenOut);
    virtual MojErr cancelImpl(MojServiceRequest* req);
    virtual MojErr dispatchReplyImpl(MojServiceRequest* req, MojServiceMessage *msg, MojObject& payload, MojErr errCode);
    virtual MojErr enableSubscriptionImpl(MojServiceMessage* msg);
    virtual MojErr removeSubscriptionImpl(MojServiceMessage* msg);
};

#endif // MOJFAKESERVICE_H
