// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef MOJDBLMDBENV_H_
#define MOJDBLMDBENV_H_

#include  <lmdb.h>
#include "core/MojString.h"
#include "core/MojFile.h"
#include "db/MojDbDefs.h"
#include "db/MojDbStorageEngine.h"

class MojDbLmdbEnv: public MojDbEnv
{
public:
	MojDbLmdbEnv();
	~MojDbLmdbEnv() override;

	MojErr configure(const MojObject& conf) override;
	MojErr open(const MojChar* dir) override;
	MojErr create();
	MojErr close();
	MDB_env* impl()
	{
		return m_env;
	}

private:
	static const MojChar* const LockFileName;

	MojErr lockDir(const MojChar* path);
	MojErr unlockDir();

	MojString m_lockFileName;
	MojFile m_lockFile;
	MDB_env* m_env;
	MojObject m_conf;
};

#endif /* MOJDBLMDBENV_H_ */
