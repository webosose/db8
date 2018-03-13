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
 *  @file pool.hpp
 */

#pragma once

#include "iterable.hpp"

#include "core/MojErr.h"

#include <leveldb/iterator.h>
#include <leveldb/any_db.hpp>
#include <leveldb/ref_db.hpp>
#include <leveldb/walker.hpp>
#include <leveldb/cover_walker.hpp>

#include <unordered_map>
#include <utility>
#include <type_traits>

class PtrIteratorFactory final : public IteratorFactory
{
    std::unique_ptr<IteratorFactory> impl;
public:
    PtrIteratorFactory(IteratorFactory *ptr) { impl.reset(ptr); }

    std::unique_ptr<leveldb::Iterator> NewIterator() noexcept override
    { return impl->NewIterator(); }
};

template <typename TSelector, typename TBackend>
class Pool
{
public:
    typedef TSelector Sel;
    typedef TBackend Backend;
private:
    static_assert(std::is_base_of<leveldb::AnyDB, Backend>::value, "Backend should provide AnyDB interface");

    std::unordered_map<Sel, Backend> impls;

    class RefIteratorFactory : public IteratorFactory
    {
        leveldb::RefDB<Backend> impl;
    public:
        RefIteratorFactory(Backend &origin) : impl(origin) {}

        std::unique_ptr<leveldb::Iterator> NewIterator() noexcept override
        { return impl.NewIterator(); }
    };

    class CoverIteratorFactory : public IteratorFactory
    {
        PtrIteratorFactory base;
        Backend *overlay;
    public:
        CoverIteratorFactory(PtrIteratorFactory &&base, Backend &overlay) :
            base{std::move(base)},
            overlay{&overlay}
        {}

        std::unique_ptr<leveldb::Iterator> NewIterator() noexcept override
        {
            return leveldb::asIterator(leveldb::walker(leveldb::cover(base, *overlay)));
        }
    };

public:
    Pool() = default;

    Pool(std::initializer_list<Sel> selectors)
    { for (auto sel : selectors) Mount(sel); }

    // gcc 4.8 ICE: Pool(std::unordered_map<Sel, T> &orig, std::function<Backend(T &)> f = [](T &x) { return {x}; }) :

    template <typename T>
    Pool(std::unordered_map<Sel, T> &orig) :
        impls(orig.bucket_count())
    {
        for (auto &kv : orig) impls.emplace(kv.first, Backend {kv.second});
    }

    template <typename T, typename F>
    Pool(std::unordered_map<Sel, T> &orig, F f) :
        impls(orig.bucket_count())
    {
        for (auto &kv : orig) impls.emplace(kv.first, f(kv.second));
    }

    template <template <typename> class T>
    Pool<Sel, T<Backend>> ref()
    { return impls; }

    template <typename T>
    Pool<Sel, T> ref(std::function<T(Backend &)> f)
    { return {impls, f}; }

    template <typename... Args>
    Backend &Mount(Sel sel, Args &&... args)
    {
        MojAssert(impls.find(sel) == impls.end());
        return impls.emplace(sel, Backend {std::forward<Args>(args)...}).first->second;
    }
    Pool &Unmount(Sel sel)
    {
        auto it = impls.find(sel);
        MojAssert(it != impls.end());
        (void) impls.erase(it);
        return *this;
    }
    Backend &Select(Sel sel)
    {
        auto it = impls.find(sel);
        MojAssert(it != impls.end());
        return it->second;
    }

    std::unique_ptr<leveldb::Iterator> NewIterator() noexcept
    {
        MojAssert( !impls.empty() );

        PtrIteratorFactory cur = new RefIteratorFactory {impls.begin()->second};
        for (auto it = impls.begin(); ++it != impls.end();)
        {
            PtrIteratorFactory next = new CoverIteratorFactory {std::move(cur), it->second};
            cur = std::move(next);
        }
        return cur.NewIterator();
    }
};
