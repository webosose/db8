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

using namespace std;

class LdbHeader
{
    public:
        LdbHeader();
        ~LdbHeader();
        int parseBuffer(unsigned char* buf, int size);
        int parseBuffer(unsigned char* buf, int size, map_is& kmap);
        int getSize() { return m_size; }
        bool getRead() { return m_read; }
        bool resetRead() { m_read = false; return true; }
        uint8 getVersion() { return m_version; }
        i64 getKindId() { return m_kindId; }
        string& getKindIdName() { return m_kindName; }
        i64 getRev() { return m_rev; }
        bool getDel() { return m_del; }
    private:
        i64 readInt(unsigned char* buf, int* pPos);
    private:
        uint8 m_version;
        i64 m_kindId;
        string m_kindName;
        i64 m_rev;
        bool m_del;
        bool m_read;
        int m_size;
};

