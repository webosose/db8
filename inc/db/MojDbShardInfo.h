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

#ifndef MOJDBSHARDINFO_H
#define MOJDBSHARDINFO_H

#include "core/MojCoreDefs.h"
#include "core/MojSignal.h"
#include "core/MojString.h"
#include "db/MojDbKindIdList.h"
#include "db/MojDbEasySignal.h"

struct MojDbShardInfo
{
    typedef MojEasySignal<const MojDbShardInfo &> Signal;

    MojUInt32 id;
    bool active;
    bool transient;

    MojInt64 timestamp;
    MojString id_base64;
    MojString deviceId;
    MojString deviceUri;
    MojString mountPath;
    MojString deviceName;
    MojString databasePath;
    MojString parentDeviceId;

    MojDbKindIdList kindIds;

    MojDbShardInfo(MojUInt32 _id = 0, bool _active = false, bool _transient = false);
};

#endif
