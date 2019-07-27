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
#include <string>
#include <vector>

namespace test
{
    using FailureFun = std::function<void()>;
    class Result
    {
    public:
        void failed() { failure_fun(); };

        friend Result pass(FailureFun);
        friend Result pass();
        friend Result fail(FailureFun);
        friend Result fail();
        friend Result pass_fail(bool, FailureFun);
        friend Result pass_fail(bool);
        friend Result error(FailureFun f);
        friend Result error();
        friend Result skip(FailureFun f);
        friend Result skip();

        template <typename...> friend class Test;
    private:
        enum class Result_type {fail, pass, error, skip};

        explicit Result(Result_type t, FailureFun f): type{t}, failure_fun{f} {}
        Result(Result_type t): type{t} {}

        Result_type type {Result_type::skip};
        FailureFun failure_fun {[](){}};
    };

    inline Result pass(FailureFun f) { return Result{Result::Result_type::pass, f}; }
    inline Result pass() { return Result{Result::Result_type::pass}; }

    inline Result fail(FailureFun f) { return Result{Result::Result_type::fail, f}; }
    inline Result fail() { return Result{Result::Result_type::fail}; }

    inline Result pass_fail(bool passed, FailureFun f) { return passed ? Result{Result::Result_type::pass, f} : Result{Result::Result_type::fail, f}; }
    inline Result pass_fail(bool passed) { return passed ? Result{Result::Result_type::pass} : Result{Result::Result_type::fail}; }

    inline Result error(FailureFun f) { return Result{Result::Result_type::error, f}; }
    inline Result error() { return Result{Result::Result_type::error}; }

    inline Result skip(FailureFun f) { return Result{Result::Result_type::skip, f}; }
    inline Result skip() { return Result{Result::Result_type::skip}; }

    template <typename... Args>
    class Test
    {
    public:
        explicit Test(const std::initializer_list<std::function<Result(Args...)>> & cases): test_cases{cases} {}

        bool test_pass(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::Result_type::pass, title, args...);
        }

        bool test_fail(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::Result_type::fail, title, args...);
        }

        bool test_error(const std::string_view & title, Args ...args)
        {
            return run_tests(Result::Result_type::error, title, args...);
        }

        bool passed() const
        {
            return num_ran == num_passed;
        }
        std::size_t get_num_passed() const { return num_passed; }
        std::size_t get_num_ran() const { return num_ran; }

    private:

        bool run_tests(Result::Result_type expected_result, const std::string_view & title, Args ...args)
        {
            std::cout<<title<<"\n";

            std::size_t total = test_cases.size();
            std::size_t count = 0;
            for(auto & f: test_cases)
            {
                Result result = f(args...);
                if(result.type == Result::Result_type::skip)
                    --total;
                else if(result.type == expected_result)
                    ++count;
                else
                    result.failed();
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
