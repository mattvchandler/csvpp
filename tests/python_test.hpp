#ifndef CSV_PYTHON_TEST_HPP
#define CSV_PYTHON_TEST_HPP

#include "csv_test_suite.hpp"

class Python_test final: public CSV_test_suite::CSV_test_group
{
public:
    Python_test();
    ~Python_test();
    Python_test(const Python_test &);
    Python_test & operator=(const Python_test &);
    Python_test(Python_test &&);
    Python_test & operator=(Python_test &&);
    void register_tests(CSV_test_suite & tests) const override;

private:
    static std::size_t interpreter_ref_count;
};

#endif // CSV_PYTHON_TEST_HPP
