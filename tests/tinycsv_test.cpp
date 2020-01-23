#include "tinycsv_test.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include <cstdio>

std::pair<std::string, int> tinycsv_parse(FILE * f);
std::pair<std::string, int> tinycsv_expanded_parse(FILE * f);

test::Result test_read_tinycsv(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    if(delimiter != ',' || quote != '"' || lenient)
        return test::skip();

    // tinycsv can't distinguish between empty data and 1 empty field
    if(std::empty(expected_data))
        return test::skip();

    std::ofstream out("test.csv", std::ifstream::binary);
    if(!out)
        throw std::runtime_error("could not open test.csv");

    out.write(csv_text.data(), csv_text.size());
    out.close();

    FILE * r = std::fopen("test.csv", "rb");

    if(!r)
        throw std::runtime_error("could not open test.csv");

    CSV_data data(1);

    while(true)
    {
        try
        {
            const auto & [field, end_of_row] = tinycsv_parse(r);

            if(std::ferror(r))
            {
                std::fclose(r);
                throw std::runtime_error("Error reading test.csv");
            }

            data.back().push_back(field);

            if(std::feof(r))
                break;

            if(end_of_row)
                data.emplace_back();
        }
        catch(int)
        {
            std::cerr<<"Tinycsv error\n";
            std::fclose(r);
            return test::error();
        }

    }
    std::fclose(r);

    remove("test.csv");
    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_read_tinycsv_expanded(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    if(delimiter != ',' || quote != '"' || lenient)
        return test::skip();

    // tinycsv can't distinguish between empty data and 1 empty field
    if(std::empty(expected_data))
        return test::skip();

    std::ofstream out("test.csv", std::ifstream::binary);
    if(!out)
        throw std::runtime_error("could not open test.csv");

    out.write(csv_text.data(), csv_text.size());
    out.close();

    FILE * r = std::fopen("test.csv", "rb");

    if(!r)
        throw std::runtime_error("could not open test.csv");

    CSV_data data(1);

    while(true)
    {
        try
        {
            const auto & [field, end_of_row] = tinycsv_expanded_parse(r);

            if(std::ferror(r))
            {
                std::fclose(r);
                throw std::runtime_error("Error reading test.csv");
            }

            data.back().push_back(field);

            if(std::feof(r))
                break;

            if(end_of_row)
                data.emplace_back();
        }
        catch(const char * what)
        {
            std::cerr<<"Tinycsv_expanded error: "<<what<<"\n";
            std::fclose(r);
            return test::error();
        }

    }
    std::fclose(r);

    remove("test.csv");
    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

void Tinycsv_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_tinycsv);
    tests.register_read_test(test_read_tinycsv_expanded);
}
