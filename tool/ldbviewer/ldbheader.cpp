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

#include "ldbdef.h"
#include "ldbheader.h"
#include "ldbreader.h"
#include <iostream>

LdbHeader::LdbHeader()
{
    m_version = 0;
    m_kindId = 0;
    m_kindName = "";
    m_del = false;
    m_read = false;
    m_size = 0;
}

LdbHeader::~LdbHeader()
{
}

int LdbHeader::parseBuffer(unsigned char* buf, int size)
{
    static map_is none;
    return parseBuffer(buf, size, none);
}

int LdbHeader::parseBuffer(unsigned char* buf, int size, map_is& kmap)
{
    int pos = 0;
    Marker marker;

    m_read = true;

    // version: uint8
    if(LdbReader::readUInt8(buf, &pos, (void*)&m_version) == false) {
        return -1;
    }
    // kindId: intvalue
    m_kindId = readInt(buf, &pos);
    // get name from kindId
    m_kindName = kmap[(int)m_kindId];

    // rev
    m_rev = readInt(buf, &pos);
    // del
    if(buf[pos] != MarkerHeaderEnd) {
        marker = (Marker)buf[pos++];
        switch(marker) {
            case MarkerTrueValue:
                m_del = true;
                break;
            case MarkerFalseValue:
                m_del = false;
                break;
            default:
                cerr << "Ooops! Illegal marker type" << endl;
                break;
        }
    }
    while(buf[pos] != MarkerHeaderEnd) {
        pos++;
    }
    pos++;
    m_size = pos;
    return pos;
}

i64 LdbHeader::readInt(unsigned char* buf, int* pPos)
{
    int iValue;
    uint8 u8Value;
    uint16 u16Value;
    uint32 u32Value;
    i64 i64Value;

    Marker marker = (Marker)buf[(*pPos)++];

    switch(marker) {
        case MarkerNegativeIntValue:
            LdbReader::readNegativeInt(&buf[*pPos], pPos, (void*)&i64Value);
            break;
        case MarkerNegativeDecimalValue:
            LdbReader::readNegativeDecimal(&buf[*pPos], pPos, (void*)&i64Value);
            break;
        case MarkerPositiveDecimalValue:
            LdbReader::readPositiveDecimal(&buf[*pPos], pPos, (void*)&i64Value);
            break;
        case MarkerZeroIntValue:
            LdbReader::readZeroInt(&buf[*pPos], pPos, (void*)&iValue);
            i64Value = iValue;
            break;
        case MarkerUInt8Value:
            LdbReader::readUInt8(&buf[*pPos], pPos, (void*)&u8Value);
            i64Value = u8Value;
            break;
        case MarkerUInt16Value:
            LdbReader::readUInt16(&buf[*pPos], pPos, (void*)&u16Value);
            i64Value = u16Value;
            break;
        case MarkerUInt32Value:
            LdbReader::readUInt32(&buf[*pPos], pPos, (void*)&u32Value);
            i64Value = u32Value;
            break;
        case MarkerInt64Value:
            LdbReader::readInt64(&buf[*pPos], pPos, (void*)&i64Value);
            break;
        default:
            cerr << "Ooops! Illegal marker type" << endl;
            break;
    }
    return i64Value;
}
