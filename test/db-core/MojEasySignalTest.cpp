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

#include "db/MojDbEasySignal.h"

#include "Runner.h"

TEST(SignalTest, easy_peasy)
{
    int signaled = -1;
    MojEasySignal<int> signal;
    decltype(signal)::EasySlot slot([&](int x) { signaled = x; return MojErrNone; });

    signal.connect(slot.slot());

    MojExpectNoErr( signal.call(42) );
    EXPECT_EQ(42, signaled);

    MojExpectNoErr( signal.fire(45) );
    EXPECT_EQ(45, signaled);

    MojExpectNoErr( signal.fire(41) );
    EXPECT_EQ(45, signaled);

    MojEasySlot<int> badSlot([&](int x) { signaled = x; MojErrThrowMsg(MojErrUnknown, "intentional"); });

    signal.connect(badSlot.slot());

    EXPECT_EQ( MojErrUnknown, signal.call(41) );
    EXPECT_EQ(41, signaled);

    EXPECT_EQ( MojErrUnknown, signal.call(41) );
    EXPECT_EQ(41, signaled);
}
