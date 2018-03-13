// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "dbhelpers.h"
#include <boost/filesystem.hpp>
#include <iostream>

MojErr getFirstLastObject(MojDb* db, MojObject* obj, bool first, MojDbReqRef req)
{
	MojErr err;
	MojDbQuery query;
	err = query.from(_T("com.foo.bar:1"));
	MojErrCheck(err);

	query.limit(1);
	query.desc(!first);

	MojDbCursor cursor;
	err = db->find(query, cursor, req);
	MojErrCheck(err);

	bool found = false;
	err = cursor.get(*obj, found);
	MojErrCheck(err);
	if (!found)
		MojErrThrow(MojErrNotFound);

	return MojErrNone;
}

MojErr getMiddleObject(MojDb* db, MojObject* obj, MojDbReqRef req)
{
	MojErr err;

	MojObject result;
	err = getFirstLastObject(db, &result, false);
	MojErrCheck(err);

	MojUInt32 index;
	err = result.getRequired(_T("a"), index);
	MojErrCheck(err);

	MojDbQuery query;
	err = query.from(_T("com.foo.bar:1"));
	MojErrCheck(err);

	query.limit(1);

	index /= 2;
	err = query.where(_T("a"), MojDbQuery::OpEq, index);
	MojErrCheck(err);

	MojDbCursor cursor;
	err = db->find(query, cursor, req);
	MojErrCheck(err);

	bool found = false;
	err = cursor.get(*obj, found);
	MojErrCheck(err);
	if (!found)
		MojErrThrow(MojErrNotFound);

	return MojErrNone;
}

bool copyDir(boost::filesystem::path const & source, boost::filesystem::path const & destination)
{
	try
	{
		// Check whether the function call is valid
		if (!boost::filesystem::exists(source) || !boost::filesystem::is_directory(source)) {
			std::cerr << "Source directory " << source.string() << " does not exist or is not a directory." << '\n';
			return false;
		}

		if(boost::filesystem::exists(destination)) {
			std::cerr << "Destination directory " << destination.string()
			<< " already exists." << '\n'
			;
			return false;
		}
		// Create the destination directory
		if(!boost::filesystem::create_directory(destination)) {
			std::cerr << "Unable to create destination directory"
			<< destination.string() << '\n';
			return false;
		}
	}
	catch(boost::filesystem::filesystem_error const & e)
	{
		std::cerr << e.what() << '\n';
		return false;
	}
	// Iterate through the source directory
	for(boost::filesystem::directory_iterator file(source); file != boost::filesystem::directory_iterator(); ++file) {
		try {
			boost::filesystem::path current(file->path());
			if(boost::filesystem::is_directory(current)) {
				// Found directory: Recursion
				if(!copyDir(current,destination / current.filename())) {
					return false;
				}
			} else {
				// Found file: Copy
				boost::filesystem::copy_file(current, destination / current.filename());
			}
		}
		catch(boost::filesystem::filesystem_error const & e) {
			std:: cerr << e.what() << '\n';
		}
	}
	return true;
}
