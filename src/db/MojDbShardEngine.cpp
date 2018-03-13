// Copyright (c) 2013-2018 LG Electronics, Inc.
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

#include "db/MojDbShardEngine.h"
#include "db/MojDbKind.h"
#include "db/MojDb.h"
#include "db/MojDbServiceDefs.h"
#include "db/MojDbMediaLinkManager.h"
#include "core/MojDataSerialization.h"
#include <boost/crc.hpp>
#include <string>
#include <sys/statvfs.h>

using namespace std;

static const MojChar* const ShardInfoKind1Str =
    _T("{\"id\":\"ShardInfo1:1\",")
    _T("\"owner\":\"mojodb.admin\",")
    _T("\"indexes\":[ {\"name\":\"ShardId\",   \"props\":[ {\"name\":\"shardId\"} ]}, \
                      {\"name\":\"DatabasePath\",\"props\":[ {\"name\":\"databasePath\"} ]}, \
                      {\"name\":\"DeviceId\",  \"props\":[ {\"name\":\"deviceId\"} ]}, \
                      {\"name\":\"IdBase64\",  \"props\":[ {\"name\":\"idBase64\"} ]}, \
                      {\"name\":\"Active\",    \"props\":[ {\"name\":\"active\"} ]}, \
                      {\"name\":\"Transient\", \"props\":[ {\"name\":\"transient\"} ]}, \
                      {\"name\":\"Timestamp\", \"props\":[ {\"name\":\"timestamp\"} ]}, \
                      {\"name\":\"KindIds\", \"props\":[ {\"name\":\"kindIds\"} ]}\
                    ]}");

MojDbShardEngine::MojDbShardEngine(MojDb& db)
  :
#ifdef LMDB_ENGINE_SUPPORT
    m_db(db),
    m_enable(false)
#else
    m_db(db)
#endif
{
}

MojDbShardEngine::~MojDbShardEngine(void)
{
}

/**
 * initialize MojDbShardEngine
 *
 * @param ip_db
 *   pointer to MojDb instance
 *
 * @param io_req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::init (const MojObject& conf, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojErr err;
    MojObject obj;

    m_cache.clear();

    err = configure(conf);
    MojErrCheck(err);

    // add type
    err = obj.fromJson(ShardInfoKind1Str);
    MojErrCheck(err);

	MojDbAdminGuard admin(req);
    err = m_db.kindEngine()->putKind(obj, req, true); // add builtin kind
    MojErrCheck(err);

    err = KindHash::registerKind(&m_db, req);
    MojErrCheck(err);

    //all devices should not be active at startup
    err = resetShards(req);
    MojErrCheck(err);

    err = initCache(req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::configure(const MojObject& conf)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojString mediaLinkDirectory;

    if (!conf.get(_T("enable_sharding"), m_enable)) {
        m_enable = false;
        return MojErrNone;
    }

    err = conf.getRequired(_T("shard_db_prefix"), m_databasePrefix);
    MojErrCheck(err);

    err = conf.getRequired(_T("device_links_path"), mediaLinkDirectory);
    MojErrCheck(err);

    err = m_mediaLinkManager.setLinkDirectory(mediaLinkDirectory);
    MojErrCheck(err);

    if (m_databasePrefix.empty()) {
        MojErrThrow (MojErrRequiredPropNotFound);
    }

    if (m_databasePrefix.at(0) == _T('/')) {
        m_databasePrefixIsAbsolute = true;
    } else {
        m_databasePrefixIsAbsolute = false;
    }

    err = conf.getRequired(_T("fallback_path"), m_fallbackPath);
    MojErrCheck(err);

    MojObject val;
    err = conf.getRequired(_T("device_minimum_free_bytes"), val);
    MojErrCheck(err);

    m_reqFreePartSpaceBytes = val.intValue();

    if (conf.get(_T("device_minimum_free_percentage"), val)) {
        MojDecimal dec = val.decimalValue();
        m_reqFreePartSpacePercantage = float(dec.floatValue());

        if (m_reqFreePartSpacePercantage >= 100.0f)
        {
            m_enable = false;
            return MojErrNone;
        }
        else if (m_reqFreePartSpacePercantage < 0.0f)
        {
            MojErrThrow(MojErrInvalidArg);
        }
    } else {
        m_reqFreePartSpacePercantage = 0.0f;
    }

    return MojErrNone;
}

/**
 * all devices should not be active at startup:
 * - reset 'active flag'
 * - reset 'mountPath'
 *
 * @param io_req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::resetShards (MojDbReq& io_req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojDbQuery query;
    MojDbCursor cursor;
    MojObject props;
    MojUInt32 count;

    MojErr err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);

    err = props.put(_T("active"), false);
    MojErrCheck(err);
    err = props.put(_T("mountPath"), MojString());
    MojErrCheck(err);

	MojDbAdminGuard admin(io_req);
    err = m_db.merge(query, props, count, MojDbFlagNone, io_req);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * put a new shard description to db
 *
 * @param shardInfo
 *   device info to store
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::put (const MojDbShardInfo& shardInfo, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(shardInfo.id);

    MojObject obj;
    MojErr err;

    err = obj.putString(_T("_kind"), _T("ShardInfo1:1"));
    MojErrCheck(err);

    MojDbShardInfo info = shardInfo;
    updateTimestamp(info);

    if (info.id_base64.empty())
    {
        err = MojDbShardEngine::convertId(info.id, info.id_base64);
        MojErrCheck(err);
    }

    err = convert(info, obj);
    MojErrCheck(err);

	MojDbAdminGuard admin(req);
    err = m_db.put(obj, MojDbFlagNone, req);
    MojErrCheck(err);

    m_cache.put(shardInfo.id, obj);

    return MojErrNone;
}

/**
 * get shard description by id
 *
 * @param shardId
 *   device id
 *
 * @param shardInfo
 *   device info, initialized if device found by id
 *
 * @param found
 *   true if found
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::get (MojUInt32 shardId, MojDbShardInfo& shardInfo, bool& found)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojObject dbObj;

    found = m_cache.get(shardId, dbObj);

    if (found)
    {
        err = convert(dbObj, shardInfo);
        MojErrCheck(err);
    }

    return MojErrNone;
}

/**
 * get list of all active shards
 *
 * @param shardInfoList
 *   list of device info collect all shards with state 'active'==true
 *
 * @param count
 *   number of devices info added
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::getAllActive (std::list<MojDbShardInfo>& shardInfoList, MojUInt32& count, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    MojDbQuery query;
    MojDbCursor cursor;
    MojObject obj(true);
    MojDbShardInfo shardInfo;

    err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);
    err = query.where(_T("active"), MojDbQuery::OpEq, obj);
    MojErrCheck(err);

    MojDbAdminGuard admin(req);
    err = m_db.find(query, cursor, req);
    MojErrCheck(err);

    count = 0;
    shardInfoList.clear();

    while (true)
    {
        bool found;
        MojObject dbObj;

        err = cursor.get(dbObj, found);
        MojErrCheck(err);
        if (!found)
            break;

        err = convert(dbObj, shardInfo);
        MojErrCheck(err);

        shardInfoList.push_back(shardInfo);
        ++count;
    }

    return MojErrNone;
}

/**
 * update shardInfo
 *
 * @param i_shardInfo
 *   update device info properties (search by ShardInfo.id)
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::update (const MojDbShardInfo& i_shardInfo, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojErr err;

    MojDbQuery query;
    MojObject dbObj;
    MojObject obj(i_shardInfo.id);

    err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);
    err = query.where(_T("shardId"), MojDbQuery::OpEq, obj);
    MojErrCheck(err);

    MojObject update;
    MojUInt32 count = 0;

    MojDbShardInfo shardInfo = i_shardInfo;
    updateTimestamp(shardInfo);

    err = convert(shardInfo, update);
    MojErrCheck(err);

	MojDbAdminGuard admin(req);
    err = m_db.merge(query, update, count, MojDbFlagNone, req);
    MojErrCheck(err);

    if (count == 0)
        return MojErrDbObjectNotFound;

    m_cache.update(i_shardInfo.id, update);

    return MojErrNone;
}

/**
 * get shard description by uuid
 *
 * @param deviceUuid
 *   uuid
 *
 * @param shardInfo
 *   device info (initialized if found)
 *
 * @param found
 *   true if found
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::getByDeviceUuid (const MojString& deviceUuid, MojDbShardInfo& shardInfo, bool& found, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojErr err;

    //get record from db, extract id
    MojDbQuery query;
    MojDbCursor cursor;
    MojObject obj(deviceUuid);
    MojObject dbObj;

    err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);
    err = query.where(_T("deviceId"), MojDbQuery::OpEq, obj);
    MojErrCheck(err);

	MojDbAdminGuard admin(req);
    err = m_db.find(query, cursor, req);
    MojErrCheck(err);

    err = cursor.get(dbObj, found);
    MojErrCheck(err);

    if (found)
        convert(dbObj, shardInfo);

    return MojErrNone;
}

/**
 * get device id by uuid
 *
 * search within db for i_deviceId, return id if found
 * else
 * allocate a new id
 *
 * @param deviceUuid
 *   device uuid
 *
 * @param shardId
 *   device id
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::getShardId (const MojString& deviceUuid, MojUInt32& shardId)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojErr err;
    MojDbShardInfo shardInfo;
    bool found;

    err = getByDeviceUuid(deviceUuid, shardInfo, found);
    MojErrCheck(err);
    if (found)  {
        shardId = shardInfo.id;
        LOG_DEBUG("[db_shardEngine] Shard id for device %s is %d", deviceUuid.data(), shardId);
    } else {
        LOG_DEBUG("[db_shardEngine] Shard id for device %s not found, generating it", deviceUuid.data());
        err = allocateId(deviceUuid, shardId);
        MojErrCheck(err);
    }

    return MojErrNone;
}

/**
 * compute a new shard id
 *
 * @param deviceUuid
 *   device uuid
 *
 * @param shardId
 *   device id
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::allocateId (const MojString& deviceUuid, MojUInt32& shardId)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojUInt32 id;
    MojUInt32 calc_id;
    MojUInt32 prefix = 1;
    MojUInt32 suffix = 1;
    bool found = false;

    err = computeId(deviceUuid, calc_id);
    MojErrCheck(err);

    do
    {
        id = calc_id | (prefix * 0x01000000);

        //check the table to see if this ID already exists
        err = isIdExist(id, found);
        MojErrCheck(err);

        if (found) {
            LOG_WARNING(MSGID_DB_SHARDENGINE_WARNING, 2,
                PMLOGKFV("id", "%x", id),
                PMLOGKFV("prefix", "%u", prefix),
                "id generation -> 'id' exist already, prefix = 'prefix'");
            prefix++;
        } else {
            MojAssert(id);

            shardId = id;
            break;  // exit from loop
        }

        if (prefix == 128)
        {
            LOG_WARNING(MSGID_DB_SHARDENGINE_WARNING, 1,
                        PMLOGKFV("prefix", "%u", prefix),
                        "id generation -> next iteration");
            prefix = 1;
            MojString modified_uuid;
            modified_uuid.format("%s%x", deviceUuid.data(), ++suffix);
            computeId(modified_uuid, calc_id); //next iteration
        }
    }
    while (!found);

    return MojErrNone;
}

/**
 * is device id exist?
 *
 * @param shardId
 *   device id
 *
 * @param found
 *   true, if found
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::isIdExist (MojUInt32 shardId, bool& found)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    found = m_cache.isExist(shardId);
    return MojErrNone;
}

/**
 * compute device id by media uuid
 *
 * @param mediaUuid
 *   media uuid
 *
 * @param shardId
 *   device id
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::computeId (const MojString& mediaUuid, MojUInt32& sharId)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojAssert(!mediaUuid.empty());

    std::string str = mediaUuid.data();

    //Create a 24 bit hash of the string
    boost::crc_32_type result;
    result.process_bytes(str.data(), str.length());
    MojInt32 code = result.checksum();
    result.reset();

    //Prefix the 24 bit hash with 0x01 to create a 32 bit unique shard ID
    sharId = code & 0xFFFFFF;

    return MojErrNone;
}

/**
 * convert device id to base64 string
 *
 * @param i_id
 *   device id
 *
 * @param o_id_base64
 *   device id converted to base64 string
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::convertId (const MojUInt32 i_id, MojString& o_id_base64)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojBuffer buf;
    MojDataWriter writer(buf);

    err = writer.writeUInt32(i_id);
    MojErrCheck(err);

    MojVector<MojByte> byteVec;
    err = buf.toByteVec(byteVec);
    MojErrCheck(err);
    MojString str;
    err = o_id_base64.base64Encode(byteVec, false);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * convert base64 string to device id
 *
 * @param i_id_base64
 *   device id converted to base64 string
 *
 * @param o_id
 *   device id
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::convertId (const MojString& i_id_base64, MojUInt32& o_id)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;
    MojVector<MojByte> idVec;
    err = i_id_base64.base64Decode(idVec);
    MojErrCheck(err);

    // extract first 32bits of _id as shard id in native order
    MojDataReader reader(idVec.begin(), idVec.size());

    err = reader.readUInt32(o_id);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * convert device info to MojObject
 *
 * @param i_shardInfo
 *   device info
 *
 * @param o_obj
 *   MojObject
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::convert (const MojDbShardInfo& i_shardInfo, MojObject& o_obj)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojObject obj1(i_shardInfo.id);
    MojErr err = o_obj.put(_T("shardId"), obj1);
    MojErrCheck(err);

    MojObject obj2(i_shardInfo.active);
    err = o_obj.put(_T("active"), obj2);
    MojErrCheck(err);

    err = o_obj.put(_T("transient"), i_shardInfo.transient);
    MojErrCheck(err);

    MojObject obj3(i_shardInfo.id_base64);
    err = o_obj.put(_T("idBase64"), obj3);
    MojErrCheck(err);

    MojObject obj4(i_shardInfo.deviceId);
    err = o_obj.put(_T("deviceId"), obj4);
    MojErrCheck(err);

    MojObject obj5(i_shardInfo.deviceUri);
    err = o_obj.put(_T("deviceUri"), obj5);
    MojErrCheck(err);

    MojObject obj6(i_shardInfo.mountPath);
    err = o_obj.put(_T("mountPath"), obj6);
    MojErrCheck(err);

    err = o_obj.put(_T("databasePath"), i_shardInfo.databasePath);
    MojErrCheck(err);

    MojObject obj7(i_shardInfo.deviceName);
    err = o_obj.put(_T("deviceName"), obj7);
    MojErrCheck(err);

    MojObject obj8(i_shardInfo.timestamp);
    err = o_obj.put(_T("timestamp"), obj8);
    MojErrCheck(err);

    err = o_obj.putString(_T("parentDeviceId"), i_shardInfo.parentDeviceId);
    MojErrCheck(err);

    //convert kindIds
    MojString strKindIds;
    err = i_shardInfo.kindIds.toString(strKindIds);
    MojErrCheck(err);
    MojObject obj9(strKindIds);
    err = o_obj.put(_T("kindIds"), obj9);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * convert MojObject to device info
 *
 * @param i_obj
 *   MojObject, input
 *
 * @param o_shardInfo
 *   device info
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::convert (const MojObject& i_obj, MojDbShardInfo& o_shardInfo)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err = i_obj.getRequired(_T("shardId"), o_shardInfo.id);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("idBase64"), o_shardInfo.id_base64);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("active"), o_shardInfo.active);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("transient"), o_shardInfo.transient);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("deviceId"), o_shardInfo.deviceId);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("deviceUri"), o_shardInfo.deviceUri);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("mountPath"), o_shardInfo.mountPath);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("databasePath"), o_shardInfo.databasePath);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("deviceName"), o_shardInfo.deviceName);
    MojErrCheck(err);

    err = i_obj.getRequired(_T("timestamp"), o_shardInfo.timestamp);
    MojErrCheck(err);

	err = i_obj.getRequired(_T("parentDeviceId"), o_shardInfo.parentDeviceId);
	MojErrCheck(err);

    MojString strKindIds;
    err = i_obj.getRequired(_T("kindIds"), strKindIds);
    MojErrCheck(err);
    err = o_shardInfo.kindIds.fromString(strKindIds);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * Support garbage collection of obsolete shards
 * remove shard objects older <numDays> days
 *
 * @param numDays
 *   days
 *
 * @param req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::purgeShardObjects (MojInt64 numDays, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojDbQuery query1, query2;
    MojDbCursor cursor;
    MojInt32 value_id = 0;
    MojInt64 value_timestamp;
    MojObject obj(value_id);
    MojObject dbObj;
    MojObject obj_active(false);
    MojVector<MojUInt32> arrShardIds;
    MojString shardIdStr;
    bool found;
    bool value_active;

    MojTime time;
    MojErrCheck( MojGetCurrentTime(time) );
    MojInt64 purgeTime = time.microsecs() - (MojTime::UnitsPerDay * numDays);
    LOG_DEBUG("[db_shardEngine] purging objects for shards inactive for more than %lld days...", numDays);

    //collect 'old' shards
    //--------------------
    MojErr err = query1.from(_T("ShardInfo1:1"));
    MojErrCheck(err);
    err = query1.where(_T("timestamp"), MojDbQuery::OpLessThanEq, purgeTime);
    MojErrCheck(err);
    query1.setIgnoreInactiveShards(false);

	MojDbAdminGuard admin(req);
    err = m_db.find(query1, cursor, req);
    MojErrCheck(err);

    while (true)
    {
        err = cursor.get(dbObj, found);
        MojErrCheck(err);
        if(!found)
            break;

        err = dbObj.getRequired(_T("shardId"), value_id);
        MojErrCheck(err);
        err = MojDbShardEngine::convertId(value_id, shardIdStr);
        MojErrCheck(err);
        err = dbObj.getRequired(_T("active"), value_active);
        MojErrCheck(err);

        if(!value_active)
        {
            err = arrShardIds.pushUnique(value_id);
            MojErrCheck(err);
            LOG_DEBUG("[db_shardEngine] Need to purge records for old shard: [%s]", shardIdStr.data());
        } else { // TODO: Remove
            LOG_DEBUG("[db_shardEngine] Ignore active shard: [%s]", shardIdStr.data());
        }

    }

    MojErrCheck(cursor.close());

    removeShardObjects(arrShardIds, req);
    LOG_DEBUG("[db_shardEngine] Ended");

    return MojErrNone;
}

/**
 * removeShardObjects
 *
 * @param strShardIdToRemove
 *   device id
 *
 * @param req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::removeShardObjects (const MojString& strShardIdToRemove, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojUInt32 shardId;

    MojVector<MojUInt32> shardIds;
    MojErr err = MojDbShardEngine::convertId(strShardIdToRemove, shardId);
    MojErrCheck(err);
    err = shardIds.push(shardId);
    MojErrCheck(err);
    LOG_DEBUG("[db_shardEngine] purging objects for shard: %s", strShardIdToRemove.data());

    return(removeShardObjects(shardIds, req));
}

/**
 * removeShardObjects
 *
 * @param shardId
 *   device id
 *
 * @param req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::removeShardObjects (const MojVector<MojUInt32>& arrShardIds, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

	MojDbAdminGuard admin(req);
    if (arrShardIds.size() > 0)
    {
        MojDbShardInfo info;
        bool foundOut;
        MojErr err;
        MojDbKind* pKind;
        for (MojVector<MojUInt32>::ConstIterator itShardId = arrShardIds.begin();
                itShardId != arrShardIds.end();
                ++itShardId)
            {
                //get shard info structure
                err = get(*itShardId, info, foundOut);
                MojErrCheck(err);

                if(foundOut)
                {
                    //iterate over kindIds array
                    for (std::list<MojString>::iterator itKindId = info.kindIds.begin(); itKindId != info.kindIds.end(); ++itKindId)
                    {
                        //verify kind for 'built-in' flag
                        err = m_db.kindEngine()->getKind((*itKindId).data(), pKind);
                        MojErrCheck(err);

                        if(pKind->isBuiltin())
                            continue;

                        LOG_DEBUG("[db_shardEngine] Get next shard for %s", (*itKindId).data()); // TODO: to debug
                        err = removeShardKindObjects(*itShardId, *itKindId, req);
                        MojErrCheck(err);
                    }
                }
            }

            LOG_DEBUG("[db_shardEngine] Returned from removeShardObjects"); // TODO: to debug
    }

    return MojErrNone;
}

/**
 * removeShardRecords
 *
 * @param shardIdStr
 *   device id
 *
 * @param kindId
 *   kind id
 *
 * @param req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::removeShardKindObjects (const MojUInt32 shardId, const MojString& kindId, MojDbReq& req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    // make query
    bool found;
    uint32_t countDeleted = 0;
    uint32_t countRead = 0;
    MojDbQuery query;
    MojErr err = query.from(kindId);
    MojErrCheck(err);
    query.setIgnoreInactiveShards(false);

    MojDbCursor cursor;
	MojDbAdminGuard admin(req);
    err = m_db.find(query, cursor, req);
    MojErrCheck(err);

    MojString shardIdStr;
    err = MojDbShardEngine::convertId(shardId, shardIdStr);
    MojErrCheck(err);

    LOG_DEBUG("[db_shardEngine] purging objects for shard: [%s], Kind: [%s]", shardIdStr.data(), kindId.data());// todo: convert to Info

    MojObject record;
    MojObject recordId;
    MojUInt32 cmpShardId;

    while(true)
    {
        err = cursor.get(record, found);
        MojErrCheck(err);

        if (!found)
            break;

        err = record.getRequired(MojDb::IdKey, recordId);
        MojErrCheck(err);
        countRead ++;

        err = MojDbIdGenerator::extractShard(recordId, cmpShardId);
        MojErrCheck(err);

        if (cmpShardId != shardId)
            continue;

        err = m_db.del(recordId, found, MojDbFlagNone, req);
        MojErrCheck(err);
        countDeleted++;
    }

    if (countDeleted) {
        LOG_DEBUG("[db_shardEngine] purged %d of %d objects for shard: [%s] from Kind: [%s]", countDeleted, countRead, shardIdStr.data(), kindId.data());
    } else {
        LOG_DEBUG("[db_shardEngine] none purged out of %d objects", countRead); // todo: convert to Info
    }

    return MojErrNone;
}

/**
 * update ShardInfo::timestamp with current time value
 */
MojErr MojDbShardEngine::updateTimestamp (MojDbShardInfo& shardInfo)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    MojTime time;
    MojErrCheck( MojGetCurrentTime(time) );
    shardInfo.timestamp = time.microsecs();
    return MojErrNone;
}

/**
 * init cache
 *
 * @param io_req
 *   batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::initCache (MojDbReq& io_req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojDbQuery query;
    MojDbCursor cursor;
    MojInt32 value_id = 0;
    MojObject obj(value_id);
    MojObject dbObj;
    bool found;

    MojErr err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);

	MojDbAdminGuard admin(io_req);
    err = m_db.find(query, cursor, io_req);
    MojErrCheck(err);

    while (true)
    {
        err = cursor.get(dbObj, found);
        MojErrCheck(err);
        if(!found)
            break;

        err = dbObj.getRequired(_T("shardId"), value_id);
        MojErrCheck(err);
        m_cache.put(value_id, dbObj);
    }

    MojErrCheck(cursor.close());

    return MojErrNone;
}

MojErr MojDbShardEngine::linkShardAndKindId (const MojString& shardIdBase64, const MojString& kindId, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if(shardIdBase64.empty())
    {
        LOG_DEBUG("[db_shardEngine] link shard and kind: empty shardId");
        return MojErrNone;
    }

    MojErr err;
    MojUInt32 id;
    err = convertId(shardIdBase64, id);
    MojErrCheck(err);
    err = linkShardAndKindId(id, kindId, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::linkShardAndKindId (const MojUInt32 shardId, const MojString& kindId, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if(kindId.empty())
    {
        LOG_DEBUG("[db_shardEngine] link shard and kind: empty kindId");
        return MojErrNone;
    }

    bool found;
    MojDbShardInfo shardInfo;
    MojErr err;

    err = get(shardId, shardInfo, found);
    MojErrCheck(err);

    if(!found)
        return MojErrNone;

    if(shardInfo.kindIds.isExist(kindId))
        return MojErrNone;

    //link shard and kindId
    shardInfo.kindIds.add(kindId);

    //update db
    err = update(shardInfo, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::unlinkShardAndKindId (const MojString& shardIdBase64, const MojString& kindId, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if(shardIdBase64.empty())
    {
        LOG_DEBUG("[db_shardEngine] link shard and kind: empty shardId");
        return MojErrNone;
    }

    MojErr err;
    MojUInt32 id;
    err = convertId(shardIdBase64, id);
    MojErrCheck(err);
    err = unlinkShardAndKindId(id, kindId, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::unlinkShardAndKindId (const MojUInt32 shardId, const MojString& kindId, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    if(kindId.empty())
    {
        LOG_DEBUG("[db_shardEngine] link shard and kind: empty kindId");
        return MojErrNone;
    }

    bool found;
    MojDbShardInfo shardInfo;
    MojErr err;

    err = get(shardId, shardInfo, found);
    MojErrCheck(err);

    if(!found)
        return MojErrNone;

    if(!shardInfo.kindIds.isExist(kindId))
        return MojErrNone;

    //unlink shard and kindId
    shardInfo.kindIds.remove(kindId);

    //update db
    err = update(shardInfo, req);
    MojErrCheck(err);

    return MojErrNone;
}

/**
 * removeShardInfo record
 *
 * @param shardIdStr
 * device id
 *
 * @param kindId
 * kind id
 *
 * @param req
 * batch support
 *
 * @return MojErr
 */
MojErr MojDbShardEngine::removeShardInfo (const MojUInt32 shardId, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojUInt32 count;
    MojDbQuery query;
    MojObject obj_id(shardId);

    MojErr err = query.from(_T("ShardInfo1:1"));
    MojErrCheck(err);
    err = query.where(_T("shardId"), MojDbQuery::OpEq, obj_id);
    MojErrCheck(err);

	MojDbAdminGuard admin(req);
    err = m_db.del(query, count, MojDbFlagNone, req);
    MojErrCheck(err);

    m_cache.del(shardId);

    return MojErrNone;
}

MojErr MojDbShardEngine::processShardInfo(const MojDbShardInfo& shardInfo, MojDbShardInfo* databaseShardInfo, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);
    LOG_DEBUG("[db_shardEngine] Shard engine notified about new shard");

    MojErr err;
    bool found;

    // Inside shardInfo we have only filled deviceId deviceUri mountPath MojString deviceName
    // Note: we'll use short transaction here if we are not in an exclusive access already
    err = getByDeviceUuid(shardInfo.deviceId, *databaseShardInfo, found, req->schemaLocked() ? req : MojDbReqRef(MojDbReq()));
    MojErrCheck(err);

    copyRequiredFields(shardInfo, *databaseShardInfo); // fill/update databaseShardInfo

    if (!found) { // initialize new shardInfo
        err = allocateId(shardInfo.deviceId, databaseShardInfo->id);
        MojErrCheck(err);
        LOG_DEBUG("[db_shardEngine] shardEngine for device %s generated shard id: %d", databaseShardInfo->deviceId.data(), databaseShardInfo->id);

        databaseShardInfo->parentDeviceId = shardInfo.parentDeviceId;
        databaseShardInfo->deviceId = shardInfo.deviceId;
        databaseShardInfo->transient = false;
    }

    MojErr accErr = MojErrNone;

    err = req->begin(&m_db, true);
    MojErrCheck(err);

    // update shardInfo and mounted shards atomically
    // note that shard mount/unmount requires exclusive lock
    if (databaseShardInfo->active) {
        err = databasePrepare(databaseShardInfo, req);
        MojErrCheck(err);

        err = req->begin(&m_db, true); // lock schema for write
        MojErrCheck(err);

        err = m_db.storageEngine()->mountShard(databaseShardInfo->id, databaseShardInfo->databasePath);
        MojErrCheck(err);
    } else {
        err = m_db.storageEngine()->unMountShard(databaseShardInfo->id);
        MojErrAccumulate(accErr, err);
    }

    err = req->end(true); // unlock and commit immediately in case of success
    MojErrCheck(err);

    err = m_mediaLinkManager.processShardInfo(*databaseShardInfo);
    MojErrAccumulate(accErr, err);

    if (found) {
        if (!databaseShardInfo->active && databaseShardInfo->transient) {
            err = removeTransientShard(*databaseShardInfo, req);
            MojErrAccumulate(accErr, err);
        } else {
            err = update(*databaseShardInfo, req);
            MojErrAccumulate(accErr, err);
        }
    } else {
        err = put(*databaseShardInfo, req);
        MojErrAccumulate(accErr, err);
    }

    err = req->end(true); // unlock and commit immediately whatever we can
    MojErrCheck(err);

    if (databaseShardInfo->active) {
        err = dropGarbage(databaseShardInfo->id, req);
        MojErrAccumulate(accErr, err);

        err = putKindsHashes(databaseShardInfo->id, req);
        MojErrAccumulate(accErr, err);
    }

    //notify MojDB about the change of shards status
    //m_db.onShardStatusChange(databaseShardInfo);
    err = m_db.shardStatusChanged.call(*databaseShardInfo);
    MojErrAccumulate(accErr, err);

    // propagate accumulated error
    MojErrCheck(accErr);

    return MojErrNone;
}

MojErr MojDbShardEngine::processShardInfo(const MojDbShardInfo& shardInfo, MojDbReqRef req)
{
    MojDbShardInfo result;
    MojErr err;

    err = processShardInfo(shardInfo, &result, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::copyRequiredFields(const MojDbShardInfo& from, MojDbShardInfo& to)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    to.deviceUri = from.deviceUri;
    to.deviceName = from.deviceName;
    to.active = from.active;
    to.transient |= from.transient;
    to.parentDeviceId = from.parentDeviceId;

    return MojErrNone;
}

MojErr MojDbShardEngine::removeTransientShard(const MojDbShardInfo& shardInfo, MojDbReqRef req)
{
    LOG_TRACE("Entering function %s", __FUNCTION__);

    MojErr err;

    MojAssert(shardInfo.databasePath.data());
    err = MojRmDirRecursive(shardInfo.databasePath.data());
    MojErrCheck(err);

    err = removeShardInfo(shardInfo.id, req);
    MojErrCheck(err);


    return MojErrNone;
}

MojErr MojDbShardEngine::databasePrepare(MojDbShardInfo* shardInfo, MojDbReqRef req)
{
    MojErr err;
    MojErr accErr = MojErrNone;

    MojString databaseId;
    err = m_db.databaseId(databaseId, req);
    MojErrCheck(err);

    if (m_databasePrefixIsAbsolute) {
        // path absolute. databasePrefix: /var/db;
        //                shardId:        777x03
        // result:                        /var/db/777x03/
        err = shardInfo->databasePath.format(_T("%s/%08x/"), m_databasePrefix.data(), shardInfo->id);
        MojErrCheck(err);

        err = MojCreateDirIfNotPresent(shardInfo->databasePath.data());
        MojErrCheck(err);
    } else {    // is relative, use mountpath as current dir
        // path relative. databasePrefix  .db8
        //                mountPath:      /media/run/3A3A
        //                databaseId:     AA333AAA
        // result:                        /media/run/3A3A/.db8/AA333AAA/
        err = shardInfo->databasePath.format(_T("%s/%s/%s"), shardInfo->deviceUri.data(), m_databasePrefix.data(), databaseId.data());
        MojErrCheck(err);

        err = checkDatabaseAccess(shardInfo->databasePath.data());
        MojErrAccumulate(accErr, err);

        err = checkDatabaseSpace(shardInfo->databasePath.data());
        MojErrAccumulate(accErr, err);

        MojErrCatchAll(accErr) {
            err = shardInfo->databasePath.format(_T("%s/%08x/"), m_fallbackPath.data(), shardInfo->id);
            MojErrCheck(err);

            LOG_DEBUG("[db_shardEngine] Switch shard to use fallback. New shard db path: %s", shardInfo->databasePath.data());
        }
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::checkDatabaseAccess(const MojChar* path)
{
    MojAssert(path);
    MojErr err;

    if (0 != access(path, F_OK | R_OK | W_OK)) {
        if (ENOENT == errno) {  // does not exist
            err = MojCreateDirIfNotPresent(path);
            MojErrCheck(err);
        } else {
            LOG_DEBUG("[db_shardEngine] Database exist on %s , but it is read/only.", path);
            return MojErrNotOpen;
        }
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::checkDatabaseSpace(const MojChar* path)
{
    MojAssert(m_reqFreePartSpaceBytes > 0);
    MojAssert(m_reqFreePartSpacePercantage >= 0.0f);
    MojAssert(m_reqFreePartSpacePercantage <= 100.0f);

    struct statvfs st;
    int ret = statvfs(path, &st);
    if (ret != 0) {
        LOG_DEBUG("[db_shardEngine] Can't statfs %s ", path);
        return MojErrNotOpen;
    }

    typedef unsigned long long FileSystemBlocks;
    FileSystemBlocks minFreeBlocks = m_reqFreePartSpaceBytes / st.f_bsize;
    if (st.f_bavail < minFreeBlocks) {
        LOG_WARNING(MSGID_MOJ_DB_WARNING,
                    2,
                    PMLOGKS("path", path),
                    PMLOGKFV("required_space", "%lu", m_reqFreePartSpaceBytes),
                    "[db_shardEngine] No free space");
        return MojErrNoMem;
    }

    if (m_reqFreePartSpacePercantage > 0.0f) {
        // and check for
        minFreeBlocks = FileSystemBlocks(double(st.f_blocks) * double(st.f_frsize) * m_reqFreePartSpacePercantage / 100);
        if ((st.f_bavail * st.f_bsize) < minFreeBlocks) {
            LOG_WARNING(MSGID_MOJ_DB_WARNING,
                        2,
                        PMLOGKS("path", path),
                        PMLOGKFV("required_space", "%llu", minFreeBlocks),
                        "[db_shardEngine] No free space");
            return MojErrNoMem;
        }
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::putKindHash(const MojChar* kindName, MojDbReqRef req)
{
    MojErr err;
    std::list<MojDbShardInfo> shards;
    MojUInt32 count;

    MojDbKind* kind;
    err = m_db.kindEngine()->getKind(kindName, kind);
    MojErrCheck(err);

    MojAssert(kind);

    err = getAllActive(shards, count, req);
    MojErrCheck(err);

    for (const MojDbShardInfo& shard : shards) {
        err = putKindHash(shard.id, *kind, req);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::putKindHash(const MojObject& kindObj, MojDbReqRef req)
{
    MojErr err;
    std::list<MojDbShardInfo> shards;
    MojUInt32 count;

    err = getAllActive(shards, count, req);
    MojErrCheck(err);

    for (const MojDbShardInfo& shard : shards) {
        err = putKindHash(shard.id, kindObj, req);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::putKindHash(const MojUInt32 shardId, const MojDbKind& kind, MojDbReqRef req)
{
    MojErr err;

    KindHash kindHash;
    err = kindHash.fromKind(kind);
    MojErrCheck(err);

    MojString shardIdStr;
    err = convertId(shardId, shardIdStr);
    MojErrCheck(err);

    MojDbAdminGuard admin(req);
    err = kindHash.save(&m_db, shardIdStr, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::putKindHash(const MojUInt32 shardId, const MojObject& kindObj, MojDbReqRef req)
{
    MojErr err;

    KindHash kindHash;
    err = kindHash.fromKindObject(kindObj);
    MojErrCheck(err);

    MojString shardIdStr;
    err = convertId(shardId, shardIdStr);
    MojErrCheck(err);

    MojDbAdminGuard admin(req);
    err = kindHash.save(&m_db, shardIdStr, req);
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::putKindsHashes(const MojUInt32 shardId, MojDbReqRef req)
{
    MojAssert(m_db.kindEngine());

    MojErr err;

    MojVector<MojObject> kinds;
    err = m_db.kindEngine()->getKinds(kinds);
    MojErrCheck(err);

    for (const MojObject& kindObj : kinds) {
        err = putKindHash(shardId, kindObj, req);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::dropObject(const MojUInt32 shardId, MojDbStorageItem* item, MojDbReqRef req)
{
    MojAssert(item);

    MojErr err;
    bool found;

    err = req->begin(&m_db, true);
    MojErrCheck(err);

    const MojObject& id = item->id();

    MojUInt32 objectShardId;
    err = MojDbIdGenerator::extractShard(id, objectShardId);
    MojErrCheck(err);

    if (objectShardId != shardId)
        return MojErrNone;

    MojString kindName;
    err = item->kindId(kindName, *m_db.kindEngine());
    MojErrCheck(err);
    MojAssert(!kindName.empty());

    MojObject object;

    // Can't unserialize object and get required fields. Simple del method unworkable here. We manually prepare object,
    // ask Kind engine to remove index and remove manually object itself.

    // code below makes the same, as m_db.del method, but in tricky way.
    /*err = m_db.del(id, found, MojDbFlagPurge, req);
     M ojErrCheck(err);*/

    err = object.put(_T("_id"), id);
    MojErrCheck(err);

    err = object.putString(_T("_kind"), kindName);
    MojErrCheck(err);

    // SEE: MojDb::delObj
    MojTokenSet tokenSet;
    // we want purge to force delete
    req->fixmode(true);

    err = m_db.kindEngine()->update(nullptr, &object, req, OpDelete, tokenSet, true);
    MojErrCheck(err);

    err = m_db.storageDatabase()->del(shardId, id, req->txn(), found);
    MojErrCheck(err);

    err = req->end();
    MojErrCheck(err);

    return MojErrNone;
}

MojErr MojDbShardEngine::delKindData(const MojUInt32 shardId, const MojChar* kindId, MojDbReqRef req)
{
    MojErr err;
    MojDbQuery query;

    err = query.from(kindId);
    MojErrCheck(err);

    MojDbCursor cursor;
    err = m_db.find(query, cursor, req);

    while (true) {
        MojDbStorageItem* item;
        bool found;

        err = cursor.get(item, found);
        MojErrCheck(err);

        if (!found)
            break;

        err = dropObject(shardId, item, req);
        MojErrCheck(err);
    }

    return MojErrNone;
}

MojErr MojDbShardEngine::dropGarbage(const MojUInt32 shardId, MojDbReqRef req)
{
    MojErr err;
    KindHash::KindHashContainer hashes;
    MojVector<MojObject> kinds;

    err = m_db.kindEngine()->getKinds(kinds);
    MojErrCheck(err);

    MojDbAdminGuard admin(req);
    err = KindHash::loadHashes(&m_db, shardId, &hashes, req);
    MojErrCheck(err);

    MojString shardIdStr;
    err = convertId(shardId, shardIdStr);
    MojErrCheck(err);

    for (KindHash& hash : hashes)
    {
        MojDbKind* kind;
        err = m_db.kindEngine()->getKind(hash.kindId().data(), kind);

        switch (err)
        {
            case MojErrNone:
                if (kind->hash() != hash.hash()) {
                    err = delKindData(shardId, hash.kindId(), req);
                    MojErrCheck(err);

                    err = hash.del(&m_db, shardIdStr, req);
                    MojErrCheck(err);
                }

                break;
            case MojErrDbKindNotRegistered:
                err = delKindData(shardId, hash.kindId(), req);
                MojErrCheck(err);

                err = hash.del(&m_db, shardIdStr, req);
                MojErrCheck(err);

                break;
            default:
                MojErrThrow(err);
                break;
        }
    }

    return MojErrNone;
}
