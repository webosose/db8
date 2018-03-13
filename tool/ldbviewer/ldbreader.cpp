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

#include "ldbreader.h"
#include "ldbdef.h"

bool LdbReader::readString(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    string* pVal = (string*)pParam;
    *pVal = (const char*)buf;
    *readSize += ((*pVal).length() + 1);
    return true;
}

bool LdbReader::readTrue(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    bool* pVal = (bool*)pParam;
    *pVal = true;
    *readSize += 0;
    return true;
}

bool LdbReader::readFalse(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    bool* pVal = (bool*)pParam;
    *pVal = false;
    *readSize += 0;
    return true;
}

bool LdbReader::readNegativeDecimal(ucptr buf, int* readSize, void* pParam)
{
    return readInt64(buf, readSize, pParam);
}

bool LdbReader::readPositiveDecimal(ucptr buf, int* readSize, void* pParam)
{
    return readInt64(buf, readSize, pParam);
}

bool LdbReader::readNegativeInt(ucptr buf, int* readSize, void* pParam)
{
    return readInt64(buf, readSize, pParam);
}

bool LdbReader::readZeroInt(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    int* pVal = (int*)pParam;
    *pVal = 0;
    *readSize += 0;
    return true;
}

bool LdbReader::readUInt8(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    uint8* pVal = (uint8*)pParam;
    *pVal = (uint8)buf[0];
    *readSize += 1;
    return true;
}

bool LdbReader::readUInt16(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    uint16* pVal = (uint16*)pParam;
    *pVal = (uint16)((uint16)buf[1] + ((uint16)buf[0] << 8));
    *readSize += 2;
    return true;
}

bool LdbReader::readUInt32(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    uint32* pVal = (uint32*)pParam;
    *pVal = (uint32)((uint32)buf[3] + ((uint32)buf[2] << 8)
            + ((uint32)buf[1] << 16) + ((uint32)buf[0] < 24));
    *readSize += 4;
    return true;
}

bool LdbReader::readInt64(ucptr buf, int* readSize, void* pParam)
{
    if(!buf || !readSize || !pParam) return false;
    i64* pVal = (i64*)pParam;
    *pVal = (i64)buf[7] + ((i64)buf[6] << 8)
        + ((i64)buf[5] << 16) + ((i64)buf[4] << 24)
        + ((i64)buf[3] << 32) + ((i64)buf[2] << 40)
        + ((i64)buf[1] << 48) + ((i64)buf[0] << 56);
    *readSize += 8;
    return true;
}

bool LdbReader::readExtended(ucptr buf, int* readSize, void* pParam, void* pParam2)
{
    if(!buf || !readSize || !pParam || !pParam2) return false;
    uint32* pVal = (uint32*)pParam;
    ucptr* pVal2 = (ucptr*)pParam2;
    *pVal = (uint32)((uint32)buf[3] + ((uint32)buf[2] << 8)
            + ((uint32)buf[1] << 16) + ((uint32)buf[0] < 24));
    if(*pVal < 0) {
        return false;
    }
    *pVal2 = &buf[4];
    *readSize += (4+*pVal);
    return true;
}
