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


#ifndef MOJDBLOCALETEST_H_
#define MOJDBLOCALETEST_H_

#include "MojDbTestRunner.h"

class MojDbLocaleTest : public MojTestCase
{
public:
	MojDbLocaleTest();

	virtual void cleanup();
	virtual MojErr run();

private:
	MojErr ICULocaleTestRun(MojDb& db);
	MojErr put(MojDb& db, const MojChar* kindName, const MojChar** objects);
	MojErr checkOrder(MojDb& db, const MojChar* kindName, const MojChar* expectedJson);
};

#endif /* MOJDBLOCALETEST_H_ */
