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


#include "db/MojDbKindEngine.h"
#include "db/MojDb.h"
#include "db/MojDbKind.h"
#include "db/MojDbQuery.h"
#include "db/MojDbServiceDefs.h"

// prefixes
const MojChar* const MojDbKindEngine::KindIdPrefix = _T("_kinds/");
// db names
const MojChar* const MojDbKindEngine::KindsDbName = _T("kinds.db");
const MojChar* const MojDbKindEngine::IndexIdsDbName = _T("indexIds.db");
const MojChar* const MojDbKindEngine::IndexIdsSeqName = _T("indexId");
// Kind built-in
const MojChar* const MojDbKindEngine::KindKindId = _T("Kind:1");
const MojChar* const MojDbKindEngine::KindKindIdPrefix = _T("Kind:");
const MojChar* const MojDbKindEngine::KindKindJson =
	_T("{\"id\":\"Kind:1\",\"owner\":\"com.palm.admin\",")
	_T("\"indexes\":[{\"name\":\"_rev\",\"props\":[{\"name\":\"_rev\"}],\"incDel\":true},{\"name\":\"kindId\",\"props\":[{\"name\":\"id\"}]}]}");
// Object built-in
const MojChar* const MojDbKindEngine::RootKindId = _T("Object:1");
const MojChar* const MojDbKindEngine::RootKindJson =
	_T("{\"id\":\"Object:1\",\"owner\":\"com.palm.admin\",")
	_T("\"indexes\":[{\"name\":\"_sync_revIncDel\",\"props\":[{\"name\":\"_sync\", \"default\":false},{\"name\":\"_rev\"}],\"incDel\":true}]}");
// Rev-Time built-in
const MojChar* const MojDbKindEngine::RevTimestampId = _T("RevTimestamp:1");
const MojChar* const MojDbKindEngine::RevTimestampJson =
	_T("{\"id\":\"RevTimestamp:1\",\"owner\":\"com.palm.admin\",")
	_T("\"indexes\":[{\"name\":\"timestamp\",\"props\":[{\"name\":\"timestamp\"}]}]}");
// DB State built-in
const MojChar* const MojDbKindEngine::DbStateId = _T("DbState:1");
const MojChar* const MojDbKindEngine::DbStateJson =
	_T("{\"id\":\"DbState:1\",\"owner\":\"com.palm.admin\"}");
// Permission built-in
const MojChar* const MojDbKindEngine::PermissionId = _T("Permission:1");
const MojChar* const MojDbKindEngine::PermissionIdPrefix = _T("Permission:");
const MojChar* const MojDbKindEngine::PermissionJson =
	_T("{\"id\":\"Permission:1\",\"owner\":\"com.palm.admin\",\"sync\":true,")
	_T("\"indexes\":[{\"name\":\"permissionIdx\",\"props\":[{\"name\":\"object\"},{\"name\":\"type\"},{\"name\":\"caller\"}]}]}");
// Quota built-in
const MojChar* const MojDbKindEngine::QuotaId = _T("Quota:1");
const MojChar* const MojDbKindEngine::QuotaIdPrefix = _T("Quota:");
const MojChar* const MojDbKindEngine::QuotaJson =
	_T("{\"id\":\"Quota:1\",\"owner\":\"com.palm.admin\"}");

//db.kindEngine

MojDbKindEngine::MojDbKindEngine()
: m_db(NULL)
{
}

MojDbKindEngine::~MojDbKindEngine()
{
	(void) close();
}

MojErr MojDbKindEngine::open(MojDb* db, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojAssert(db);
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(db->isDbAppLockingEnabled(), db->m_schemaLock);
#else
	MojAssertWriteLocked(db->m_schemaLock);
#endif

	// open kind db and index seq
	m_db = db;
	MojDbStorageEngine* engine = db->storageEngine();
	MojDbStorageTxn* txn = req.txn();
	MojAssert(engine);
	MojErr err = engine->openDatabase(KindsDbName, txn, m_kindDb);
	MojErrCheck(err);
	err = engine->openDatabase(IndexIdsDbName, txn, m_indexIdDb);
	MojErrCheck(err);
	err = engine->openSequence(IndexIdsSeqName, txn, m_indexIdSeq);
	MojErrCheck(err);

	// built-in kinds
    if (m_db->isRootKindEnabled()) {
        // add 'Object:1' when flag is true
        err = addBuiltin(RootKindJson, req);
        MojErrCheck(err);
    }
	err = addBuiltin(KindKindJson, req);
	MojErrCheck(err);
	err = addBuiltin(RevTimestampJson, req);
	MojErrCheck(err);
	err = addBuiltin(DbStateJson, req);
	MojErrCheck(err);
	err = addBuiltin(PermissionJson, req);
	MojErrCheck(err);
	err = addBuiltin(QuotaJson, req);
	MojErrCheck(err);
	// built-in indexes
    if (db->isRootKindEnabled()) {
        err = setupRootKind();
        MojErrCheck(err);
    }
	// locale
	err = db->getLocale(m_locale, req);
	MojErrCheck(err);
	// load kinds from obj db
	err = loadKinds(req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::close()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = MojErrNone;

	if (isOpen()) {
		// close all kinds
		for (KindMap::ConstIterator i = m_kinds.begin(); i != m_kinds.end(); ++i) {
			MojErr errClose = (*i)->close();
			MojErrAccumulate(err, errClose);
		}
		m_kinds.clear();
		// close index seq/db
		MojErr errClose = m_indexIdSeq->close();
		MojErrAccumulate(err, errClose);
		m_indexIdSeq.reset();
		errClose = m_indexIdDb->close();
		MojErrAccumulate(err, errClose);
		m_indexIdDb.reset();
		// close kind db
		errClose = m_kindDb->close();
		MojErrAccumulate(err, errClose);
		m_kindDb.reset();
		m_locale.clear();
		m_db = NULL;
	}
	return err;
}

MojErr MojDbKindEngine::stats(MojObject& objOut, MojDbReq& req, bool verify, MojString *pKind)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());


	for (KindMap::ConstIterator i = m_kinds.begin(); i != m_kinds.end(); ++i) {
		MojObject kindAnalysis;
		MojSize usage = 0;
		if (pKind && *pKind != (*i)->id())
			continue;		// get stats for only one kind
		MojErr err = (*i)->stats(kindAnalysis, usage, req, verify);
		if (err != MojErrNone)
			continue;
		err = objOut.put((*i)->id(), kindAnalysis);
		if (err != MojErrNone)
			continue;		// go through all kinds, even if some have errors
	}
	return MojErrNone;
}

MojErr MojDbKindEngine::updateLocale(const MojChar* locale, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(locale);
	MojAssert(isOpen());
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif


	MojErr err = m_locale.assign(locale);
	MojErrCheck(err);
	for (KindMap::ConstIterator i = m_kinds.begin(); i != m_kinds.end(); ++i) {
		err = (*i)->updateLocale(locale, req);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDbKindEngine::update(MojObject* newObj, const MojObject* oldObj, MojDbReq& req, MojDbOp op, MojTokenSet& tokenSetOut, bool checkSchema)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
	MojAssert(newObj || oldObj);

	MojDbKind* oldKind = NULL;
	MojErr err = MojErrNone;
	if (oldObj) {
		err = getKind(*oldObj, oldKind);
		MojErrCheck(err);
	}
	MojDbKind* newKind = NULL;
	err = getKind(newObj ? *newObj : *oldObj, newKind);
	MojErrCheck(err);

	if (oldKind && oldKind != newKind) {
		// if the kind changes, remove this object from the old
		// kind and add it to the new kind
       err = oldKind->update(NULL, oldObj, OpDelete, req, checkSchema);
		MojErrCheck(err);
		err = newKind->update(newObj, NULL, OpCreate, req, checkSchema);
		MojErrCheck(err);
	} else {
       err = newKind->update(newObj, oldObj, op, req, checkSchema);
		MojErrCheck(err);
	}
	err = newKind->tokenSet(tokenSetOut);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::find(const MojDbQuery& query, MojDbCursor& cursor, MojDbWatcher* watcher, MojDbReq& req, MojDbOp op)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());

	MojErr err = query.validate();
	MojErrCheck(err);
    // In order to find collate index, cursor needs kindEngine
    cursor.kindEngine(this);
	err = cursor.init(query);
	MojErrCheck(err);

	MojDbKind* kind = NULL;
	err = getKind(query.from().data(), kind);
	MojErrCheck(err);
	MojAssert(kind);
	err = kind->find(cursor, watcher, req, op);
	MojErrCheck(err);
	cursor.kindEngine(this);

	return MojErrNone;
}

MojErr MojDbKindEngine::checkPermission(const MojString& id, MojDbOp op, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());

	MojDbKind* kind = NULL;
	MojErr err = getKind(id.data(), kind);
	MojErrCheck(err);
	err = kind->checkPermission(op, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::checkOwnerPermission(const MojString& id, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());

	MojDbKind* kind = NULL;
	MojErr err = getKind(id.data(), kind);
	MojErrCheck(err);
	err = kind->checkOwnerPermission(req);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::getKind(MojVector<MojObject>& kindsOut, const MojChar* kindId)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(isOpen());

    MojDbQuery query;
    MojErr err = query.from(kindId);
    MojErrCheck(err);
    MojDbCursor cursor;
    err = m_db->find(query, cursor);
    MojErrCheck(err);

    for (;;) {
        MojObject obj;
        bool found = false;
        err = cursor.get(obj, found);
        MojErrCheck(err);
        if (!found)
            break;
        err = kindsOut.push(obj);
        MojErrCheck(err);
    }
    err = cursor.close();
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbKindEngine::getKinds(MojVector<MojObject>& kindsOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbKind* rootKind = NULL;
	MojErr err = getKind(RootKindId, rootKind);
	MojErrCheck(err);

	err = rootKind->subKinds(kindsOut);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::putKind(const MojObject& obj, MojDbReq& req, bool builtin)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif


	// parse out id
	MojString id;
	MojErr err = obj.getRequired(MojDbServiceDefs::IdKey, id);
	MojErrCheck(err);

	KindMap::ConstIterator i = m_kinds.find(id);
	if (i == m_kinds.end()) {
		// new kind
		err = createKind(id, obj, req, builtin);
		MojErrCheck(err);
	} else {
		// existing kind
        LOG_DEBUG("[db_kindEngine] UpdatingKind: %s \n", id.data());
		err = (*i)->configure(obj, m_kinds, m_locale, req);
		MojErrCheck(err);
	}

    // TODO: remake to use signals
    if (!builtin && m_db->shardEngine() && m_db->shardEngine()->enabled()) {
        err = m_db->shardEngine()->putKindHash(id.data(), req);
        MojErrCheck(err);
    }

	return MojErrNone;
}

MojErr MojDbKindEngine::delKind(const MojString& id, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif


	KindMap::ConstIterator i = m_kinds.find(id);
	if (i != m_kinds.end()) {
		// notify kind obj
		MojErr err = (*i)->drop(req);
		MojErrCheck(err);
		// delete from map
		bool found = false;
		err = m_kinds.del(id, found);
		MojErrCheck(err);
		MojAssert(found);
	}
	return MojErrNone;
}

MojErr MojDbKindEngine::reloadKind(const MojString& id, const MojObject& kindObj, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif

	// delete old one
	bool found = false;
	MojErr err = m_kinds.del(id, found);
	MojErrCheck(err);
	// replace with new one
	err = createKind(id, kindObj, req);
	MojErrCheck(err);

	return MojErrNone;
}

MojDbPermissionEngine* MojDbKindEngine::permissionEngine()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	return m_db->permissionEngine();
}

MojDbQuotaEngine* MojDbKindEngine::quotaEngine()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	return m_db->quotaEngine();
}

MojErr MojDbKindEngine::tokenFromId(const MojChar* id, MojInt64& tokOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(id);

	MojDbKind* kind;
	MojErr err = getKind(id, kind);
	MojErrCheck(err);
	tokOut = kind->token();

	return MojErrNone;
}

MojErr MojDbKindEngine::idFromToken(MojInt64 tok, MojString& idOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	TokMap::ConstIterator i = m_tokens.find(tok);
	if (i == m_tokens.end())
		MojErrThrow(MojErrDbInvalidKindToken);
	idOut = i.value();

	return MojErrNone;
}

MojErr MojDbKindEngine::tokenSet(const MojChar* kindName, MojTokenSet& tokenSetOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojAssert(kindName);
	MojDbKind* kind = NULL;
	MojErr err = getKind(kindName, kind);
	MojErrCheck(err);
	MojAssert(kind);
	err = kind->tokenSet(tokenSetOut);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::setupRootKind()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif


	MojDbKind* kind = NULL;
	MojErr err = getKind(RootKindId, kind);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::addBuiltin(const MojChar* json, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif

	MojObject obj;
	MojErr err = obj.fromJson(json);
	MojErrCheck(err);
	err = putKind(obj, req, true);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::createKind(const MojString& id, const MojObject& obj, MojDbReq& req, bool builtin)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojRefCountedPtr<MojDbKind> kind(new MojDbKind(m_db->storageDatabase(), this, builtin));
	MojAllocCheck(kind.get());
	MojErr err = kind->init(id);
	MojErrCheck(err);
	err = kind->configure(obj, m_kinds, m_locale, req);
	MojErrCheck(err);
	// update kind map
	err = m_kinds.put(id, kind);
	MojErrCheck(err);
	err = m_tokens.put(kind->token(), id);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::loadKinds(MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(isOpen());
#ifdef LMDB_ENGINE_SUPPORT
	MojAssertWriteLocked(m_db->isDbAppLockingEnabled(), m_db->m_schemaLock);
#else
	MojAssertWriteLocked(m_db->m_schemaLock);
#endif
	MojDbQuery query;
	MojErr err = query.from(KindKindId);
	MojErrCheck(err);
	MojDbCursor cursor;
	err = m_db->find(query, cursor, req);
	MojErrCheck(err);

	for (;;) {
		MojObject obj;
		bool found = false;
		err = cursor.get(obj, found);
		MojErrCheck(err);
		if (!found)
			break;
		// load kind
		MojErr loadErr = err = putKind(obj, req);
		MojErrCatchAll(err) {
			MojString id;
			bool found = false;
			MojErr err = obj.get(MojDbServiceDefs::IdKey, id, found);
			MojErrCheck(err);
			MojString errStr;
			MojErrToString(loadErr, errStr);
            LOG_ERROR(MSGID_DB_KIND_ENGINE_ERROR, 2,
            		PMLOGKS("data", id.data()),
            		PMLOGKS("error", errStr.data()),
            		"error loading kind 'data' - 'error'");
		}
	}
	err = cursor.close();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::getKind(const MojObject& obj, MojDbKind*& kind)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojString kindName;
	bool found = false;
	MojErr err = obj.get(MojDb::KindKey, kindName, found);
	MojErrCheck(err);
	if (!found)
		MojErrThrow(MojErrDbKindNotSpecified);
	err = getKind(kindName.data(), kind);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDbKindEngine::getKind(const MojChar* kindName, MojDbKind*& kind)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	KindMap::ConstIterator iter = m_kinds.find(kindName);
	if (iter == m_kinds.end())
		MojErrThrowMsg(MojErrDbKindNotRegistered, _T("kind not registered: '%s'"), kindName);
	kind = iter.value().get();

	return MojErrNone;
}
MojErr MojDbKindEngine::getByOwner (const MojString& owner, MojVector<MojDbKind*>& vect)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojDbKind* kind;
    KindMap::ConstIterator iter = m_kinds.begin();

    while(iter != m_kinds.end())
    {
       kind = iter.value().get();
       ++iter;

       if(kind->isBuiltin())
           continue;

       if(kind->owner().compare(owner) == 0)
       {
           (void) vect.push(kind);
       }
    }

    return MojErrNone;
}

MojErr MojDbKindEngine::formatKindId(const MojChar* id, MojString& dbIdOut)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err = dbIdOut.format(_T("_kinds/%s"), id);
    MojErrCheck(err);

    return MojErrNone;
}
