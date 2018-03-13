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


#ifndef MOJDBIDGENERATOR_H_
#define MOJDBIDGENERATOR_H_

#include "db/MojDbDefs.h"
#include "core/MojThread.h"
#include "core/MojString.h"

typedef MojUInt32 MojDbShardId;

class MojDbIdGenerator
{
public:
    // magical shard id that is not used as a prefix for _id
    static const MojDbShardId MainShardId;

	MojDbIdGenerator();

	MojErr init();
    MojErr id(MojObject& idOut, MojString shard);
    MojErr id(MojObject& idOut, MojDbShardId shard = MainShardId);
    static MojErr extractShard(const MojObject &id, MojDbShardId &shardIdOut);

private:
	char m_randStateBuf[8];
	MojRandomDataT m_randBuf;
	MojThreadMutex m_mutex;
};

#endif /* MOJDBIDGENERATOR_H_ */
