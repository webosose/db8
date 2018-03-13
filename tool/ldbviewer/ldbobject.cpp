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

#include "ldbobject.h"

LdbArray::LdbArray()
{
    clear();
}

LdbArray::LdbArray(const LdbArray& other)
{
    clear();
    std::copy(other.m_value_list.begin(), other.m_value_list.end(), m_value_list.begin());
}

LdbArray::~LdbArray()
{
    clear();
}

int LdbArray::getCount()
{
    return m_value_list.size();
}

bool LdbArray::clear()
{
    m_value_list.clear();
    return true;
}

list<LdbValue>& LdbArray::value()
{
    return m_value_list;
}


LdbObject::LdbObject()
{
    clear();
}

LdbObject::LdbObject(const LdbObject& other)
{
    clear();
    std::copy(other.m_data_list.begin(), other.m_data_list.end(), m_data_list.begin());
}

LdbObject::~LdbObject()
{
    clear();
}

int LdbObject::getCount()
{
    return m_data_list.size();
}

bool LdbObject::clear()
{
    m_data_list.clear();
    return true;
}

list<LdbData>& LdbObject::data()
{
    return m_data_list;
}

LdbObject& LdbObject::operator=(const LdbObject& other)
{
    clear();
    std::copy(other.m_data_list.begin(), other.m_data_list.end(), m_data_list.begin());
    return *this;
}
