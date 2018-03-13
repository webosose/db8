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
#include "db/MojDbDefs.h"
#include "db/MojDbServiceClient.h"

#include "Runner.h"

#include "gtest/gtest.h"

class MojDbClientTestResultHandler : public MojSignalHandler
{
public:
    MojDbClientTestResultHandler()
    : m_dbErr(MojErrNone),
      m_callbackInvoked (false),
      m_slot(this, &MojDbClientTestResultHandler::handleResult)
    {
    }

    virtual MojErr handleResult(MojObject& result, MojErr errCode)
    {
        m_callbackInvoked = true;
        m_result = result;
        m_dbErr = errCode;
        if (errCode != MojErrNone) {
            bool found = false;
            MojErr err = result.get(MojServiceMessage::ErrorTextKey, m_errTxt, found);
            MojExpectNoErr(err);
            MojAssert(found);
        }
        return MojErrNone;
    }

    void reset()
    {
        m_dbErr = MojErrNone;
        m_callbackInvoked = false;
        m_errTxt.clear();
        m_result.clear();
    }

    MojErr wait(MojService* service)
    {
        while (!m_callbackInvoked) {
            MojErr err = service->dispatch();
            MojExpectNoErr(err);
        }
        return MojErrNone;
    }

    MojErr m_dbErr;
    MojString m_errTxt;
    MojObject m_result;
    bool m_callbackInvoked;
    MojDbClient::Signal::Slot<MojDbClientTestResultHandler> m_slot;
};
#endif
