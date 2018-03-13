#pragma once

#include "suites.h"
#include "sample.h"
#include <tuple>
#include <vector>
#include <boost/chrono.hpp>

using Durations = std::vector<boost::chrono::thread_clock::duration>;
using Suite = std::tuple<const char*, std::function<MojErr(MojDb* db, size_t, size_t, Durations*)>, const char*>;
