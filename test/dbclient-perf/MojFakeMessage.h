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

#ifndef MOJFAKEMESSAGE_H
#define MOJFAKEMESSAGE_H

#include <luna/MojLunaMessage.h> /* MojLunaMessage */

struct LSMessage;

class MojFakeMessage : public MojLunaMessage
{

public:
    MojFakeMessage(MojService* service, LSMessage* msg);
    virtual ~MojFakeMessage();
    virtual const MojChar* appId() const;
    virtual const MojChar* category() const;
    virtual const MojChar* method() const;
    virtual const MojChar* senderId() const;
    virtual const MojChar* senderAddress() const;
    virtual MojErr replyImpl();
    virtual Token token() const;
    MojErr payload(MojObjectVisitor& visitor) const;

private:
    Token m_token;
    LSMessage *m_msg;
};

#endif //MOJFAKEMESSAGE_H
