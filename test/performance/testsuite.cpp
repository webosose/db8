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

#include "suites.h"
#include "report.h"

#include <iostream>
#include <chrono>
#include <fstream>

#include <functional>
#include <chrono>
#include <boost/program_options.hpp>
#include <boost/chrono/thread_clock.hpp>

#include <thread>
#include <mutex>

#include <boost/regex.hpp>

namespace po = boost::program_options;

MojErr metric_ProcessNObjects(const Suite& suite, const boost::filesystem::path& dbpath, size_t repeats, size_t sampleIterations, size_t threads, std::map<MojUInt32, Durations>* results)
{
	MojErr err;

	std::unique_ptr<MojDb> db(new MojDb);
	err = db->open(dbpath.c_str());
	MojErrCheck(err);

	Durations durations;
	durations.reserve(repeats);

	auto sample = std::get<1>(suite);
	for (size_t iteration = 0; iteration < repeats; ++iteration) {
		// get data for
		err = sample(db.get(), threads, sampleIterations, &durations);
		MojErrCheck(err);
	}

	MojUInt32 dataset = std::atol(dbpath.filename().c_str());
	results->insert(std::make_pair(dataset, std::move(durations)));

	err = db->close();
	MojErrCheck(err);

	return MojErrNone;
}


bool canProcessDataset(const Suite& suite, const std::string& dataset)
{
	boost::regex expression;

	expression.assign (std::get<2>(suite));

	boost::cmatch what;
	if(boost::regex_match(dataset.c_str(), what, expression)) {
		return true;
	} else {
		return false;
	}
}

int main (int argc, const char** argv)
{
	MojErr err;
	boost::filesystem::path datasetPath = "dbs";
	boost::filesystem::path reportsPath = "reports";
	size_t repeats;
	size_t samples;
	size_t clientCount;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "produce help message")
	("workdir", po::value<std::string>()->default_value("dbs"),      "Path to workdir, where database will be opened")
	("report",  po::value<std::string>()->default_value("reports"),  "Path to report file (if not set, will print to stdout")
	("repeats", po::value<size_t>()->default_value(100),             "How many repeats make in case of one sample")
	("samples", po::value<size_t>()->default_value(1000),            "How many samples do (independent tests)")
	("clientcount", po::value<size_t>()->default_value(1),      "No of  clients to database");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("workdir")) {
		datasetPath = vm["workdir"].as<std::string>();

		if (!boost::filesystem::is_directory(datasetPath)) {
			std::cerr << "directory \"" << datasetPath << "\" is not writeable or doesn't exists" << std::endl;
			return 2;
		}
	}

	if (vm.count("report")) {
		reportsPath = vm["report"].as<std::string>();

		if (!boost::filesystem::is_directory(reportsPath)) {
			std::cerr << "directory \"" << reportsPath << "\" is not writeable or doesn't exists" << std::endl;
			return 2;
		}
	}

	if (vm.count("repeats")) {
		repeats = vm["repeats"].as<size_t>();
	}

	if (vm.count("samples")) {
		samples = vm["samples"].as<size_t>();
	}
	if (vm.count("clientcount")) {
		clientCount = vm["clientcount"].as<size_t>();
	}

	boost::filesystem::directory_iterator end;

    boost::filesystem::path full_path( boost::filesystem::current_path());
    full_path += boost::filesystem::path::preferred_separator;
    full_path += "CombinedReport";
    boost::filesystem::remove_all(boost::filesystem::path(full_path));

    boost::filesystem::create_directory(full_path);

	for (auto& suite : Suites)
	{
		for (boost::filesystem::directory_iterator datasetPathIter(datasetPath); datasetPathIter != end; ++datasetPathIter)
		{
			boost::filesystem::path suiteDirectoryName(*datasetPathIter);

			std::cout << "Run test \"" << std::get<0>(suite) << "\" with \"" << suiteDirectoryName.string() << "\"" << std::endl;

			if (!canProcessDataset(suite, suiteDirectoryName.filename().string())) {
				std::cout << "Test \"" << std::get<0>(suite) << "\" can't be runned with \"" << suiteDirectoryName.filename().string() << "\". Ignoring it" << std::endl;
				continue;
			}
			std::map<MojUInt32, Durations> times;
			for(size_t nThreads = 1;nThreads <= clientCount; nThreads++)
			{
				for (boost::filesystem::directory_iterator dbsIter(suiteDirectoryName); dbsIter != end; ++dbsIter)
				{
					err = metric_ProcessNObjects(suite, *dbsIter, samples, repeats, nThreads, &times);
					MojErrCheck(err);
				}
				boost::filesystem::path reportPath;
					err = getReportPath(reportsPath, std::get<0>(suite), suiteDirectoryName.filename().string(), &reportPath, std::to_string(nThreads));
				MojErrCheck(err);

				err = writeNumbers2Report(reportPath, times);
				MojErrCheck(err);

				times.clear();
			}
		}
	}

	return 0;
}
