#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <functional>
#include <vector>

namespace test
{
    class Skip_test: public std::exception { };

    template <typename... Args>
    class Test
    {
    public:
        explicit Test(const std::initializer_list<std::function<bool(Args...)>> & cases): test_cases(cases)
        {}

        bool test_pass(const std::string_view & title, Args ...args)
        {
            return run_tests(true, title, args...);
        }

        bool test_fail(const std::string_view & title, Args ...args)
        {
            return run_tests(false, title, args...);
        }
        bool passed() const
        {
            return num_ran == num_passed;
        }
        std::size_t get_num_passed() const { return num_passed; }
        std::size_t get_num_ran() const { return num_ran; }

    private:

        bool run_tests(bool test_for_pass, const std::string_view & title, Args ...args)
        {
            std::cout<<title<<"\n";

            std::size_t total = test_cases.size();
            std::size_t count = 0;
            for(auto & f: test_cases)
            {
                try
                {
                    if(f(args...) == test_for_pass)
                        ++count;
                }
                catch(Skip_test)
                {
                    --total;
                }
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
                std::cout<<" "<<test_cases.size() - total<<" - tests skipped";
            std::cout<<")\n";

            return passed;
        }

        std::vector<std::function<bool(Args...)>> test_cases;
        std::size_t num_passed = 0;
        std::size_t num_ran = 0;
    };
}

#endif // TEST_HPP
