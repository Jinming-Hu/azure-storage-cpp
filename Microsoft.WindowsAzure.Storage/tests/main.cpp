// -----------------------------------------------------------------------------------------
// <copyright file="main.cpp" company="Microsoft">
//    Copyright 2013 Microsoft Corporation
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//      http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
// </copyright>
// -----------------------------------------------------------------------------------------

#include "stdafx.h"

#include <unordered_set>

#include "was/blob.h"

#ifndef _WIN32

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#endif


int run_tests(const std::unordered_set<std::string>& included_cases, const std::unordered_set<std::string>& excluded_cases)
{
    UnitTest::TestReporterStdout reporter;
    UnitTest::TestRunner runner(reporter);

    auto match = [](const std::unordered_set<std::string>& all_cases, const std::string& suite_name, const std::string& test_name)
    {
        return all_cases.find(suite_name) != all_cases.end() || all_cases.find(suite_name + ":" + test_name) != all_cases.end();
    };

    return runner.RunTestsIf(UnitTest::Test::GetTestList(), nullptr, [match, included_cases, excluded_cases](const UnitTest::Test* test) -> bool
    {
        std::string suite_name = test->m_details.suiteName;
        std::string test_name = test->m_details.testName;

        return !match(excluded_cases, suite_name, test_name) && (included_cases.empty() || match(included_cases, suite_name, test_name));
    }, 0);
}

int main(int argc, const char** argv)
{
    azure::storage::operation_context::set_default_log_level(azure::storage::client_log_level::log_level_verbose);

#ifndef _WIN32
    boost::log::add_common_attributes();
    boost::log::add_file_log
    (
        boost::log::keywords::file_name = "test_log.log",
        boost::log::keywords::format = 
        (
            boost::log::expressions::stream << "<Sev: " << boost::log::trivial::severity
            << "> " << boost::log::expressions::smessage
        )
    );

#endif

    std::unordered_set<std::string> included_cases;
    std::unordered_set<std::string> excluded_cases;

    auto starts_with = [](const std::string& str, const std::string& prefix)
    {
        size_t i = 0;
        while (i < str.length() && i < prefix.length() && prefix[i] == str[i]) ++i;
        return i == prefix.length();
    };

    auto add_to = [](std::unordered_set<std::string>& all_cases, const std::string& name)
    {
        auto colon_pos = name.find(":");
        std::string suite_name = name.substr(0, colon_pos);
        if (suite_name.empty())
        {
            throw std::invalid_argument("Invalid test case \"" + name + "\".");
        }
        all_cases.emplace(name);
    };

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (starts_with(arg, "--exclude="))
        {
            add_to(excluded_cases, arg.substr(arg.find('=') + 1));
        }
        else
        {
            add_to(included_cases, arg);
        }
    }

    return run_tests(included_cases, excluded_cases);
}
