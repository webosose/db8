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

#ifndef MOJDBLMDBERR_H_
#define MOJDBLMDBERR_H_

#include <lmdb.h>
#include <core/MojCoreDefs.h>
#include "core/MojLogDb8.h"

inline MojErr translateErr(int dbErr)
{
	LOG_TRACE("Entering function %s", __FUNCTION__);
	dbErr = MojErrDbFatal;
	return static_cast<MojErr>(dbErr);
}

#define MojLmdbErrCheck(E, FNAME) if (E) MojErrThrowMsg(translateErr(E), _T("mdb: %s - %s"), FNAME, mdb_strerror(E))
#define MojLmdbErrAccumulate(EACC, E, FNAME) if (E) MojErrAccumulate(EACC, translateErr(E))
#define MojLmdbTxnFromStorageTxn(TXN) ((TXN) ? static_cast<MojDbLmdbTxn*>(TXN)->impl() : NULL)

#endif /* MOJDBLMDBERR_H_ */
