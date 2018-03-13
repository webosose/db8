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

#include "MojDbClientTestResultHandler.h"


MojDbClientTestResultHandler::MojDbClientTestResultHandler()
    : m_callbackInvoked (false),
      m_dbErr(MojErrNone),
      m_slot(this, &MojDbClientTestResultHandler::handleResult)
{
}

MojErr MojDbClientTestResultHandler::handleResult(MojObject &result, MojErr errCode)
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

void MojDbClientTestResultHandler::reset()
{
    m_dbErr = MojErrNone;
    m_callbackInvoked = false;
    m_errTxt.clear();
    m_result.clear();
}

MojErr MojDbClientTestResultHandler::wait(MojService *service)
{
    while (!m_callbackInvoked) {
        MojErr err = service->dispatch();
        MojErrCheck(err);
    }
    return MojErrNone;
}

MojErr MojDbClientTestResultHandler::database_error() const
{
    return m_dbErr;
}

MojDbClient::Signal::SlotRef MojDbClientTestResultHandler::slot()
{
    return m_slot;
}

MojObject& MojDbClientTestResultHandler::result()
{
    return m_result;
}
