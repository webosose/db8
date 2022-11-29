// Copyright (c) 2009-2021 LG Electronics, Inc.
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


#ifndef MOJLUNAREQUEST_H_
#define MOJLUNAREQUEST_H_

#include "MojLunaDefs.h"
#include "core/MojJson.h"
#include "core/MojServiceRequest.h"

class MojLunaRequest : public MojServiceRequest
{
public:
	MojLunaRequest(MojService* service);
	MojLunaRequest(MojService* service, const MojString& requester);
	MojLunaRequest(MojService* service, const MojString& originaExe, const MojString& originaId, const MojString& originaName);
	bool isProxyRequest() const { return !m_requester.empty(); }
	bool isOriginRequest() const { return !m_originName.empty(); }
	bool cancelled() const { return m_cancelled; }
	const MojChar* payload() const { return m_writer.json(); }
	const MojChar* getRequester() const { return m_requester.data(); }
	const MojChar* getOriginExe() const { return m_originExe.data(); }
	const MojChar* getOriginId() const { return m_originId.data(); }
	const MojChar* getOriginName() const { return m_originName.data(); }
	virtual MojObjectVisitor& writer() { return m_writer; }

private:
	friend class MojLunaService;

	void cancelled(bool val) { m_cancelled = val; }

	MojJsonWriter m_writer;

	MojString m_requester;

	MojString m_originExe;
	MojString m_originId;
	MojString m_originName;

	bool m_cancelled;
};

#endif /* MOJLUNAREQUEST_H_ */
