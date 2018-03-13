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

#include "DbGenerator.h"
#include "Runner.h"

extern const MojChar * const g_moj_test_kind;
extern const char *g_temp_folder;

MojErr generateDatabase(size_t count, const char * outputFile)
{
    MojDb db;
    MojErr err;
    MojUInt32 countOut;

    err = db.open(g_temp_folder);
    MojErrCheck(err);

    // add kind
    MojObject kindObj;
    err = kindObj.fromJson(g_moj_test_kind);
    MojErrCheck(err);
    err = db.putKind(kindObj);
    MojErrCheck(err);

    // put test objects
    for (MojSize i = 0; i < count; ++i) {
        MojObject obj;
        MojString entry;
        entry.format(_T("{\"_id\":%lu,\"_kind\":\"LunaDbClientTest:1\",\"foo\":\"test_foo\",\"bar\":\"test_bar\"}"), i + 1);
        err = obj.fromJson(entry);
        MojErrCheck(err);
        err = db.put(obj);
        if (err)
            std::cerr << "Object #" << i << " should be put into db";
        MojErrCheck(err);
    }
    err = db.dump(outputFile, countOut);
    MojErrCheck(err);

    return MojErrNone;
}
