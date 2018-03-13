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

#include "core/MojRefCount.h"

#include <array>
#include <thread>
#include <mutex>
#include <functional>

namespace {
	template <typename T>
	struct LifeMonitor : T
	{
		LifeMonitor(bool &alive) : alive(alive) { alive = true; }
		~LifeMonitor() { alive = false; }
		bool &alive;
	};

	typedef LifeMonitor<MojRefCounted> RefMonitor;
} // anonymous namespace

TEST(RefCount, basic)
{
	bool alive;

	MojRefCountedPtr<RefMonitor> ref(new RefMonitor(alive));
	EXPECT_TRUE( alive );

	ref.reset();
	EXPECT_FALSE( alive );
}

TEST(RefCount, zombie_bash)
{
	struct Man : MojRefCounted
	{
		Man(std::function<void()> lastWill) :
			m_lastWill(lastWill)
		{}
		~Man() { m_lastWill(); }
	private:
		std::function<void()> m_lastWill;
	};

	MojAtomicInt deaths = 0;
	{
		MojRefCountedPtr<Man> zombie;
		{
			MojRefCountedPtr<Man> man;

			auto lastWill = [&]() {
				EXPECT_EQ( 0, deaths.value() );
				++deaths;
				zombie = man; // turn into a zombie
			};
			man.reset(new Man(lastWill));
			EXPECT_EQ( 0, deaths.value() );
		}
		EXPECT_EQ( 1, deaths.value() );
	} // magic happens on this line
	EXPECT_EQ( 1, deaths.value() );
}

TEST(RefCount, threaded)
{
	const size_t nthreads = 32, ntries = 100;
	pthread_barrier_t a, b, c;

	bool alive = false;
	MojRefCountedPtr<RefMonitor> ref;

	const auto f = [&]() {
		EXPECT_TRUE( alive );
		(void) pthread_barrier_wait(&a);
		ref->retain();
		EXPECT_TRUE( alive );
		(void) pthread_barrier_wait(&b);
		(void) pthread_barrier_wait(&c);
		ref->release();
	};

	for (size_t ntry = 0; ntry < ntries; ++ntry)
	{
		ASSERT_EQ( 0, pthread_barrier_init(&a, nullptr, nthreads+1) );
		ASSERT_EQ( 0, pthread_barrier_init(&b, nullptr, nthreads+1) );
		ASSERT_EQ( 0, pthread_barrier_init(&c, nullptr, nthreads+1) );

		EXPECT_FALSE( alive );

		ref.reset(new RefMonitor(alive));
		EXPECT_TRUE( alive );
		EXPECT_TRUE( ref.get() );

		std::array<std::thread, nthreads> threads;
		for (auto &thread : threads) thread = std::thread(f);
		ASSERT_TRUE( alive );
		ASSERT_TRUE( ref.get() );

		(void) pthread_barrier_wait(&a);
		// all threads call retain()

		(void) pthread_barrier_wait(&b);
		// all threads finished retain()

		ASSERT_TRUE( ref.get() );
		EXPECT_EQ( MojInt32(nthreads+1), ref->refCount() );
		ASSERT_TRUE( alive );

		(void) pthread_barrier_wait(&c);
		// all threads started release()

		for(auto &thread : threads) thread.join();
		// all threads finished release() and exited
		EXPECT_EQ( MojInt32(1), ref->refCount() );
		ASSERT_TRUE( alive ); // last ref
		ref.reset(); // drop it
		EXPECT_FALSE( alive );

		EXPECT_EQ( 0, pthread_barrier_destroy(&a) );
		EXPECT_EQ( 0, pthread_barrier_destroy(&b) );
		EXPECT_EQ( 0, pthread_barrier_destroy(&c) );
	}
}
