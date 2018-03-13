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
#include "CpuMemStats.h"

#include <thread>
#include <iostream> /* std::cout, std::fixed */
#include <iomanip> /* std::setprecision */
#include <fstream>

namespace
{
    timeval operator- (const timeval& time1, const timeval& time2)
    {
        timeval result;

        if (time1.tv_usec >= time2.tv_usec) {
            result.tv_sec  = time1.tv_sec - time2.tv_sec;
            result.tv_usec = time1.tv_usec - time2.tv_usec;
        } else {
            result.tv_sec  = time1.tv_sec - time2.tv_sec - 1;
            result.tv_usec = 1000000 - (time2.tv_usec - time1.tv_usec);
        }

        return result;

    }
} // end namespace

CpuStat::CpuStat(const char *name)
: m_name(name)
{
    statBegin();
}

CpuStat::~CpuStat()
{
    statEnd();
    statPrintOut();
}

void CpuStat::statEnd()
{
    m_prev_u = m_u;
    getrusage(RUSAGE_SELF, &m_u);

    m_diff_utime = m_u.ru_utime - m_prev_u.ru_utime;
    m_diff_ktime = m_u.ru_stime - m_prev_u.ru_stime;
}

void CpuStat::statPrintOut()
{
    std::cout << std::setprecision(7) << std::fixed;
    std::cout << m_name << ".utime = " << m_diff_utime.tv_sec + m_diff_utime.tv_usec / 10e6L << " sec" << std::endl;
    std::cout << m_name << ".ktime = " << m_diff_ktime.tv_sec + m_diff_ktime.tv_usec / 10e6L << " sec" << std::endl;
}

void CpuStat::statBegin()
{
    getrusage(RUSAGE_SELF, &m_u);
}

MemStat::MemStat(const char *name) :
    m_name(name)
{
    statBegin();
}

MemStat::~MemStat()
{
    statEnd();
    statPrintOut();
}

void MemStat::statBegin()
{
    std::string s;
    std::ifstream in;

    in.open("/proc/self/status");
    while (std::getline(in, s)) {
        if (s.find("VmSize:") != std::string::npos) {
            m_min = std::stoul(&s[sizeof("VmSize:") - 1]);
        }
    }
    in.close();
}

void MemStat::statEnd()
{
    std::string s;
    std::ifstream in;

    in.open("/proc/self/status");
    while (std::getline(in, s)) {
        if (s.find("VmPeak:") != std::string::npos) {
            m_peak = (std::stoul(&s[sizeof("VmPeak:") - 1]));
        }
    }
    in.close();
}

void MemStat::statPrintOut()
{
    std::cout << m_name << ".avg = " << (m_peak + m_min) / 2 << " Kb" << std::endl;
    std::cout << m_name << ".peak = " <<  m_peak << " Kb" << std::endl;
}
