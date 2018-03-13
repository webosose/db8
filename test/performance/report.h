#pragma once

#include <ostream>
#include <iostream>
#include <boost/filesystem.hpp>
#include <fstream>
#include "suite.h"

std::ostream& operator<<(std::ostream& stream, const std::map<MojUInt32 , Durations>& results)
{
	for (const auto& ts : results) {
		stream << ts.first;

		for (const auto& t : ts.second) {
			stream << "," << boost::chrono::duration <double, boost::milli> (t).count();
		}
		stream << std::endl;
	}

	return stream;
}

std::ostream& operator<<(std::ostream& stream, const std::map<long int, double>& results)
{
        for (const auto& ts : results) {
            stream << ts.first;
            stream << "," << boost::chrono::duration <double, boost::milli> (ts.second).count();
            stream << std::endl;
        }

        return stream;
}

MojErr combineAvgNumbers2Report(const boost::filesystem::path& reportPath, const std::map<MojUInt32, Durations>& results) {

    boost::filesystem::path full_path( boost::filesystem::current_path());
    full_path += boost::filesystem::path::preferred_separator;

    //ignore multi threaded reports for combining report
    if (reportPath.parent_path().leaf().compare("1")) {
        std::cout << "ignore multi threaded reports" << std::endl;
        return MojErrNone;
    }

    std::map<long int, double> avgTimeMap;

    for (const auto& ts : results) {
        double sum = 0;
        for (const auto& t : ts.second) {
            sum = sum + boost::chrono::duration <double, boost::milli> (t).count();
        }
        avgTimeMap[ts.first] = sum/ts.second.size();
    }

    if (reportPath.string().find("get")!= std::string::npos) {
        full_path += "/CombinedReport/getFinalReport.csv";
    } else if (reportPath.string().find("find")!= std::string::npos) {
        full_path += "/CombinedReport/findFinalReport.csv";
    } else if (reportPath.string().find("search")!= std::string::npos) {
        full_path += "/CombinedReport/searchFinalReport.csv";
    }

    std::ofstream fstream(full_path.c_str(), std::ofstream::app);
    fstream << avgTimeMap;
    std::cout << results;
    fstream.close();

    return MojErrNone;
}

MojErr writeNumbers2Report(const boost::filesystem::path& reportPath, const std::map<MojUInt32, Durations>& results)
{
    combineAvgNumbers2Report(reportPath,results);

	std::ofstream fstream(reportPath.c_str(), std::ofstream::out);
	if (!fstream.is_open()) {
		MojErrThrow(MojErrNotOpen);
	}

	fstream << results;
	std::cout << results;

	fstream.close();

	return MojErrNone;
}

MojErr getReportPath(const boost::filesystem::path& outdir,
					 const std::string& suitename,
					 const std::string& dataset,
					 boost::filesystem::path* path,
                     const std::string& threadPath)
{
	// generate report part
	boost::filesystem::path& reportPath = *path;

	reportPath = outdir;
	reportPath += boost::filesystem::path::preferred_separator;
	reportPath += suitename;

	if (!boost::filesystem::exists(reportPath)) {
		boost::filesystem::create_directory(reportPath);
	}
	reportPath += boost::filesystem::path::preferred_separator;
        reportPath += threadPath;

	if(!boost::filesystem::exists(reportPath)){
                boost::filesystem::create_directory(reportPath);
        }

	reportPath += boost::filesystem::path::preferred_separator;
	reportPath += dataset;
	reportPath += ".csv";

	return MojErrNone;
}
