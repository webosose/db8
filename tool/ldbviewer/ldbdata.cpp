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

#include "ldbdata.h"
#include "ldbobject.h"
#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

LdbValue::LdbValue()
{
    m_Id = 0;
    m_type = TypeUndefined;
    clear();
}

LdbValue::LdbValue(const LdbValue& other)
{
    m_Id = other.m_Id;
    m_type = other.m_type;
    m_uni_value = other.m_uni_value;
    if(m_type == TypeString) {
        m_zVal = other.m_zVal;
    } else if(m_type == TypeObject) {
        m_objVal = unique_ptr<LdbObject>(new LdbObject(*other.m_objVal));
    } else if(m_type == TypeArray) {
        m_arrVal = unique_ptr<LdbArray>(new LdbArray(*other.m_arrVal));
    } else if(m_type == TypeExtended) {
        m_extSize = other.m_extSize;
        m_exVal = unique_ptr<ucvec>(new ucvec(other.m_extSize));
        if(m_exVal.get() == NULL) {
            cerr << "out of memory!!" << endl;
        }
        m_exVal->assign(other.m_exVal->begin(), other.m_exVal->end());
    }
}

LdbValue::LdbValue(LdbType type)
{
    m_Id = 0;
    m_type = type;
    clear();
}

LdbValue::~LdbValue()
{
    clear();
}

string& LdbValue::getString()
{
    return m_zVal;
}

bool LdbValue::setString(string& val)
{
    clear();
    m_type = TypeString;
    m_zVal = val;
    return true;
}

bool LdbValue::getBool()
{
    return m_uni_value.bVal;
}

bool LdbValue::setBool(bool val)
{
    clear();
    m_type = TypeBool;
    m_uni_value.bVal = val;
    return true;
}

int LdbValue::getInt()
{
    return m_uni_value.iVal;
}

bool LdbValue::setInt(int val)
{
    clear();
    m_type = TypeInt;
    m_uni_value.iVal = val;
    return true;
}

i64 LdbValue::getInt64()
{
    return m_uni_value.dVal;
}

bool LdbValue::setInt64(i64 val)
{
    clear();
    m_type = TypeInt64;
    m_uni_value.dVal = val;
    return true;
}

unsigned int LdbValue::getUInt()
{
    return m_uni_value.uiVal;
}

bool LdbValue::setUInt(unsigned int val)
{
    clear();
    m_type = TypeUInt;
    m_uni_value.uiVal = val;
    return true;
}

unsigned int LdbValue::getExtendedSize()
{
    return m_extSize;
}

unique_ptr<ucvec>& LdbValue::getExtendedVal()
{
    return m_exVal;
}

static void vecCopy(unique_ptr<ucvec>& vval, ucptr pval, int size)
{
    for(int i = 0 ; i < size ; i++) {
        (*vval)[i] = pval[i];
    }
}

bool LdbValue::setExtended(unsigned int size, ucptr val)
{
    clear();
    m_type = TypeExtended;
    assert(size >= 0);
    m_extSize = size;
    m_exVal = unique_ptr<ucvec>(new ucvec(size));
    if(m_exVal.get() == NULL) {
        cerr << "out of memory!!" << endl;
        return false;
    }
    vecCopy(m_exVal, val, size);
    return true;
}

LdbType LdbValue::getType()
{
    return m_type;
}

bool LdbValue::setType(LdbType type)
{
    clear();
    m_type = type;
    return true;
}

unique_ptr<LdbObject>& LdbValue::getObject()
{
    return m_objVal;
}

bool LdbValue::setObject(unique_ptr<LdbObject>& pObj)
{
    clear();
    m_type = TypeObject;
    m_objVal = std::move(pObj);
    return true;
}

unique_ptr<LdbArray>& LdbValue::getArray()
{
    return m_arrVal;
}

bool LdbValue::setArray(unique_ptr<LdbArray>& pArr)
{
    clear();
    m_type = TypeArray;
    m_arrVal = std::move(pArr);
    return true;
}

bool LdbValue::setNull()
{
    clear();
    m_type = TypeNull;
    return true;
}

bool LdbValue::setId(unsigned int id)
{
    clear();
    m_type = TypeId;
    m_Id = id;
    return true;
}

unsigned int LdbValue::getId()
{
    return m_Id;
}

bool LdbValue::clear()
{
    m_zVal.clear();
    m_objVal.reset();
    m_arrVal.reset();
    m_exVal.reset();
    m_Id = 0;
    m_extSize = 0;
    m_type = TypeUndefined;
    return true;
}

LdbValue& LdbValue::operator=(const LdbValue& other)
{
    clear();
    m_Id = other.m_Id;
    m_type = other.m_type;
    m_uni_value = other.m_uni_value;
    if(m_type == TypeString) {
        m_zVal = other.m_zVal;
    } else if(m_type == TypeObject) {
        m_objVal = unique_ptr<LdbObject>(new LdbObject(*other.m_objVal));
    } else if(m_type == TypeArray) {
        m_arrVal = unique_ptr<LdbArray>(new LdbArray(*other.m_arrVal));
    } else if(m_type == TypeExtended) {
        m_extSize = other.m_extSize;
        m_exVal = unique_ptr<ucvec>(new ucvec(other.m_extSize));
        if(m_exVal.get() == NULL) {
            cerr << "out of memory!!" << endl;
            return *this;
        }
        m_exVal->assign(other.m_exVal->begin(), other.m_exVal->end());
    }
    return *this;
}
