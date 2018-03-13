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
#include <string>
#include <memory>

using namespace std;

class LdbObject;
class LdbArray;

class LdbValue
{
    public:
        LdbValue();
        LdbValue(const LdbValue& other);
        LdbValue(LdbType type);
        ~LdbValue();

        string& getString();
        bool setString(string& val);
        bool getBool();
        bool setBool(bool val);
        i64 getDecimal();
        bool setDecimal(i64 val);
        int getInt();
        bool setInt(int val);
        i64 getInt64();
        bool setInt64(i64 val);
        unsigned int getUInt();
        bool setUInt(unsigned int val);
        unsigned int getExtendedSize();
        unique_ptr<ucvec>& getExtendedVal();
        bool setExtended(unsigned int size, ucptr val);
        LdbType getType();
        bool setType(LdbType type);
        unique_ptr<LdbObject>& getObject();
        bool setObject(unique_ptr<LdbObject>& pObj);
        unique_ptr<LdbArray>& getArray();
        bool setNull();
        bool setArray(unique_ptr<LdbArray>& pArr);
        bool setId(unsigned int id);
        unsigned int getId();

        bool clear();
        LdbValue& operator=(const LdbValue& other);

    protected:

        union Value {
            bool bVal;
            i64 dVal;
            int iVal;
            unsigned int uiVal;
            unsigned char uVal;
        };
        Value                  m_uni_value;
        string                 m_zVal;
        unique_ptr<LdbObject>  m_objVal;
        unique_ptr<LdbArray>   m_arrVal;
        unique_ptr<ucvec>      m_exVal;
        LdbType                m_type;
        unsigned int           m_Id;
        unsigned int           m_extSize;
};

class LdbProperty
{
    public:
        LdbProperty() {
            clear();
        }
        ~LdbProperty() {
            clear();
        }

        bool setPropertyId(unsigned int id) {
            m_propId = id;
            m_isIdOnly = true;
            return true;
        }
        unsigned int getPropertyId() { return m_propId; }
        bool setPropertyName(const char* str) {
            m_propName = string(str);
            m_isIdOnly = false;
            return true;
        }
        bool setPropertyName(string& str) {
            m_propName = str;
            m_isIdOnly = false;
            return true;
        }
        string& getPropertyName() { return m_propName; }
        bool setPropertyIdOnly(bool is) {
            m_isIdOnly = is;
            return true;
        }
        bool getPropertyIdOnly() { return m_isIdOnly; }
        bool clear() {
            m_propId = 0;
            m_propName = "";
            m_isIdOnly = true;
            return true;
        }
    protected:
        unsigned int m_propId;
        string m_propName;
        bool m_isIdOnly;
};

class LdbData
{
    public:
        LdbData() { }
        ~LdbData() { }

        LdbValue& value() { return m_value; }
        LdbProperty& prop() { return m_prop; }

        bool clear() {
            m_value.clear();
            m_prop.clear();
            return true;
        }

    protected:
        LdbValue m_value;
        LdbProperty m_prop;
};


