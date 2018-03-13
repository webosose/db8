// Copyright (c) 2014-2018 LG Electronics, Inc.
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

/**
 *  @file iteratable.hpp
 */

#pragma once

#include <leveldb/iterator.h>
#include <leveldb/any_db.hpp>

struct IteratorFactory
{
    virtual std::unique_ptr<leveldb::Iterator> NewIterator() noexcept = 0;

    struct Walker final : leveldb::AnyDB::Walker
    {
        Walker() = default;
        Walker(IteratorFactory &factory) : leveldb::AnyDB::Walker(factory.NewIterator())
        {}
    };
    virtual ~IteratorFactory()
    {
    }
};
