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
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>
#include <unordered_map>
#include <thread>

namespace po = boost::program_options;

static const std::pair<const char*, const char*> DataSuites[] = {
	{"allindexdb",     "{\"id\":\"com.foo.bar:1\",\"owner\":\"mojodb.admin\",\"indexes\":[ \
		{\"name\":\"a\",\"props\":[{\"name\":\"a\"}]}, \
		{\"name\":\"b\",\"props\":[{\"name\":\"b\"}]}, \
		{\"name\":\"c\",\"props\":[{\"name\":\"c\"}]}, \
		{\"name\":\"ab\",\"props\":[{\"name\":\"a\"},{\"name\":\"b\"}]}, \
		{\"name\":\"ba\",\"props\":[{\"name\":\"b\"},{\"name\":\"a\"}]}, \
		{\"name\":\"ac\",\"props\":[{\"name\":\"a\"},{\"name\":\"c\"}]}, \
		{\"name\":\"ca\",\"props\":[{\"name\":\"c\"},{\"name\":\"a\"}]}, \
		{\"name\":\"bc\",\"props\":[{\"name\":\"b\"},{\"name\":\"c\"}]}, \
		{\"name\":\"cb\",\"props\":[{\"name\":\"c\"},{\"name\":\"b\"}]} \
		] \
	}"},
};

MojErr putObjects(size_t paralel, const size_t goffset, const size_t repeat, MojDb* db)
{
	MojErr err;

	std::vector<std::thread> workers;

	const size_t window = static_cast<size_t> (std::ceil(repeat / paralel));

	for (size_t t = 0; t < paralel; ++t) {
		const size_t offset = t * window;
		const size_t max = std::min(offset + window, repeat);

		workers.push_back(std::thread([=, &db]{
			MojErr err;

			MojDbReq req;
			req.beginBatch();

			for (size_t i = offset; i < max; ++i) {
				MojObject obj;

				err = obj.putInt(_T("a"), i + goffset);
				MojErrCheck(err);
				err = obj.putInt(_T("b"), i + goffset);
				MojErrCheck(err);
				err = obj.putInt(_T("c"), i + goffset);
				MojErrCheck(err);

				err = obj.putString(MojDb::KindKey, _T("com.foo.bar:1"));
				MojErrCheck(err);

				err = db->put(obj, MojDbFlagNone, req);
				MojErrCheck(err);

				err = req.endBatch();
				MojErrCheck(err);
			}
			return MojErrNone;
		}));
	}

	std::for_each(workers.begin(), workers.end(), [](std::thread &t) {
		t.join();
	});


	return MojErrNone;
}

int main (int argc, const char** argv)
{
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "Produce help message")
		("maxobjects", po::value<size_t>()->default_value(204800 + 1), "How many objects should be in database")
		("threads", po::value<size_t>()->default_value(10), "How many threads to run in paralel")
		("outdir", po::value<std::string>()->default_value("."), "Output directory");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	std::string outdir = ".";
	size_t maxobjects;
	size_t paralelThreads;

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("outdir")) {
		outdir = vm["outdir"].as< std::string >();
	}

	if (!boost::filesystem::is_directory(outdir)) {
		std::cerr << "directory \"" << outdir << "\" is not writeable or doesn't exists" << std::endl;
		return 2;
	}

	if (vm.count("maxobjects")) {
		maxobjects = vm["maxobjects"].as< size_t >();
	}

	if (vm.count("threads")) {
		paralelThreads = vm["threads"].as< size_t >();
	}

	boost::filesystem::path tmppath = boost::filesystem::temp_directory_path();
	tmppath += boost::filesystem::path::preferred_separator;
	tmppath += boost::filesystem::unique_path();

	boost::filesystem::create_directory(tmppath);

	std::cout << tmppath << std::endl;

	std::unordered_map<std::string, std::unique_ptr<MojDb>> databases;
	std::unordered_map<std::string, boost::filesystem::path> databasesPathes;
	MojErr err;

	for (const auto& suite : DataSuites) {
		boost::filesystem::path dbpath = tmppath;

		dbpath += boost::filesystem::path::preferred_separator;
		dbpath += suite.first;

		boost::filesystem::create_directory(dbpath);

		databasesPathes.insert(std::make_pair(suite.first, dbpath));
		databases.insert(std::make_pair(suite.first, std::unique_ptr<MojDb> (new MojDb)));

		MojDb& db = *databases[suite.first];

		err = db.open(databasesPathes[suite.first].c_str());
		MojErrCheck(err);

		MojObject kindObject;

		err = kindObject.fromJson(suite.second);
		MojErrCheck(err);

		err = db.putKind(kindObject);
		MojErrCheck(err);
	}

	size_t objects = 0;
	for (size_t i = 100; i < maxobjects; i = i * 2) {
		size_t repeat = i - objects;

		std::vector<std::thread> threads;

		size_t counter = 0;
		for (auto dbi = databases.begin(); dbi != databases.end(); ++dbi, ++counter) {
			MojDb* db = dbi->second.get();

			threads.push_back(std::thread([=]() {
				MojErr err;
				err = putObjects(paralelThreads, objects, repeat, db);
				MojErrCheck(err);

				err = db->compact();
				MojErrCheck(err);

				return MojErrNone;
			}));

			std::for_each(threads.begin(), threads.end(), [](std::thread &t) {
				t.join();
			});

			threads.clear();
		}

		for (auto dbi = databases.begin(); dbi != databases.end(); ++dbi, ++counter) {
			err = dbi->second->close();
			MojErrCheck(err);

			boost::filesystem::path dbout = outdir;
			dbout += boost::filesystem::path::preferred_separator;
			dbout += dbi->first;

			boost::filesystem::create_directory(dbout);

			dbout += boost::filesystem::path::preferred_separator;
			dbout += std::to_string(i);

			if (boost::filesystem::exists(dbout)) {
				boost::filesystem::remove_all(dbout);
			}
			copyDir(databasesPathes.at(dbi->first), dbout);

			err = dbi->second->open(databasesPathes.at(dbi->first).c_str());
			MojErrCheck(err);
		}

		std::cout << i << " " << std::endl;

		objects += repeat;
	}

	return 0;
}
