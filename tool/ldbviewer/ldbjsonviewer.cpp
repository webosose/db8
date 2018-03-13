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
#include "ldbjsonviewer.h"
#include <iostream>
#include <string>
#include <sstream>
#include <map>

using namespace std;

namespace std
{
    inline string my_to_string(i64 _Val)
    {
        stringstream ss;
        ss << _Val;
        return ss.str();
    }
}

bool LdbJsonViewer::view(unique_ptr<LdbObject>& pObj, string& out, map_is& kmap)
{
    if(!pObj) {
        cerr << "Null object" << endl;
        return false;
    }

    list<LdbData>& datalist = pObj->data();
    for( list<LdbData>::iterator it = datalist.begin() ; it != datalist.end() ; it++ ) {
        if(it != datalist.begin()) {
            out += ",";
        }
        LdbProperty& prop = it->prop();
        LdbValue& value = it->value();

        if(prop.getPropertyIdOnly()) {
            string propName = kmap[prop.getPropertyId()];
            if(propName.empty()) {
                out += my_to_string(prop.getPropertyId());
            } else {
                out += "\"";
                out += propName;
                out += "\"";
            }
            out += ":";
        } else {
            out += "\"";
            out += prop.getPropertyName();
            out += "\"";
            out += ":";
        }

        if(viewValue(value, out, kmap) == false) {
            return false;
        }
    }
    return true;
}

bool LdbJsonViewer::arrayView(unique_ptr<LdbArray>& pArr, string& out, map_is& kmap)
{
    if(!pArr) {
        cerr << "Null array" << endl;
        return false;
    }

    list<LdbValue>& valuelist = pArr->value();
    for( list<LdbValue>::iterator it = valuelist.begin() ; it != valuelist.end() ; it++ ) {
        if(it != valuelist.begin()) {
            out += ",";
        }
        LdbValue& value = *it;

        if(viewValue(value, out, kmap) == false) {
            return false;
        }
    }
    return true;
}

bool LdbJsonViewer::viewValue(LdbValue& val, string& out, map_is& kmap)
{
    string valueName;
    LdbType type = val.getType();
    switch(type) {
        case TypeId:
            valueName = kmap[val.getId()];
            if(valueName.empty()) {
                out += my_to_string(val.getId());
            } else {
                out += "\"";
                out += valueName;
                out += "\":";
            }
            break;
        case TypeObject:
            out += "{";
            if(view(val.getObject(), out, kmap) == false) {
                return false;
            }
            out += "}";
            break;
        case TypeArray:
            out += "[";
            if(arrayView(val.getArray(), out, kmap) == false) {
                return false;
            }
            out += "]";
            break;
        case TypeNull:
            out += "Null";
            break;
        case TypeString:
            out += "\"";
            out += val.getString();
            out += "\"";
            break;
        case TypeBool:
            out += (val.getBool() == true) ? "true" : "false";
            break;
            //case TypeDecimal:
        case TypeInt64:
            out += my_to_string(val.getInt64());
            break;
        case TypeInt:
            out += my_to_string((i64)val.getUInt());
            break;
        case TypeUInt:
            out += my_to_string((i64)val.getInt());
            break;
        case TypeExtended:
            //TODO
            out += "EX:";
            out += my_to_string((i64)val.getUInt());
            break;
        case TypeUndefined:
        default:
            cerr << "Unknown type: " << (int)type << endl;
            return false;
    }
    return true;
}

