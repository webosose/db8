// Copyright (c) 2009-2019 LG Electronics, Inc.
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


#ifndef MOJDBSERVICEDEFS_H_
#define MOJDBSERVICEDEFS_H_

#include "db/MojDbDefs.h"

class MojDbServiceDefs
{
public:
	// property keys
	static const MojChar* const BytesKey;
	static const MojChar* const CallerKey;
	static const MojChar* const CountKey;
	static const MojChar* const CountryCodeKey;
	static const MojChar* const CreateKey;
	static const MojChar* const DeleteKey;
	static const MojChar* const DeletedRevKey;
	static const MojChar* const DescriptionKey;
	static const MojChar* const DirKey;
	static const MojChar* const ExtendKey;
	static const MojChar* const FilesKey;
	static const MojChar* const FiredKey;
	static const MojChar* const FullKey;
	static const MojChar* const HasMoreKey;
	static const MojChar* const IdKey;
	static const MojChar* const IdsKey;
	static const MojChar* const IgnoreMissingKey;
	static const MojChar* const IncludeDeletedKey;
	static const MojChar* const IncrementalKey;
	static const MojChar* const KeysKey;
	static const MojChar* const KindKey;
	static const MojChar* const KindNameKey;
	static const MojChar* const LanguageCodeKey;
	static const MojChar* const LocaleKey;
	static const MojChar* const QueryKey;
	static const MojChar* const MethodKey;
	static const MojChar* const NextKey;
	static const MojChar* const ObjectKey;
	static const MojChar* const ObjectsKey;
	static const MojChar* const OperationsKey;
	static const MojChar* const OwnerKey;
	static const MojChar* const ParamsKey;
	static const MojChar* const PathKey;
	static const MojChar* const PermissionsKey;
	static const MojChar* const PostBackupKey;
	static const MojChar* const PostRestoreKey;
	static const MojChar* const PreBackupKey;
	static const MojChar* const PreRestoreKey;
	static const MojChar* const PrevKey;
	static const MojChar* const ProceedKey;
	static const MojChar* const PropsKey;
	static const MojChar* const PurgeKey;
	static const MojChar* const QuotasKey;
	static const MojChar* const ReadKey;
	static const MojChar* const ResponsesKey;
	static const MojChar* const ResultsKey;
	static const MojChar* const RevKey;
    static const MojChar* const ShardIdKey;
	static const MojChar* const SizeKey;
	static const MojChar* const ServiceKey;
	static const MojChar* const SubscribeKey;
	static const MojChar* const TypeKey;
	static const MojChar* const UpdateKey;
	static const MojChar* const UsedKey;
	static const MojChar* const VersionKey;
	static const MojChar* const VerifyKey;
	static const MojChar* const WarningsKey;
	static const MojChar* const WatchKey;
	static const MojChar* const WindowKey;
	static const MojChar* const ListActiveMediaKey;
	static const MojChar* const ShardInfoKey;
	static const MojChar* const ShardKindKey;
	static const MojChar* const SetShardModeKey;
	static const MojChar* const MediaKey;
	static const MojChar* const TransientKey;
	static const MojChar* const OwnersKey;
    static const MojChar* const AssignIdKey;
    static const MojChar* const ProfileEnableKey;
    static const MojChar* const ProfileApplicationKey;
	// property values
	static const MojChar* const AllowValue;
	static const MojChar* const DenyValue;
	// method names
	static const MojChar* const BatchMethod;
	static const MojChar* const CompactMethod;
	static const MojChar* const DelMethod;
	static const MojChar* const DelKindMethod;
	static const MojChar* const DumpMethod;
	static const MojChar* const FindMethod;
	static const MojChar* const GetMethod;
	static const MojChar* const GetPermissionsMethod;
	static const MojChar* const LoadMethod;
	static const MojChar* const MergeMethod;
    static const MojChar* const MergePutMethod;
	static const MojChar* const PostBackupMethod;
	static const MojChar* const PostRestoreMethod;
	static const MojChar* const PreBackupMethod;
	static const MojChar* const PreRestoreMethod;
	static const MojChar* const PurgeMethod;
	static const MojChar* const PurgeStatusMethod;
	static const MojChar* const PutMethod;
	static const MojChar* const PutKindMethod;
	static const MojChar* const PutPermissionsMethod;
	static const MojChar* const PutQuotasMethod;
	static const MojChar* const QuotaStatsMethod;
	static const MojChar* const ReserveIdsMethod;
	static const MojChar* const ScheduledPurgeMethod;
	static const MojChar* const SearchMethod;
	static const MojChar* const SpaceCheckMethod;
	static const MojChar* const ScheduledSpaceCheckMethod;
	static const MojChar* const ReleaseAdminProfileMethod;
	static const MojChar* const StatsMethod;
	static const MojChar* const WatchMethod;
	static const MojChar* const ListActiveMediaMethod;
	static const MojChar* const ShardInfoMethod;
	static const MojChar* const ShardKindMethod;
	static const MojChar* const SetShardModeMethod;
    static const MojChar* const ProfileMethod;
    static const MojChar* const GetProfileMethod;
	static const MojChar* const RemoveAppDataMethod;
	// service names
	static const MojChar* const Category;
	static const MojChar* const InternalCategory;
	static const MojChar* const ServiceName;
    static const MojChar* const MediaServiceName;
	static const MojChar* const TempServiceName;
    static const MojChar* const ASMServiceName;
    static const MojChar* const ASMListDeviceMethod;
    static const MojChar* const ASMRegisterDeviceStatusMethod;
    static const MojChar* const ASMResponseDeviceStatusMethod;
    static const MojChar* const ASMUnRegisterDeviceStatusMethod;
    static const MojChar* const ASMDeviceStatusEjecting;
    static const MojChar* const ASMDeviceStatusFormatting;

    // other
    static const MojChar* const DefaultMediaLinkPath;
};

#endif /* MOJDBSERVICEDEFS_H_ */
