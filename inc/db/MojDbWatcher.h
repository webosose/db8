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


#ifndef MOJDBWATCHER_H_
#define MOJDBWATCHER_H_

#include "db/MojDbDefs.h"
#include "db/MojDbQueryPlan.h"
#include "db/MojDbShardInfo.h"
#include "core/MojSignal.h"
#include "core/MojThread.h"

class MojDbWatcher : public MojSignalHandler
{
public:
	typedef MojVector<MojDbKeyRange> RangeVec;
	typedef MojSignal<> Signal;

	MojDbWatcher(Signal::SlotRef handler);
	~MojDbWatcher();

	void domain(const MojString& val) { m_domain = val; }
	const MojString& domain() const { return m_domain; }
	const RangeVec& ranges() const { return m_ranges; }

	void init(MojDbIndex* index, const RangeVec& ranges, bool desc, bool active);
	MojErr activate(const MojDbKey& endKey);
	MojErr fire(const MojDbKey& key);
	MojErr abandon(); //!< mark MojDbWatcher that no one will trigger it

private:
	typedef enum {
		StateInvalid,
		StatePending,
		StateActive
	} State;

	virtual MojErr handleCancel();
	MojErr fireImpl();
	MojErr invalidate();
	MojErr shardStatusChanged(const MojDbShardInfo& shardInfo);

	MojThreadMutex m_mutex;
	Signal m_signal;
	RangeVec m_ranges;
	MojDbKey m_limitKey;
	MojDbKey m_fireKey;
	MojString m_domain;
	bool m_desc;
	State m_state;
	MojDbIndex* m_index;

public:
	MojDbShardInfo::Signal::Slot<MojDbWatcher> shardStatusChangedSlot;
};

#endif /* MOJDBWATCHER_H_ */
