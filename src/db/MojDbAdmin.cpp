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


#include "db/MojDb.h"
#include "db/MojDbServiceDefs.h"
#include "db/MojDbKind.h"
#include "core/MojJson.h"
#include "core/MojTime.h"
#include "core/MojObjectBuilder.h"

MojErr MojDb::stats(MojObject& objOut, MojDbReqRef req, bool verify, MojString *pKind)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = beginReq(req);
	MojErrCheck(err);
	err = m_kindEngine.stats(objOut, req, verify, pKind);
	MojErrCheck(err);
	err = req->end();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::quotaStats(MojObject& objOut, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojErr err = beginReq(req);
	MojErrCheck(err);
	err = m_quotaEngine.stats(objOut, req);
	MojErrCheck(err);
	err = req->end();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::updateLocale(const MojChar* locale, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojAssert(locale);

	MojErr err = beginReq(req, true);
	MojErrCheck(err);
	MojString oldLocale;
	err = getLocale(oldLocale, req);
	MojErrCheck(err);
	MojString newLocale;
	err = newLocale.assign(locale);
	MojErrCheck(err);
	MojErr updateErr = err = updateLocaleImpl(oldLocale, newLocale, req);
	MojErrCatchAll(err) {
		err = req->abort();
		MojErrCheck(err);
		err = m_kindEngine.close();
		MojErrCheck(err);
		MojDbReq openReq;
		err = beginReq(openReq, true);
		MojErrCheck(err);
		err = m_kindEngine.open(this, openReq);
		MojErrCheck(err);
		err = openReq.end();
		MojErrCheck(err);
		MojErrThrow(updateErr);
	}
	return MojErrNone;
}

MojErr MojDb::compact()
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojThreadGuard guard(m_compact_mutex, false);
    if(guard.tryLock())
    {
		MojErr err = requireOpen();
		MojErrCheck(err);

		LOG_DEBUG("[db_mojodb] compacting...");

		err = m_storageEngine->compact();
		MojErrCheck(err);

		LOG_DEBUG("[db_mojodb] compaction complete");
    }

	return MojErrNone;
}

MojErr MojDb::purgeStatus(MojObject& revOut, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	revOut = -1;
	MojErr err = beginReq(req);
	MojErrCheck(err);
    bool isFound;
	err = getState(LastPurgedRevKey, revOut, isFound, req);
	MojErrCheck(err);
	err = req->end();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::purge(MojUInt32& countOut, MojInt64 numDays, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if (!m_enablePurge) {
        return MojErrNone;
    }

	countOut = 0;
	if (numDays <= -1) {
		numDays = m_purgeWindow;
	}

    //mark obsolete shards records
    shardEngine()->purgeShardObjects(numDays);
#ifdef LMDB_ENGINE_SUPPORT
	MojErr err = beginReq(req, true);
#else
	MojErr err = beginReq(req);
#endif
	MojErrCheck(err);

    LOG_DEBUG("[db_mojodb] purging objects deleted more than %lld days ago...", numDays);

	MojTime time;
	err = MojGetCurrentTime(time);
	MojErrCheck(err);

	// store the revision number to current timestamp mapping
	MojObject revTimeMapping;
	MojInt64 rev;
#ifdef LMDB_ENGINE_SUPPORT
	err = nextId(rev, req->txn());
#else
	err = nextId(rev);
#endif
	MojErrCheck(err);
	err = revTimeMapping.put(RevNumKey, rev);
	MojErrCheck(err);
	err = revTimeMapping.put(TimestampKey, time.microsecs());
	MojErrCheck(err);
	err = revTimeMapping.putString(KindKey, MojDbKindEngine::RevTimestampId);
	MojErrCheck(err);

	err = putImpl(revTimeMapping, MojDbFlagNone, req);
	MojErrCheck(err);

	// find the revision number for numDays prior to now
	MojInt64 purgeTime = time.microsecs() - (MojTime::UnitsPerDay * numDays);
	MojDbQuery query;
	err = query.from(MojDbKindEngine::RevTimestampId);
	MojErrCheck(err);
	query.limit(1);
	err = query.where(TimestampKey, MojDbQuery::OpLessThanEq, purgeTime);
	MojErrCheck(err);
	err = query.order(TimestampKey);
	MojErrCheck(err);
	query.desc(true);

	MojDbCursor cursor;
	err = findImpl(query, cursor, NULL, req, OpDelete);
	MojErrCheck(err);
	bool found = false;
	MojObject obj;
	err = cursor.get(obj, found);
	MojErrCheck(err);
	err = cursor.close();
	MojErrCheck(err);

	MojUInt32 batchCount = 0;
	MojUInt32 totalCount = 0;

	while ((found))
	{
		// Do it in AutoBatchSize batches
		batchCount = 0;
		req->fixmode(true);		// purge even if index mis-matches
		err = purgeImpl(obj, batchCount, req);
		LOG_DEBUG("[db_mojodb] purge batch processed: batch: %d; total: %d; err = %d\n",
            batchCount, (totalCount + batchCount), err);
		MojErrCheck(err);
		totalCount += batchCount;
		countOut = totalCount;
		if (batchCount < AutoBatchSize)	// last batch
			break;
		err = commitBatch(req);
		MojErrCheck(err);
		continue;
	}

	// end request
	err = req->end();
	MojErrCheck(err);

	//compact db
	compact();

    LOG_DEBUG("[db_mojodb] purged %d objects", countOut);

	return MojErrNone;
}

MojErr MojDb::dump(const MojChar* path, MojUInt32& countOut, bool incDel, MojDbReqRef req, bool backup,
		MojUInt32 maxBytes, const MojObject* incrementalKey, MojObject* backupResponse)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(path);

	MojErr err = beginReq(req);
	MojErrCheck(err);

	if (!req->admin()) {
        LOG_ERROR(MSGID_DB_ADMIN_ERROR, 2,
        		PMLOGKS("data", req->domain().data()),
        		PMLOGKS("path", path),
        		"access denied: 'data' cannot dump db to path: 'path'");
		MojErrThrow(MojErrDbAccessDenied);
	}

	MojFile file;
	err = file.open(path, MOJ_O_WRONLY | MOJ_O_CREAT | MOJ_O_TRUNC, MOJ_S_IRUSR | MOJ_S_IWUSR);
	MojErrCheck(err);

	// write out kinds first, then existing objects, then deleted objects
	MojSize bytesWritten = 0;
	MojSize totalwarns = 0;
	MojSize newwarns = 0;
	MojDbQuery objQuery;
	MojVector<MojObject> kindVec;
	MojObject revParam = -1;
	MojObject delRevParam = -1;

	// if we were given an incremental key, pull out the revs now
	if (incrementalKey) {
		(void) incrementalKey->get(MojDbServiceDefs::RevKey, revParam);
		(void) incrementalKey->get(MojDbServiceDefs::DeletedRevKey, delRevParam);
	}

    // if 'enableRootKind' flag is true, find subkinds from root
    if (m_enableRootKind) {
        err = m_kindEngine.getKinds(kindVec);
        MojErrCheck(err);
    }
    else {
        err = m_kindEngine.getKind(kindVec, MojDbKindEngine::KindKindId);
        MojErrCheck(err);
        err = m_kindEngine.getKind(kindVec, MojDbKindEngine::PermissionId);
        MojErrCheck(err);
        err = m_kindEngine.getKind(kindVec, MojDbKindEngine::DbStateId);
        MojErrCheck(err);
        err = m_kindEngine.getKind(kindVec, MojDbKindEngine::RevTimestampId);
        MojErrCheck(err);
        err = m_kindEngine.getKind(kindVec, MojDbKindEngine::QuotaId);
        MojErrCheck(err);
    }

	// write kinds - if incremental, only write the kinds that have changed since the respective revs
	MojString countStr;
	for (MojVector<MojObject>::ConstIterator i = kindVec.begin(); i != kindVec.end(); ++i) {
		if (backup) {
			bool backupKind = false;
			i->get(MojDbKind::SyncKey, backupKind);
			if (!backupKind)
				continue;
			MojString id;
			err = i->getRequired(MojDbServiceDefs::IdKey, id);
			MojErrCheck(err);
			MojDbQuery countQuery;
			err = countQuery.from(id);
			MojErrCheck(err);
			MojDbCursor cursor;
			err = find(countQuery, cursor, req);
			MojErrCheck(err);
			MojUInt32 count = 0;
			err = cursor.count(count);
			MojErrCheck(err);
			if (count > 0) {
				if (i != kindVec.begin()) {
					err = countStr.appendFormat(_T(", "));
					MojErrCheck(err);
				}
				err = countStr.appendFormat("%s=%u", id.data(), count);
				MojErrCheck(err);
			}
		}
        // '_del'(DelKey) is only exist when
        // - enableRootKind : true
        // - enablePurge : true
        // - query must contain "purge":false option
        //
		bool deleted = false;
		i->get(DelKey, deleted);
		MojObject kindRev;
		err = i->getRequired(RevKey, kindRev);
		MojErrCheck(err);
		if ((deleted && kindRev > delRevParam) || (!deleted && kindRev > revParam)) {
			err = dumpObj(file, (*i), bytesWritten, maxBytes);
			MojErrCheck(err);
			countOut++;
		}
	}

	// dump all the non-deleted objects
    if (m_enableRootKind) {
        err = dumpImpl(file, backup, false, revParam, delRevParam, true, countOut, req, backupResponse, MojDbServiceDefs::RevKey, bytesWritten, newwarns, maxBytes);
        MojErrCheck(err);
        totalwarns += newwarns;
    }
    else {
        err = dumpImpl(file, backup, false, revParam, delRevParam, true, countOut, req, backupResponse, MojDbServiceDefs::RevKey, bytesWritten, newwarns, kindVec, maxBytes);
        MojErrCheck(err);
        totalwarns += newwarns;
    }
	// If we're supposed to include deleted objects, dump the deleted objects now.
	// There's a chance that we may have run out of space in our backup.  If that's the case,
	// we don't want to try to dump deleted objects - we can detect this by looking for the HasMoreKey
	if (incDel && backupResponse && !backupResponse->contains(MojDbServiceDefs::HasMoreKey)) {
        if (m_enableRootKind) {
            err = dumpImpl(file, backup, true, revParam, delRevParam, false, countOut, req, backupResponse, MojDbServiceDefs::DeletedRevKey, bytesWritten, newwarns, maxBytes);
            MojErrCheck(err);
        }
        else {
            err = dumpImpl(file, backup, true, revParam, delRevParam, false, countOut, req, backupResponse, MojDbServiceDefs::DeletedRevKey, bytesWritten, newwarns, kindVec, maxBytes);
            MojErrCheck(err);
        }
	}
	totalwarns += newwarns;

	// Add the Full and Version keys
	if (backup && backupResponse) {
		bool incremental = (incrementalKey != NULL);
		err = backupResponse->putBool(MojDbServiceDefs::FullKey, !incremental);
		MojErrCheck(err);
		err = backupResponse->put(MojDbServiceDefs::VersionKey, DatabaseVersion);
		MojErrCheck(err);
		err = backupResponse->put(MojDbServiceDefs::WarningsKey, (MojInt32)totalwarns);
		MojErrCheck(err);
		MojString description;
		err = description.format(_T("incremental=%u"), countOut);
		MojErrCheck(err);
		if (!countStr.empty()) {
			err = description.appendFormat(_T(", %s"), countStr.data());
			MojErrCheck(err);
		}
		err = backupResponse->put(_T("description"), description);
		MojErrCheck(err);
	}

	err = req->end();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::load(const MojChar* path, MojUInt32& countOut, MojUInt32 flags, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
	MojAssert(path);

	MojErr err = beginReq(req, true);
	MojErrCheck(err);

	MojFile file;
	err = file.open(path, MOJ_O_RDONLY);
	MojErrCheck(err);

	MojJsonParser parser;
	parser.begin();
	MojSize bytesRead = 0;
	MojObjectBuilder visitor;

	int total_mutexes, mutexes_free, mutexes_used, mutexes_used_highwater, mutex_regionsize;
	m_objDb->mutexStats(&total_mutexes, &mutexes_free, &mutexes_used, &mutexes_used_highwater, &mutex_regionsize);

    LOG_DEBUG("[db_mojodb] Starting load of %s, total_mutexes: %d, mutexes_free: %d, mutexes_used: %d, mutexes_used_highwater: %d, &mutex_regionsize: %d\n",
		path,	total_mutexes, mutexes_free, mutexes_used, mutexes_used_highwater, mutex_regionsize);

	int orig_mutexes_used = mutexes_used;

	struct timeval startTime = {0,0}, stopTime = {0,0};

	gettimeofday(&startTime, NULL);

	int total_transaction_time = 0;

	int total = 0;
	int transactions = 0;

	do {
		MojChar buf[MojFile::MojFileBufSize];
		err = file.read(buf, sizeof(buf), bytesRead);
		MojErrCheck(err);

		const MojChar* parseEnd = buf;
		while (parseEnd < (buf + bytesRead)) {
			err = parser.parseChunk(visitor, parseEnd, bytesRead - (parseEnd - buf), parseEnd);
			MojErrCheck(err);
			if (parser.finished()) {
				//store the object
				err = loadImpl(visitor.object(), flags, req);
				MojErrCheck(err);
				countOut++;
				parser.begin();
				visitor.reset();

				total++;

				if ((total % 10) == 0) {
					// For debugging mutex consumption during load operations, we periodically retrieve the mutex stats.
					m_objDb->mutexStats(&total_mutexes, &mutexes_free, &mutexes_used, &mutexes_used_highwater, &mutex_regionsize);

					LOG_DEBUG("[db_mojodb] Loading %s record %d, total_mutexes: %d, mutexes_free: %d, mutexes_used: %d, mutexes_used_highwater: %d, &mutex_regionsize: %d\n",
						path, total, total_mutexes, mutexes_free, mutexes_used, mutexes_used_highwater, mutex_regionsize);
				}

				// If a loadStepSize is configured, then break up the load into separate transactions.
				// This is intended to prevent run-away mutex consumption in some particular scenarios.
				// The transactions do not reverse or prevent mutex consumption, but seem to reduce the
				// growth and eventually cause it to level off.

				if ((m_loadStepSize > 0) && ((total % m_loadStepSize) == 0)) {
					// Close and reopen transaction, to prevent a very large transaction from building up.
					LOG_DEBUG("[db_mojodb] Loading %s record %d, closing and reopening transaction.\n", path, total);

					struct timeval transactionStartTime = {0,0}, transactionStopTime = {0,0};

					gettimeofday(&transactionStartTime, NULL);

					err = req->end();
					MojErrCheck(err);

					err = req->endBatch();
					MojErrCheck(err);

					req->beginBatch(); // beginBatch() invocation for first transaction happened in MojDbServiceHandlerBase::invokeImpl

					err = beginReq(req, true);
					MojErrCheck(err);

					gettimeofday(&transactionStopTime, NULL);

					long int elapsedTransactionTimeMS = (transactionStopTime.tv_sec - transactionStartTime.tv_sec) * 1000 +
								(transactionStopTime.tv_usec - transactionStartTime.tv_usec) / 1000;

					total_transaction_time += (int)elapsedTransactionTimeMS;

					transactions++;
				}
			}
		}
	} while (bytesRead > 0);

	err = parser.end(visitor);
	MojErrCheck(err);
	if (parser.finished()) {
		err = loadImpl(visitor.object(), flags, req);
		MojErrCheck(err);
		countOut++;
	} else if (bytesRead > 0) {
		MojErrThrow(MojErrJsonParseEof);
	}

	err = req->end();
	MojErrCheck(err);
	err = req->endBatch(); //double call for commit() isn't a problem
	MojErrCheck(err);

	gettimeofday(&stopTime, NULL);
	long int elapsedTimeMS = (stopTime.tv_sec - startTime.tv_sec) * 1000 +
				(stopTime.tv_usec - startTime.tv_usec) / 1000;

	m_objDb->mutexStats(&total_mutexes, &mutexes_free, &mutexes_used, &mutexes_used_highwater, &mutex_regionsize);

	LOG_DEBUG("[db_mojodb] Finished load of %s, total_mutexes: %d, mutexes_free: %d, mutexes_used: %d, mutexes_used_highwater: %d, &mutex_regionsize: %d\n",
		path, total_mutexes, mutexes_free, mutexes_used, mutexes_used_highwater, mutex_regionsize);

    LOG_DEBUG("[db_mojodb] Loaded %s with %d records in %ldms (%dms of that for %d extra transactions), consuming %d mutexes, afterwards %d are available out of %d\n",
		path, total, elapsedTimeMS, total_transaction_time, transactions, mutexes_used - orig_mutexes_used, mutexes_free, total_mutexes);

	return MojErrNone;
}

MojErr MojDb::updateLocaleImpl(const MojString& oldLocale, const MojString& newLocale, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	if (oldLocale != newLocale) {
		MojErr err = m_kindEngine.updateLocale(newLocale, req);
		MojErrCheck(err);
		err = updateState(LocaleKey, newLocale, req);
		MojErrCheck(err);
	}
	MojErr err = req.end();
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::dumpImpl(MojFile& file, bool backup, bool incDel, const MojObject& revParam, const MojObject& delRevParam, bool skipKinds, MojUInt32& countOut, MojDbReq& req,
        MojObject* response, const MojChar* keyName, MojSize& bytesWritten, MojSize& warns, MojVector<MojObject>& kindVec, MojUInt32 maxBytes)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    // Store all object to 'idSet'
    // Because we suppose that there is no root kind
    // idSet is treated like Object:1 kind
    MojSet<MojObject> idSet;
    for (MojVector<MojObject>::ConstIterator i = kindVec.begin(); i != kindVec.end(); ++i) {
        MojDbQuery query;
        MojString id;
        bool found;
        MojErr err = i->get(MojDbServiceDefs::IdKey, id, found);
        MojErrCheck(err);
        if (!found)
            continue;
        err = query.from(id);
        MojErrCheck(err);
        err = query.where(MojDb::DelKey, MojDbQuery::OpEq, incDel);
        MojErrCheck(err);

        MojDbCursor cursor;
        err = findImpl(query, cursor, NULL, req, OpRead);
        MojErrCheck(err);
        warns = 0;

        for(;;) {
            bool found = false;
            MojObject obj;
            err = cursor.get(obj, found);
            // So that we can get as much data as possible from a corrupt database
            // We simply skip ghost keys and continue
            if (err == MojErrInternalIndexOnFind) {
                warns++;
                continue;
            }
            MojErrCheck(err);
            if (!found)
                break;

            if (skipKinds) {
                MojString kind;
                err = obj.getRequired(KindKey, kind);
                MojErrCheck(err);
                if (kind == MojDbKindEngine::KindKindId) {
                    continue;
                }
            }
            // find and store 'ALL' object into the set (for removing duplication)
            err = idSet.put(obj);
            MojErrCheck(err);
        }
        err = cursor.close();
        MojErrCheck(err);
    }

    MojObject curRev;
    for (MojSet<MojObject>::ConstIterator i = idSet.begin(); i != idSet.end(); ++i) {
        // write out each object, if the backup is full, insert the appropriate incremental key
        MojErr err = dumpObj(file, (*i), bytesWritten, maxBytes);
        MojErrCatch(err, MojErrDbBackupFull) {
            if (response) {
                MojErr errBackup = MojErrNone;
                if (!curRev.undefined()) {
                    errBackup = insertIncrementalKey(*response, keyName, curRev);
                    MojErrCheck(errBackup);
                } else {
                    errBackup = insertIncrementalKey(*response, keyName, incDel ? delRevParam: revParam);
                    MojErrCheck(errBackup);
                }
                errBackup = handleBackupFull(revParam, delRevParam, *response, keyName);
                MojErrCheck(errBackup);
            }
            return MojErrNone;
        }
        MojErrCheck(err);
        err = (*i).getRequired(MojDb::RevKey, curRev);
        MojErrCheck(err);
        countOut++;
    }

    if (warns > 0) {
        LOG_WARNING(MSGID_MOJ_DB_ADMIN_WARNING, 1,
                PMLOGKFV("warn", "%d", (int)warns),
                "Finished Backup with 'warn' warnings");
    } else {
        LOG_DEBUG("[db_mojodb] Finished Backup with no warnings \n");
    }

    // construct the next incremental key
    if (response && !curRev.undefined()) {
        MojErr err = insertIncrementalKey(*response, keyName, curRev);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDb::dumpImpl(MojFile& file, bool backup, bool incDel, const MojObject& revParam, const MojObject& delRevParam, bool skipKinds, MojUInt32& countOut, MojDbReq& req,
		MojObject* response, const MojChar* keyName, MojSize& bytesWritten, MojSize& warns, MojUInt32 maxBytes)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// query for objects, adding the backup key and rev key if necessary
	MojDbQuery query;
	MojErr err = query.from(MojDbKindEngine::RootKindId);
	MojErrCheck(err);
	err = query.where(MojDb::DelKey, MojDbQuery::OpEq, incDel);
	MojErrCheck(err);
	if (backup) {
		err = query.where(MojDb::SyncKey, MojDbQuery::OpEq, true);
		MojErrCheck(err);
		if (incDel) {
			err = query.where(MojDb::RevKey, MojDbQuery::OpGreaterThan, delRevParam);
			MojErrCheck(err);
		} else {
			err = query.where(MojDb::RevKey, MojDbQuery::OpGreaterThan, revParam);
			MojErrCheck(err);
		}
	}

	MojDbCursor cursor;
	err = findImpl(query, cursor, NULL, req, OpRead);
	MojErrCheck(err);
	warns = 0;

	MojObject curRev;
	for(;;) {
		bool found = false;
		MojObject obj;
		err = cursor.get(obj, found);
		// So that we can get as much data as possible from a corrupt database
		// We simply skip ghost keys and continue
		if (err == MojErrInternalIndexOnFind) {
			warns++;
			continue;
		}
		MojErrCheck(err);
		if (!found)
			break;

		if (skipKinds) {
			MojString kind;
			err = obj.getRequired(KindKey, kind);
			MojErrCheck(err);
			if (kind == MojDbKindEngine::KindKindId) {
				continue;
			}
		}

		// write out each object, if the backup is full, insert the appropriate incremental key
		err = dumpObj(file, obj, bytesWritten, maxBytes);
		MojErrCatch(err, MojErrDbBackupFull) {
			if (response) {
				MojErr errBackup = MojErrNone;
				if (!curRev.undefined()) {
					errBackup = insertIncrementalKey(*response, keyName, curRev);
					MojErrCheck(errBackup);
				} else {
					errBackup = insertIncrementalKey(*response, keyName, incDel ? delRevParam: revParam);
					MojErrCheck(errBackup);
				}
				errBackup = handleBackupFull(revParam, delRevParam, *response, keyName);
				MojErrCheck(errBackup);
			}
			return MojErrNone;
		}
		MojErrCheck(err);
		err = obj.getRequired(MojDb::RevKey, curRev);
		MojErrCheck(err);
		countOut++;
	}

	err = cursor.close();
	MojErrCheck(err);

    if (warns > 0) {
        LOG_WARNING(MSGID_MOJ_DB_ADMIN_WARNING, 1,
            PMLOGKFV("warn", "%d", (int)warns),
            "Finished Backup with 'warn' warnings");
    } else {
        LOG_DEBUG("[db_mojodb] Finished Backup with no warnings \n");
    }

	// construct the next incremental key
	if (response && !curRev.undefined()) {
		err = insertIncrementalKey(*response, keyName, curRev);
		MojErrCheck(err);
	}
	return MojErrNone;
}

MojErr MojDb::dumpObj(MojFile& file, MojObject obj, MojSize& bytesWrittenOut, MojUInt32 maxBytes)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// remove the rev key before dumping the object
	bool found = false;
	MojErr err = obj.del(RevKey, found);
	MojErrCheck(err);

	MojString str;
	err = obj.toJson(str);
	MojErrCheck(err);
	err = str.append(_T("\n"));
	MojErrCheck(err);
	MojSize len = str.length() * sizeof(MojChar);
	// if writing this object will put us over the max length, throw an error
	if (maxBytes && bytesWrittenOut + len > maxBytes) {
		MojErrThrow(MojErrDbBackupFull);
	}
	err = file.writeString(str, bytesWrittenOut);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::handleBackupFull(const MojObject& revParam, const MojObject& delRevParam, MojObject& response, const MojChar* keyName)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// if we filled up the allocated space for our backup, we need to construct the appropriate
	// response with incremental key so that we are called back to continue where we left off
	MojString keyStr;
	MojErr err = keyStr.assign(keyName);
	MojErrCheck(err);
	MojObject incremental;
	(void) response.get(MojDbServiceDefs::IncrementalKey, incremental);

	err = response.put(MojDbServiceDefs::HasMoreKey, true);
	MojErrCheck(err);
	if (keyStr == MojDbServiceDefs::RevKey) {
		err = incremental.put(MojDbServiceDefs::DeletedRevKey, delRevParam);
		MojErrCheck(err);
	} else if (keyStr == MojDbServiceDefs::DeletedRevKey) {
		err = incremental.put(MojDbServiceDefs::RevKey, revParam);
		MojErrCheck(err);
	}
	err = response.put(MojDbServiceDefs::IncrementalKey, incremental);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::insertIncrementalKey(MojObject& response, const MojChar* keyName, const MojObject& curRev)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	// get the incremental key if it already exists
	MojObject incremental;
	(void) response.get(MojDbServiceDefs::IncrementalKey, incremental);

	MojErr err = incremental.put(keyName, curRev);
	MojErrCheck(err);
	err = response.put(MojDbServiceDefs::IncrementalKey, incremental);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr MojDb::loadImpl(MojObject& obj, MojUInt32 flags, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	bool found = false;
	MojString kindName;
	MojErr err = obj.get(MojDb::KindKey, kindName, found);
	MojErrCheck(err);
	if (!found)
		MojErrThrow(MojErrDbKindNotSpecified);

	bool deleted = false;
	obj.get(MojDb::DelKey, deleted);

	// when loading objects, if the object is deleted, call delKind/del
	// otherwise, call putKind, putPermissions or put depending on the kind
	if (deleted) {
		MojObject id;
		if (obj.get(MojDbServiceDefs::IdKey, id)) {
			if (kindName.startsWith(MojDbKindEngine::KindKindIdPrefix)) {
				err = delKind(id, found, flags, req);
				MojErrCheck(err);
			} else {
				err = del(id, found, flags, req);
				MojErrCheck(err);
			}
		}
	} else {
		if (kindName.startsWith(MojDbKindEngine::KindKindIdPrefix)) {
			err = putKind(obj, flags, req);
			MojErrCheck(err);
		} else if (kindName.startsWith(MojDbKindEngine::PermissionIdPrefix)) {
			err = putPermissions(&obj, &obj + 1, req);
			MojErrCatch(err, MojErrDbKindNotRegistered); // kind may not have been backed up
			MojErrCheck(err);
		} else {
           		err = putImpl(obj, flags, req, false /*checkSchema*/);
			MojErrCatch(err, MojErrDbKindNotRegistered); // kind may not have been backed up
			MojErrCheck(err);
		}
	}
	return MojErrNone;
}

MojErr MojDb::purgeImpl(MojObject& obj, MojUInt32& countOut, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojObject val;
	MojErr err = obj.getRequired(RevNumKey, val);
	MojErrCheck(err);
	MojObject timestamp;
	err = obj.getRequired(TimestampKey, timestamp);
	MojErrCheck(err);

	// purge all objects that were deleted on or prior to this rev num

	// query for objects in two passes - once where backup is true and once where backup is false
	MojDbQuery objQuery;
	err = objQuery.from(MojDbKindEngine::RootKindId);
	MojErrCheck(err);
	err = objQuery.where(DelKey, MojDbQuery::OpEq, true);
	MojErrCheck(err);
	err = objQuery.where(SyncKey, MojDbQuery::OpEq, true);
	MojErrCheck(err);
	err = objQuery.where(RevKey, MojDbQuery::OpLessThanEq, val);
	MojErrCheck(err);

	MojUInt32 backupCount = 0;
	req.autobatch(true);
	req.fixmode(true);
	objQuery.limit(AutoBatchSize);
	err = delImpl(objQuery, backupCount, req, MojDbFlagPurge);
	MojErrCheck(err);

	MojDbQuery objQuery2;
	err = objQuery2.from(MojDbKindEngine::RootKindId);
	MojErrCheck(err);
	err = objQuery2.where(DelKey, MojDbQuery::OpEq, true);
	MojErrCheck(err);
	err = objQuery2.where(SyncKey, MojDbQuery::OpEq, false);
	MojErrCheck(err);
	err = objQuery2.where(RevKey, MojDbQuery::OpLessThanEq, val);
	MojErrCheck(err);
	MojUInt32 count = 0;
	MojUInt32 batchRemain = 0;
	if (backupCount <= AutoBatchSize)
		batchRemain = AutoBatchSize - backupCount;
	req.autobatch(true);		// enable auto batch
	req.fixmode(true);		// force deletion of bad entries

	if (batchRemain > 0) {
		objQuery2.limit(batchRemain);
		err = delImpl(objQuery2, count, req, MojDbFlagPurge);
		MojErrCheck(err);
	}

	countOut = count + backupCount;

	req.autobatch(false);

	// if we actually deleted objects, store this rev num as the last purge rev
	if (countOut > 0) {
		err = updateState(LastPurgedRevKey, val, req);
		MojErrCheck(err);
	}

	// delete all the RevTimestamp entries prior to this one
	MojDbQuery revTimestampQuery;
	err = revTimestampQuery.from(MojDbKindEngine::RevTimestampId);
	MojErrCheck(err);
	err = revTimestampQuery.where(TimestampKey, MojDbQuery::OpLessThanEq, timestamp);
	MojErrCheck(err);

	count = 0;
	err = delImpl(revTimestampQuery, count, req, MojDbFlagPurge);
	MojErrCheck(err);


	return MojErrNone;
}
