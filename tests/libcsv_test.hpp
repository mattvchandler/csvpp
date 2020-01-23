#ifndef CSV_LIBCSV_TEST_HPP
#define CSV_LIBCSV_TEST_HPP

#include "csv_test_suite.hpp"

class Libcsv_test final: public CSV_test_suite::CSV_test_group
{
private:
    void register_tests(CSV_test_suite & tests) const override;
};

#endif // CSV_LIBCSV_TEST_HPP
