// Copyright (c) 2009-2018 LG Electronics, Inc.
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

#pragma once

#include "ldbdef.h"
#include "ldbobject.h"

using namespace std;

class LdbJsonViewer
{
    public:
        static bool view(unique_ptr<LdbObject>& pObj, string& out, map_is& kmap);
        static bool arrayView(unique_ptr<LdbArray>& pArr, string& out, map_is& kmap);
        static bool viewValue(LdbValue& val, string& out, map_is& kmap);
};
