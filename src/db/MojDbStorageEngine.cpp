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


#include <cstdlib>

#include "db/MojDbStorageEngine.h"
#include "core/MojObjectBuilder.h"
#include "core/MojJson.h"
#include "core/MojLogDb8.h"

MojDbStorageEngine::Factory MojDbStorageEngine::m_factory;
MojDbStorageEngine::Factories MojDbStorageEngine::m_factories;

MojErr MojDbStorageItem::toObject(MojObject& objOut, MojDbKindEngine& kindEngine, bool headerExpected) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObjectBuilder builder;
	MojErr err = visit(builder, kindEngine, headerExpected);
	MojErrCheck(err);
	objOut = builder.object();
	return MojErrNone;
}

MojErr MojDbStorageItem::toJson(MojString& strOut, MojDbKindEngine& kindEngine) const
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojJsonWriter writer;
	MojErr err = visit(writer, kindEngine);
	MojErrCheck(err);
	strOut = writer.json();
	return MojErrNone;
}

MojErr MojDbStorageEngine::createDefaultEngine(MojRefCountedPtr<MojDbStorageEngine>& engineOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (!m_factory.get())
    {
        const char *default_engine = getenv("MOJODB_ENGINE");
        if (default_engine) // someone asked for specific engine through env variable
        {
            MojString key;
            MojErr err = key.assign(default_engine);
            MojErrCheck(err);
            if (!m_factories.get(key, m_factory))
            { MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage default engine not found")); }
        }
        else
        {
            if (m_factories.size() != 1) // if none or ambiguous
            {
                MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage engine is not set"));
            }
            // lets use that the only one which is present
            m_factory = *m_factories.begin();
        }
    }

   MojErr err = m_factory->create(engineOut);
   MojErrCheck(err);
   return MojErrNone;
}

MojErr MojDbStorageEngine::createEnv(MojRefCountedPtr<MojDbEnv>& envOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (!m_factory.get())
    {
        const char *default_engine = getenv("MOJODB_ENGINE");
        if(!default_engine)
        {
#ifdef LMDB_ENGINE_SUPPORT
            default_engine = "lmdb";
#else
            default_engine = "sandwich";
#endif
        }
        if (default_engine) // someone asked for specific engine through env variable
        {
            MojString key;
            MojErr err = key.assign(default_engine);
            MojErrCheck(err);
            if (!m_factories.get(key, m_factory))
            { MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage default engine not found")); }
        }
        else
        {
            if (m_factories.size() != 1) // if none or ambiguous
            {
                MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage engine is not set"));
            }
            // lets use that the only one which is present
            m_factory = *m_factories.begin();
        }
    }

   MojErr err = m_factory->createEnv(envOut);
   MojErrCheck(err);
   return MojErrNone;
}

MojErr MojDbStorageEngine::createEngine(const MojChar* name, MojRefCountedPtr<MojDbStorageEngine>& engineOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(name);

	// check default factory
	if (m_factory.get() && MojStrCmp(name, m_factory->name()) == 0) {
		MojErr err = m_factory->create(engineOut);
		MojErrCheck(err);
		return MojErrNone;
	}

	// lookup within all registered engine factories
	MojString key;
	MojErr err = key.assign(name);
	Factory factory;
	if (m_factories.get(key, factory))
	{
		err = factory->create(engineOut);
		MojErrCheck(err);
		return MojErrNone;
	}

	MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage engine not found: '%s'"), name);
}
MojErr MojDbStorageEngine::setEngineFactory(MojDbStorageEngineFactory* factory)
{
   m_factory.reset(factory);
   return MojErrNone;
}

MojErr MojDbStorageEngine::setEngineFactory(const MojChar *name)
{
	MojString key;
	MojErr err = key.assign(name);
	Factory factory;
	if (m_factories.get(key, factory))
	{
		m_factory = factory;
		return MojErrNone;
	}

	MojErrThrowMsg(MojErrDbStorageEngineNotFound, _T("Storage engine not found: '%s'"), name);
}

MojDbStorageEngine::MojDbStorageEngine()
{
}

MojErr MojDbStorageEngine::mountShard(MojDbShardId shardId, const MojString& databasePath)
{
    return MojErrNone;
}

MojErr MojDbStorageEngine::unMountShard(MojDbShardId shardId)
{
    return MojErrNone;
}

MojDbStorageTxn::MojDbStorageTxn()
: m_quotaEnabled(true),
  m_refreshQuotas(false),
  m_quotaEngine(NULL),
  m_preCommit(this),
  m_postCommit(this)
{
}

MojDbStorageTxn::~MojDbStorageTxn()
{
    for (auto monitor : m_monitors)
    {
	    MojErr err = monitor->destroy(*this);
	    MojErrCatchAll(err);
    }
}

MojErr MojDbStorageTxn::Monitor::committed(MojDbStorageTxn&) { return MojErrNone; }
MojErr MojDbStorageTxn::Monitor::destroy(MojDbStorageTxn&) { return MojErrNone; }
MojDbStorageTxn::Monitor::~Monitor() {}

MojErr MojDbStorageTxn::subscribe(MojDbStorageTxn::Monitor& monitor)
{
    MojErr err = m_monitors.put(&monitor);
    MojErrCheck(err);
    return MojErrNone;
}

MojErr MojDbStorageTxn::unsubscribe(MojDbStorageTxn::Monitor& monitor)
{
    bool found;
    MojErr err = m_monitors.del(&monitor, found);
    MojErrCheck(err);
    return MojErrNone;
}

MojErr MojDbStorageTxn::offsetQuota(MojInt64 offset)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (m_curQuotaOffset.get() && m_quotaEnabled) {
		MojErr err = m_curQuotaOffset->apply(offset);
		MojErrCheck(err);
	}
	return MojErrNone;
}

void MojDbStorageTxn::notifyPreCommit(CommitSignal::SlotRef slot)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	m_preCommit.connect(slot);
}

void MojDbStorageTxn::notifyPostCommit(CommitSignal::SlotRef slot)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	m_postCommit.connect(slot);
}

MojErr MojDbStorageTxn::commit()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = m_preCommit.fire(this);
	MojErrCheck(err);
	if (m_quotaEngine) {
		err = m_quotaEngine->applyUsage(this);
		MojErrCheck(err);
	}

	err = commitImpl();
	MojErrCheck(err);

	err = m_postCommit.fire(this);
	MojErrCheck(err);
	if (m_quotaEngine) {
		err = m_quotaEngine->applyQuota(this);
		MojErrCheck(err);
		if (m_refreshQuotas) {
			err = m_quotaEngine->refresh();
			MojErrCheck(err);
		}
	}

	for (auto monitor : m_monitors)
	{
		err = monitor->committed(*this);
		MojErrCheck(err);
	}

	return MojErrNone;
}
