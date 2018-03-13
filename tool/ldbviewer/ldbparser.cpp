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

#include "ldbparser.h"
#include "ldbreader.h"
#include <iostream>
#include <memory>

static unique_ptr<LdbObject> makeObject(unsigned char* buf, int size, int* pPos);
static unique_ptr<LdbArray> makeArray(unsigned char* buf, int size, int* pPos);
static bool readProp(unsigned char* buf, int size, int* pPos, LdbProperty& prop);
static bool readValue(unsigned char* buf, int size, int* pPos, LdbValue& value);

unique_ptr<LdbObject> LdbParser::parseBuffer(unsigned char* buf, int size, bool isArray)
{
    Marker marker;
    int pos = 0;
    unique_ptr<LdbObject> pRootObj = unique_ptr<LdbObject>(new LdbObject());
    if(pRootObj == NULL) {
        cerr << "Out of memory!" << endl;
        return unique_ptr<LdbObject>();
    }
    LdbData tempData;
    pRootObj->data().push_back(tempData);
    LdbData& rootData = pRootObj->data().front();
    rootData.prop().setPropertyName("First Object");
    marker = (Marker)buf[pos];
    if(marker == MarkerObjectBegin) {
        //Object
        pos++; // Skip Object begin marker
        unique_ptr<LdbObject> pObj = std::move(makeObject(buf, size, &pos));
        if(pObj.get() == NULL) {
            cerr << "makeObject failed" << endl;
            return unique_ptr<LdbObject>();
        }
        rootData.value().setObject(pObj);
    } else {
        //Array
        if(marker == MarkerArrayBegin) {
            pos++;
        }
        unique_ptr<LdbArray> pArr = std::move(makeArray(buf, size, &pos));
        if(pArr.get() == NULL) {
            cerr << "makeArray failed" << endl;
            return unique_ptr<LdbObject>();
        }
        rootData.value().setArray(pArr);
    }
    return pRootObj;
}

unique_ptr<LdbObject> makeObject(unsigned char* buf, int size, int* pPos)
{
    unique_ptr<LdbObject> pObj = unique_ptr<LdbObject>(new LdbObject());
    if(!pObj) {
        cerr << "Out of memory!" << endl;
        return unique_ptr<LdbObject>();
    }

    while(*pPos < size) {
        if(buf[*pPos] == MarkerObjectEnd) {
            break;
        }
        LdbData tempData;
        pObj->data().push_back(tempData);
        LdbData& data = pObj->data().back();
        if(readProp(buf, size, pPos, data.prop()) == false) {
            cerr << "read property failed" << endl;
            return unique_ptr<LdbObject>();
        }
        if(readValue(buf, size, pPos, data.value()) == false) {
            cerr << "read value failed" << endl;
            return unique_ptr<LdbObject>();
        }
    }

    return pObj;
}

unique_ptr<LdbArray> makeArray(unsigned char* buf, int size, int* pPos)
{
    unique_ptr<LdbArray> pArr = unique_ptr<LdbArray>(new LdbArray());
    if(pArr.get() == NULL) {
        cerr << "Out of memory!" << endl;
        return unique_ptr<LdbArray>();
    }

    while(*pPos < size) {
        if(buf[*pPos] == MarkerObjectEnd) {
            break;
        }
        LdbValue tempValue;
        pArr->value().push_back(tempValue);
        LdbValue& value = pArr->value().back();
        if(readValue(buf, size, pPos, value) == false) {
            cerr << "read value failed" << endl;
            return NULL;
        }
    }

    return pArr;
}

bool readProp(unsigned char* buf, int size, int* pPos, LdbProperty& prop)
{
    unsigned int propId = 0;
    Marker marker;
    string strValue;

    if(*pPos >= size) {
        cerr << "buffer overflow" << endl;
        return false;
    }

    marker = (Marker)buf[(*pPos)++];
    if((int)marker >= 0x20) {
        propId = (unsigned int)marker;
        prop.setPropertyId(propId);
        prop.setPropertyIdOnly(true);
        return true;
    } else {
        propId = 0;
    }

    switch(marker) {
        case MarkerStringValue:
            if(LdbReader::readString(&buf[*pPos], pPos, (void*)&strValue) == false) {
                cerr << "read string failed" << endl;
                return false;
            }
            prop.setPropertyName(strValue);
            break;
        case MarkerObjectEnd:
        default:
            cerr << "Illegal marker type" << endl;
            return false;
    }
    return true;
}

bool readValue(unsigned char* buf, int size, int* pPos, LdbValue& value)
{
    unsigned int valueId = 0;
    Marker marker;
    unique_ptr<LdbObject> pObj;
    unique_ptr<LdbArray> pArr;

    string strValue;
    uint8 u8Value;
    uint16 u16Value;
    uint32 u32Value;
    i64 i64Value;
    int iValue;
    bool bValue;

    if(*pPos < size) {
        marker = (Marker)buf[(*pPos)++];
        if((int)marker >= 0x20) {
            valueId = (unsigned int)marker;
            value.setId(valueId);
            return true;
        } else {
            valueId = 0;
        }

        switch(marker) {
            case MarkerObjectBegin:
                pObj = std::move(makeObject(buf, size, pPos));
                if(pObj.get() == NULL) {
                    cerr << "makeObject failed" << endl;
                    return false;
                }
                marker = (Marker)buf[(*pPos)++];
                if(marker != MarkerObjectEnd) {
                    cerr << "object begin & end mismatch" << endl;
                    return false;
                }
                value.setObject(pObj);
                break;
            case MarkerArrayBegin:
                pArr = std::move(makeArray(buf, size, pPos));
                if(pArr.get() == NULL) {
                    cerr << "makeArray failed" << endl;
                    return false;
                }
                marker = (Marker)buf[(*pPos)++];
                if(marker != MarkerObjectEnd) {
                    cerr << "array begin & end mismatch" << endl;
                    return false;
                }
                value.setArray(pArr);
                break;
            case MarkerNullValue:
                value.setNull();
                break;
            case MarkerStringValue:
                if(LdbReader::readString(&buf[*pPos], pPos, (void*)&strValue) == false) {
                    cerr << "Parsing string value failed" << endl;
                    return false;
                }
                value.setString(strValue);
                break;
            case MarkerFalseValue:
                if(LdbReader::readFalse(&buf[*pPos], pPos, (void*)&bValue) == false) {
                    cerr << "Parsing false value failed" << endl;
                    return false;
                }
                value.setBool(bValue);
                break;
            case MarkerTrueValue:
                if(LdbReader::readTrue(&buf[*pPos], pPos, (void*)&bValue) == false) {
                    cerr << "Parsing true value failed" << endl;
                    return false;
                }
                value.setBool(bValue);
                break;
            case MarkerZeroIntValue:
                if(LdbReader::readZeroInt(&buf[*pPos], pPos, (void*)&iValue) == false) {
                    cerr << "Parsing zeroint value failed" << endl;
                    return false;
                }
                value.setInt(iValue);
                break;
            case MarkerUInt8Value:
                if(LdbReader::readUInt8(&buf[*pPos], pPos, (void*)&u8Value) == false) {
                    cerr << "Parsing uint8 value failed" << endl;
                    return false;
                }
                value.setUInt(u8Value);
                break;
            case MarkerUInt16Value:
                if(LdbReader::readUInt16(&buf[*pPos], pPos, (void*)&u16Value) == false) {
                    cerr << "Parsing uint16 value failed" << endl;
                    return false;
                }
                value.setUInt(u16Value);
                break;
            case MarkerUInt32Value:
                if(LdbReader::readUInt32(&buf[*pPos], pPos, (void*)&u32Value) == false) {
                    cerr << "Parsing uint32 value failed" << endl;
                    return false;
                }
                value.setUInt(u32Value);
                break;
            case MarkerInt64Value:
                if(LdbReader::readInt64(&buf[*pPos], pPos, (void*)&i64Value) == false) {
                    cerr << "Parsing int64 value failed" << endl;
                    return false;
                }
                value.setInt64(i64Value);
                break;
            case MarkerNegativeDecimalValue:
                if(LdbReader::readNegativeInt(&buf[*pPos], pPos, (void*)&i64Value) == false) {
                    cerr << "Parsing negative decimal value failed" << endl;
                    return false;
                }
                value.setInt64(i64Value);
                break;
            case MarkerPositiveDecimalValue:
                if(LdbReader::readPositiveDecimal(&buf[*pPos], pPos, (void*)&i64Value) == false) {
                    cerr << "Parsing positive decimal value failed" << endl;
                    return false;
                }
                value.setInt64(i64Value);
                break;
            case MarkerNegativeIntValue:
                if(LdbReader::readNegativeInt(&buf[*pPos], pPos, (void*)&i64Value) == false) {
                    cerr << "Parsing negative int value failed" << endl;
                    return false;
                }
                value.setInt64(i64Value);
                break;
            case MarkerExtensionValue:
                {
                    ucptr extVal;
                    if(LdbReader::readExtended(&buf[*pPos], pPos, (void*)&u32Value, (void*)&extVal) == false) {
                        cerr << "Parsing extended value failed" << endl;
                        return false;
                    }
                    value.setExtended(u32Value, extVal);
                }
                break;
            case MarkerHeaderEnd:
            case MarkerInvalid:
            default:
                cerr << "Not supported marker type" << endl;
                return false;
        }
    }

    return true;
}


