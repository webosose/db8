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

namespace LdbReader
{
    bool readString(ucptr buf, int* readSize, void* pParam);
    bool readTrue(ucptr buf, int* readSize, void* pParam);
    bool readFalse(ucptr buf, int* readSize, void* pParam);
    bool readNegativeDecimal(ucptr buf, int* readSize, void* pParam);
    bool readPositiveDecimal(ucptr buf, int* readSize, void* pParam);
    bool readNegativeInt(ucptr buf, int* readSize, void* pParam);
    bool readZeroInt(ucptr buf, int* readSize, void* pParam);
    bool readUInt8(ucptr buf, int* readSize, void* pParam);
    bool readUInt16(ucptr buf, int* readSize, void* pParam);
    bool readUInt32(ucptr buf, int* readSize, void* pParam);
    bool readInt64(ucptr buf, int* readSize, void* pParam);
    bool readExtended(ucptr buf, int* readSize, void* pParam, void* pParam2);
};
