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

#include "Runner.h"

#include "core/MojSignal.h"

#include <array>
#include <thread>
#include <mutex>

namespace {
    typedef MojSignal<> Signal;
    struct SomeSignal : MojSignalHandler
    {
        SomeSignal(bool *alive = nullptr) :
            alive(alive),
            signal(this)
        {
            if (alive) *alive = true;
        }

        SomeSignal(Signal::SlotRef handler, bool *alive = nullptr) :
            alive(alive),
            signal(this)
        {
            if (alive) *alive = true;
            signal.connect(handler);
        }

        ~SomeSignal() { if (alive) *alive = false; }

        bool *alive;
        Signal signal;
    };

    struct SomeSlot : MojSignalHandler
    {
        SomeSlot(bool *alive = nullptr) :
            alive(alive),
            slot(this, &SomeSlot::handle)
        { if (alive) *alive = true; }
        ~SomeSlot() { if (alive) *alive = false; }

        MojErr handle() { return MojErrNone; }

        bool *alive;
        Signal::Slot<SomeSlot> slot;
    };
} // anonymous namespace

TEST(Signal, retain_logic)
{
    bool signalAlive = false;
    bool slotAlive = false;
    MojRefCountedPtr<SomeSlot> someSlot(new SomeSlot(&slotAlive));
    MojRefCountedPtr<SomeSignal> someSignal(new SomeSignal(someSlot->slot, &signalAlive));

    EXPECT_TRUE( signalAlive );
    EXPECT_TRUE( slotAlive );

    someSignal.reset(); // release signal
    EXPECT_FALSE( signalAlive );
    EXPECT_TRUE( slotAlive );

    someSignal.reset(new SomeSignal(someSlot->slot, &signalAlive));
    EXPECT_TRUE( signalAlive );
    EXPECT_TRUE( slotAlive );

    auto &slotRef = someSlot->slot; // remember ref to slot
    someSlot.reset(); // release strong reference on slot
    EXPECT_TRUE( signalAlive );
    EXPECT_TRUE( slotAlive );

    slotRef.cancel(); // drop connection
    EXPECT_TRUE( signalAlive );
    EXPECT_FALSE( slotAlive );

    someSlot.reset(new SomeSlot(&slotAlive));
    someSignal->signal.connect(someSlot->slot);
    EXPECT_TRUE( signalAlive );
    EXPECT_TRUE( slotAlive );

    someSlot.reset(); // same as before signal keeps ref on slot
    EXPECT_TRUE( signalAlive );
    EXPECT_TRUE( slotAlive );

    someSignal.reset();     // on the other hand when signal lose its last ref
                            // noone will be able to signal it so it drop refs
    EXPECT_FALSE( signalAlive );
    EXPECT_FALSE( slotAlive );
}

TEST(Signal, slots_cancel_vs_signal_release)
{
    const size_t nthreads = 32, ntries = 10000;

    pthread_barrier_t barrier;

    MojRefCountedPtr<SomeSignal> someSignal;

    const auto f = [&](MojRefCountedPtr<SomeSlot> someSlot) {
        (void) pthread_barrier_wait(&barrier);
        someSlot->slot.cancel();
    };

    for (size_t ntry = 0; ntry < ntries; ++ntry)
    {
        ASSERT_EQ( 0, pthread_barrier_init(&barrier, nullptr, nthreads+1) );

        someSignal.reset(new SomeSignal());

        std::array<std::thread, nthreads> threads;
        for (auto &thread : threads)
        {
            MojRefCountedPtr<SomeSlot> someSlot(new SomeSlot());

            // connect slots synchronously and delegate to thread
            someSignal->signal.connect(someSlot->slot);

            thread = std::thread(f, someSlot);
        }

        (void) pthread_barrier_wait(&barrier);

        someSignal.reset();

        for(auto &thread : threads) thread.join();

        EXPECT_EQ( 0, pthread_barrier_destroy(&barrier) );
    }
}

TEST(Signal, signal_cancel_vs_vector_release)
{
    const size_t nthreads = 32, ntries = 10000;

    pthread_barrier_t barrier;

    MojVector<MojRefCountedPtr<SomeSignal> > signals;
    std::mutex signals_mutex;

    const auto f = [&](const size_t worker) {
        MojRefCountedPtr<SomeSlot> someSlot(new SomeSlot());
        MojRefCountedPtr<SomeSignal> someSignal(new SomeSignal(someSlot->slot));

        signals_mutex.lock();
        MojExpectNoErr( signals.push(someSignal.get()) );
        someSignal.reset();
        signals_mutex.unlock();

        (void) pthread_barrier_wait(&barrier);

        someSlot->slot.cancel();
    };

    for (size_t ntry = 0; ntry < ntries; ++ntry)
    {
        ASSERT_EQ( 0, pthread_barrier_init(&barrier, nullptr, nthreads+1) );

        std::array<std::thread, nthreads> threads;
        for (size_t worker = 0; worker < threads.size(); ++worker)
        {
            threads[worker] = std::thread(f, worker);
        }

        (void) pthread_barrier_wait(&barrier);

        signals.clear();

        for(auto &thread : threads) thread.join();

        EXPECT_EQ( 0, pthread_barrier_destroy(&barrier) );
    }
}
