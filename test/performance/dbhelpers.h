// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#define BOOST_NO_CXX11_SCOPED_ENUMS // BUG in gentoo
#include <boost/filesystem.hpp>
#include "db/MojDb.h"

MojErr getMiddleObject(MojDb* db, MojObject* obj, MojDbReqRef req = MojDbReq());
MojErr getFirstLastObject(MojDb* db, MojObject* obj, bool first, MojDbReqRef req = MojDbReq());

bool copyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination);
