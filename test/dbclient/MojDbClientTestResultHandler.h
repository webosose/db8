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

#ifndef MOJDBCLIENTTESTRESULTHANDLER_H
#define MOJDBCLIENTTESTRESULTHANDLER_H

#include "db/MojDbServiceHandler.h"
#include "db/MojDbServiceClient.h"
#include "Utils.h"


class MojDbClientTestResultHandler : public MojSignalHandler
{
public:
        MojDbClientTestResultHandler();
        virtual MojErr handleResult(MojObject& result, MojErr errCode);
        void reset();
        MojErr wait(MojService* service);
        MojErr database_error() const;
        MojDbClient::Signal::SlotRef slot();
        MojObject& result();
        MojObject m_result;

protected:
        bool m_callbackInvoked;

private:
        MojErr m_dbErr;
        MojString m_errTxt;
        MojDbClient::Signal::Slot<MojDbClientTestResultHandler> m_slot;
};
#endif
