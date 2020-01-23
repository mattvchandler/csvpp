#ifndef TINYCSV_TEST_HPP
#define TINYCSV_TEST_HPP

#include "csv_test_suite.hpp"

class Tinycsv_test final: public CSV_test_suite::CSV_test_group
{
private:
    void register_tests(CSV_test_suite & tests) const override;
};

#endif // TINYCSV_TEST_HPP
