// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include <getopt.h>
#include <iostream>

#include "core/MojUtil.h"

#include "gtest/gtest.h"

#include "DbGenerator.h"

const char *g_dump_path;
const char *g_dump_folder;
const char *g_temp_folder;
// Number of threads to start in Concurrent tests
size_t g_nthreads;
// Number of steps per each thread
size_t g_nsteps;
// Number of iterations for Sequential tests
size_t g_counter;

const MojChar *g_moj_test_kind =
        _T("{\"id\":\"LunaDbClientTest:1\",")
        _T("\"owner\":\"com.palm.mojfakeluna\",")
        _T("\"indexes\":[")
                _T("{\"name\":\"foo\",\"props\":[{\"name\":\"foo\",\"tokenize\":\"all\",\"collate\":\"primary\"}]},")
                _T("{\"name\":\"bar\",\"props\":[{\"name\":\"bar\",\"tokenize\":\"all\",\"collate\":\"primary\"}]}")
        _T("]}");

void usage()
{
    std::cout << ("Usage: gtest_dbclient_perf [--nsteps|-s] NUM [--nthreads|-t] NUM [--prefix|p] PATH\n");
}

int main(int argc, char **argv) {
    int c;
    unsigned nsteps = 100;
    unsigned nthreads = 10;
    char prefix[NAME_MAX];
    char test_folder_tmp[NAME_MAX];
    bool verbose = false;
    bool dumping = false;
    static struct option long_options[] = {
        {"nsteps", 1, 0, 's'},
        {"nthreads", 1, 0, 't'},
        {"prefix", 1, 0, 'p'},
        {"dump-to", 1, 0, 'd'},
        {"load-from", 1, 0, 'l'},
        {"verbose", 0, 0, 'v'},
        {"help", 0, 0, 'h'},
        {NULL, 0, NULL, 0}
    };
    int option_index = 0;
    testing::InitGoogleTest(&argc, argv);

    while ((c = getopt_long(argc, argv, "s:t:p:hv",
                     long_options, &option_index)) != -1) {
    switch (c) {
        case 'p':
	    snprintf(prefix, NAME_MAX, "%s", optarg);
            break;
        case 's':
            nsteps = atoi(optarg);
            break;
        case 't':
            nthreads = atoi(optarg);
            break;
        case 'h':
            usage();
            return EXIT_SUCCESS;
            break;
        case 'd':
            g_dump_path = optarg;
            dumping = true;
            break;
        case 'l':
            g_dump_path = optarg;
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            break;
        default:
            std::cerr << "Unrecognized option: " << c << std::endl;
            usage();
            return EXIT_FAILURE;
        }
    }

    if (!nsteps) {
        std::cerr << "Number of steps not specified." << std::endl;
        usage();
        return EXIT_FAILURE;
    }
    if (!nthreads) {
        std::cerr << "Number of threads not specified." << std::endl;
        usage();
        return EXIT_FAILURE;
    }
    if (nsteps % nthreads) {
        std::cerr << "Number of threads must be adjective to number of steps." << std::endl;
        return EXIT_FAILURE;
    }
    if (!prefix[0]) {
        strncpy(prefix, ".", sizeof("."));
    }

    snprintf(test_folder_tmp, NAME_MAX, "%s/dbclient-perf-test-dir-XXXXXX", prefix);
    g_temp_folder = mkdtemp(test_folder_tmp);

    if (!g_temp_folder)
        g_temp_folder = "/tmp/dbclient-perf-test-dir"; // fallback

    g_counter = nsteps;
    g_nthreads = nthreads;
    g_nsteps = g_counter / g_nthreads;
    if (verbose) {
        std::cout << "nthreads = " <<  g_nthreads << std::endl;
        std::cout << "nsteps = " << g_counter / g_nthreads << std::endl;
        std::cout << "counter = " << g_counter << std::endl;
    }

    if (g_dump_path) {
        if (dumping) {
            MojErr errGeneration = generateDatabase(g_counter, g_dump_path);
            MojErr errRmDir = MojRmDirRecursive(g_temp_folder);
            if (errGeneration || errRmDir)
                return EXIT_FAILURE;

            return EXIT_SUCCESS;
        } else if (access(g_dump_path, F_OK)) {
            std::cerr << "access: dump file does not exist: " << strerror(errno) << std::endl;
            return EXIT_FAILURE;
        }
    }
    // setup and run tests
    int status = RUN_ALL_TESTS();

    // cleanup temp folder
    MojErr err = MojRmDirRecursive(g_temp_folder);
    if (err)
        std::cerr << "Unable to remove "<< g_temp_folder << std::endl;

    return status;
}
