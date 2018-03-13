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

/**
****************************************************************************************************
* Filename              : MojErrTest.h
* Description           : Header file for MojErr test.
****************************************************************************************************
**/

#ifndef MOJERRTEST_H_
#define MOJERRTEST_H_

#include "MojCoreTestRunner.h"

class MojErrTest : public MojTestCase
{
public:
	MojErrTest();

	virtual MojErr run();
};

#endif /* MOJERRTEST_H_ */
