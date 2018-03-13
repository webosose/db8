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

#include <map>
#include <string>
#include <memory>
#include <vector>

using namespace std;

enum LdbType {
    TypeUndefined,
    TypeId,
    TypeNull,
    TypeObject,
    TypeArray,
    TypeString,
    TypeBool,
    TypeInt,
    TypeUInt,
    TypeDecimal=0x19,
    TypeInt64=0x19,
    TypeExtended,
};

enum Marker {
    MarkerInvalid = -1,
    MarkerObjectEnd = 0,
    MarkerNullValue = 1,
    MarkerObjectBegin = 2,
    MarkerArrayBegin = 3,
    MarkerStringValue = 4,
    MarkerFalseValue = 5,
    MarkerTrueValue = 6,
    MarkerNegativeDecimalValue = 7,
    MarkerPositiveDecimalValue = 8,
    MarkerNegativeIntValue = 9,
    MarkerZeroIntValue = 10,
    MarkerUInt8Value = 11,
    MarkerUInt16Value = 12,
    MarkerUInt32Value = 13,
    MarkerInt64Value = 14,
    MarkerExtensionValue = 15,
    MarkerHeaderEnd = 16
};

typedef long long i64;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef vector<unsigned char> ucvec;
typedef unsigned char* ucptr;

typedef map<string,string> map_str;
typedef map<int,string> map_is;
typedef map<string,int> map_si;
typedef map<i64,map_is> map_imis;

struct db8tokens_map_t
{
    map_si si;
    map_imis imis;
};
