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

#ifndef MOJDBEASYSIGNAL_H
#define MOJDBEASYSIGNAL_H

#include <functional>

#include "core/MojSignal.h"
#include "core/internal/MojSignalInternal.h"

template <typename... Args>
class MojEasySlot
{
    struct Handler : MojSignalHandler
    {
        typedef typename MojSignal<Args...>::template Slot<Handler> SlotType;
        SlotType slot;

        std::function<MojErr(Args...)> handler;

        MojErr handle(Args... args) { return handler(args...); }

        Handler(decltype(handler) func) :
            slot(this, &Handler::handle),
            handler(std::move(func))
        {}
    };

    // handler always should be ref-counted
    MojRefCountedPtr<Handler> m_handler;
public:

    MojEasySlot(std::function<MojErr(Args...)> handler) :
        m_handler(new Handler(handler))
    {}

    ~MojEasySlot() { cancel(); }

    void cancel() { m_handler->slot.cancel(); }

    typename Handler::SlotType &slot() { return m_handler->slot; }

};

template <class... Args>
class MojEasySignal
{
    struct Handler : MojSignalHandler
    {
        typedef MojSignal<Args...> SignalType;
        SignalType signal;

        Handler() : signal(this) {}
    };

    // handler always should be ref-counted
    MojRefCountedPtr<Handler> m_handler;
public:
    template <typename T>
    using Slot = typename Handler::SignalType::template Slot<T>;

    typedef MojEasySlot<Args...> EasySlot;

    MojEasySignal() : m_handler(new Handler()) {}

    typename Handler::SignalType &signal() { return m_handler->signal; }

    void connect(typename Handler::SignalType::SlotRef ref)
    { m_handler->signal.connect(ref); }

    void connect(EasySlot &easySlot)
    { m_handler->signal.connect(easySlot.slot()); }

    MojErr call(Args... args)
    { return m_handler->signal.call(args...); }

    MojErr fire(Args... args)
    { return m_handler->signal.fire(args...); }
};

#endif
