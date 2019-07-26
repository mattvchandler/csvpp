// Copyright 2018 Matthew Chandler

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <functional>
#include <vector>

namespace test
{
    enum class Result {fail, pass, error, skip};

    template <typename... Args>
    class Test
    {
    public:
        explicit Test(const std::initializer_list<std::function<Result(Args...)>> & cases): test_cases(cases)
        {}

        bool test_pass(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::pass, title, args...);
        }

        bool test_fail(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::fail, title, args...);
        }

        bool test_error(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::error, title, args...);
        }

        bool passed() const
        {
            return num_ran == num_passed;
        }
        std::size_t get_num_passed() const { return num_passed; }
        std::size_t get_num_ran() const { return num_ran; }

    private:

        bool run_tests(Result expected_result, const std::string_view & title, Args ...args)
        {
            std::cout<<title<<"\n";

            std::size_t total = test_cases.size();
            std::size_t count = 0;
            for(auto & f: test_cases)
            {
                auto result = f(args...);
                if(result == expected_result)
                    ++count;

                if(result == Result::skip)
                    --total;
            }

            auto passed = count == total;

            if(passed)
                std::cout<<"PASSED ";
            else
                std::cout<<"***FAILED*** ";

            num_passed += count;
            num_ran += total;

            std::cout<<"("<<count<<"/"<<total;
            if(total != test_cases.size())
                std::cout<<" - "<<test_cases.size() - total<<" tests skipped";
            std::cout<<")\n";

            return passed;
        }

        std::vector<std::function<Result(Args...)>> test_cases;
        std::size_t num_passed = 0;
        std::size_t num_ran = 0;
    };
}

#endif // TEST_HPP
