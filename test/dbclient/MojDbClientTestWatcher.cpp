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

#include "MojDbClientTestWatcher.h"

MojDbClientTestWatcher::MojDbClientTestWatcher()
    : m_fired(false)
{
}

MojErr MojDbClientTestWatcher::handleResult(MojObject &result, MojErr errCode)
{
    if (!m_callbackInvoked) {
        // results callback
        MojErr err = MojDbClientTestResultHandler::handleResult(result, errCode);
        MojErrCheck(err);
        // check for an optional fired field
        bool fired = false;
        bool found = result.get(MojDbServiceDefs::FiredKey, fired);
        if (found)
            m_fired = fired;
        return MojErrNone;
    } else {
        // watch fire
        MojString s;
        result.toJson(s);
        MojAssert(!m_fired);
        bool fired = false;
        MojErr err = result.getRequired(MojDbServiceDefs::FiredKey, fired);
        MojExpectNoErr(err);
        MojAssert(fired);
        m_fired = true;
        return MojErrNone;
    }
}

bool MojDbClientTestWatcher::fired() const
{
    return m_fired;
}
