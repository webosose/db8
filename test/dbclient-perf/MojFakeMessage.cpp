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

#include "MojFakeMessage.h"
#include "FakeLSMessage.h"

MojFakeMessage::MojFakeMessage(MojService *service, LSMessage *msg):
    MojLunaMessage(service, msg),
    m_token(LSMESSAGE_TOKEN_INVALID),
    m_msg(msg)
{
}

MojFakeMessage::~MojFakeMessage()
{
}

const MojChar *MojFakeMessage::appId() const
{
    return "com.palm.mojfakeluna";
}

const MojChar *MojFakeMessage::category() const
{
    return m_msg->category;
}

const MojChar *MojFakeMessage::senderId() const
{
    return "com.palm.mojfakeluna";
}

const MojChar *MojFakeMessage::senderAddress() const
{
    return "com.palm.mojfakeluna";
}

MojErr MojFakeMessage::replyImpl()
{
    return MojErrNone;
}

MojErr MojFakeMessage::payload(MojObjectVisitor &visitor) const
{
    MojJsonWriter *pwriter = (MojJsonWriter *) &((MojLunaMessage *)this)->writer();
    MojString s = pwriter->json();
    if (s.empty())
        s.assign("{}");
    MojErr err = MojJsonParser::parse(visitor, s);
    MojErrCheck(err);

    return MojErrNone;
}

MojServiceMessage::Token MojFakeMessage::token() const
{
    return m_token;
}

const MojChar *MojFakeMessage::method() const
{
    return "";
}
