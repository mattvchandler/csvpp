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

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <csv.h>

#ifdef CSV_ENABLE_PYTHON
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>
#endif

#include "csv.h"

#include "csv.hpp"

#include "test.hpp"

#ifdef CSV_ENABLE_TINYCSV
#include "tinycsv_test.hpp"
#endif

template<typename Tuple, std::size_t N>
constexpr void print_tuple(std::ostream &o, const Tuple & t)
{
    if constexpr(N == 1)
    {
        o<<std::get<0>(t);
    }
    else
    {
        print_tuple<decltype(t), N - 1>(o, t);
        o<<','<<std::get<N - 1>(t);
    }
}
template<typename ... Packed>
std::ostream & operator<<(std::ostream & o, const std::tuple<Packed...> & t)
{
    o<<'(';
    print_tuple<decltype(t), sizeof...(Packed)> (o, t);
    return o<<')';
}

template <typename T>
std::ostream & operator<<(std::ostream & o, const std::vector<T> & v)
{
    o<<'[';
    bool first = true;
    for(auto & i: v)
    {
        if(!first)
            o<<',';
        o<<i;
        first = false;
    }
    o<<']';
    return o;
}

using CSV_data = std::vector<std::vector<std::string>>;

bool test_read_mine_c(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    std::ofstream out("test.csv", std::ifstream::binary);
    if(!out)
        return false;

    out.write(csv_text.data(), csv_text.size());
    out.close();

    CSV_reader * r_test = CSV_reader_init_from_filename("test.csv");

    if(!r_test)
        return false;

    CSV_reader_set_delimiter(r_test, delimiter);
    CSV_reader_set_quote(r_test, quote);

    size_t row = 0;
    while(1)
    {
        char ** fields;
        size_t num_fields;
        CSV_status status = CSV_reader_read_record(r_test, &fields, &num_fields);

        if(status != CSV_OK && status != CSV_EOF && status != CSV_EMPTY_ROW)
            goto error;
        else if(status == CSV_EOF)
            break;
        else if(status == CSV_EMPTY_ROW)
            continue;

        if(row >= expected_data.size() || num_fields != expected_data[row].size())
            goto error;

        if(fields)
        {
            for(size_t col = 0; col < num_fields; ++col)
            {
                if(col > expected_data[row].size() || std::string(fields[col]) != expected_data[row][col])
                    goto error;
            }
        }
        ++row;
    }
    if(row != expected_data.size())
        goto error;

    CSV_reader_free(r_test);
    return true;

error:
    CSV_reader_free(r_test);
    return false;
}

bool test_read_mine_simple_c(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    char * csv = (char *)malloc(sizeof(char) * (csv_text.size() + 1));
    strcpy(csv, csv_text.c_str());

    std::size_t row_num = 0, field_num = 0;
    char * start = csv, * end = csv;
    while(true)
    {
        field_num = 0;
        bool start_row = true;
        while(true)
        {
            bool quoted = false;
            if(*start == quote)
            {
                quoted = true;
                ++start; ++end;
            }

            // find end of current field
            while(*end && (quoted || (*end != delimiter && *end != '\n' && *end != '\r')))
            {
                start_row = false;
                // we need special handling for quotes
                if(*end == quote)
                {
                    // look at next char
                    ++end;
                    if(quoted)
                    {
                        // is it an end-quote? (if so, we are at the end of the record)
                        if(!*end || *end == delimiter || *end == '\n' || *end == '\r')
                        {
                            break;
                        }
                        // is it an escaped quote?
                        if(*end == quote)
                        {
                            // remove the 2nd quote by shifting everything past it left by one
                            char * shift_char;
                            for(shift_char = end; *shift_char; ++shift_char)
                                *shift_char = *(shift_char + 1);
                        }
                        // it's an error
                        else
                        {
                            printf("[ERROR] row %d, field %d: unescaped double-quote inside quoted field\n", (int)row_num + 1, (int)field_num + 1);
                            free(csv);
                            return false;
                        }
                    }
                    // quotes are not allowed inside of an unquoted field
                    else
                    {
                        printf("[ERROR] row %d, field %d: double-quote in unquoted field\n", (int)row_num + 1, (int)field_num + 1);
                        free(csv);
                        return false;
                    }
                }
                else
                    ++end;
            }
            if(*end == delimiter)
                start_row = false;

            if(!*end && quoted)
            {
                printf("[ERROR] row %d, field %d: Unterminated quoted field\n", (int)row_num + 1, (int)field_num + 1);
                free(csv);
                return false;
            }

            std::string field(start, end);
            if(quoted)
                field.pop_back();

            while((*end == '\r' || *end == '\n') && (!*(end + 1) || *(end + 1) == '\n' || *(end + 1) == '\r'))
                ++end;

            if(row_num < expected_data.size() && field_num < expected_data[row_num].size() && field != expected_data[row_num][field_num])
            {
                free(csv);
                return false;
            }

            ++field_num;

            if(*end == '\r' || *end == '\n' || !*end)
                break;

            start = ++end;
        }

        if(!start_row && row_num < expected_data.size() && field_num != expected_data[row_num].size())
        {
            free(csv);
            return false;
        }

        if(!start_row)
            ++row_num;

        if(!*end)
            break;

        start = ++end;
    }

    if(row_num != expected_data.size())
    {
        free(csv);
        return false;
    }

    free(csv);
    return true;
}

#ifdef CSV_ENABLE_TINYCSV
bool test_read_tinycsv(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    if(delimiter != ',' || quote != '"')
        throw test::Skip_test{};

    // tinycsv can't distinguish between empty data and 1 empty field
    if(std::empty(expected_data))
        throw test::Skip_test();

    std::ofstream out("test.csv", std::ifstream::binary);
    if(!out)
        return false;

    out.write(csv_text.data(), csv_text.size());
    out.close();

    FILE * r_test = std::fopen("test.csv", "rb");

    if(!r_test)
        return false;

    CSV_data data(1);

    while(true)
    {
        try
        {
            const auto & [field, end_of_row] = tinycsv_parse(r_test);

            if(std::ferror(r_test))
                goto error;

            data.back().push_back(field);

            if(std::feof(r_test))
                break;

            if(end_of_row)
                data.emplace_back();
        }
        catch(int)
        {
            std::cerr<<"Tinycsv error\n";
            goto error;
        }

    }
    std::fclose(r_test);

    return data == expected_data;

error:
    std::fclose(r_test);
    return false;
}

bool test_read_tinycsv_expanded(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    if(delimiter != ',' || quote != '"')
        throw test::Skip_test{};

    // tinycsv can't distinguish between empty data and 1 empty field
    if(std::empty(expected_data))
        throw test::Skip_test();

    std::ofstream out("test.csv", std::ifstream::binary);
    if(!out)
        return false;

    out.write(csv_text.data(), csv_text.size());
    out.close();

    FILE * r_test = std::fopen("test.csv", "rb");

    if(!r_test)
        return false;

    CSV_data data(1);

    while(true)
    {
        try
        {
            const auto & [field, end_of_row] = tinycsv_expanded_parse(r_test);

            if(std::ferror(r_test))
                goto error;

            data.back().push_back(field);

            if(std::feof(r_test))
                break;

            if(end_of_row)
                data.emplace_back();
        }
        catch(const char * what)
        {
            std::cerr<<"Tinycsv_expanded error: "<<what<<"\n";
            goto error;
        }

    }
    std::fclose(r_test);

    return data == expected_data;

error:
    std::fclose(r_test);
    return false;
}
#endif

bool test_read_mine_cpp_read_all(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        return csv::Reader(csv::Reader::input_string, csv_text, delimiter, quote).read_all() == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_read_rows(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        CSV_data data;
        while(true)
        {
            auto row = r.get_row();
            if(!row)
                break;
            data.push_back(row.read_vec());
        }
        return data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_read_row_vec(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        CSV_data data;
        while(true)
        {
            auto row = r.read_row_vec();
            if(!row)
                break;
            data.push_back(*row);
        }
        return data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_read_all_as_int(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    std::vector<std::vector<int>> expected_ints;
    for(auto & row: expected_data)
    {
        expected_ints.emplace_back();
        for(auto & col: row)
        {
            auto skip = [](const std::exception & e)
            {
                using namespace std::string_literals;
                if(e.what() == "stoi"s)
                    throw test::Skip_test();
            };

            try { expected_ints.back().push_back(std::stoi(col)); }
            catch(const std::invalid_argument & e)
            {
                skip(e);
                return false;
            }
            catch(const std::out_of_range & e)
            {
                skip(e);
                return false;
            }
        }
    }
    try
    {
        return csv::Reader(csv::Reader::input_string, csv_text, delimiter, quote).read_all<int>() == expected_ints;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_stream(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            for(auto & expected_field: expected_row)
            {
                std::string field;
                try
                {
                    r>>field;
                }
                catch(const csv::Out_of_range_error&)
                {
                    return false;
                }
                if(field != expected_field)
                    return false;
            }
            if(!r.end_of_row())
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_fields(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            for(auto & expected_field: expected_row)
            {
                std::string field;
                try
                {
                    field = r.read_field();;
                }
                catch(const csv::Out_of_range_error&)
                {
                    return false;
                }
                if(field != expected_field)
                    return false;
            }
            if(!r.end_of_row())
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_iters(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);
        CSV_data data;

        for(auto & r: r)
        {
            data.emplace_back();
            std::copy(r.begin(), r.end(), std::back_inserter(data.back()));
        }

        return data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_range(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);
        CSV_data data;

        for(auto & r: r)
        {
            data.emplace_back();
            for(auto & field: r)
            {
                data.back().push_back(field);
            }
        }

        return data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_fields(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row = r.get_row();
            for(auto & expected_field: expected_row)
            {
                if(row.end_of_row())
                    return false;

                auto field = row.read_field();
                if(field!= expected_field)
                    return false;
            }

            try
            {
                row.read_field();
                return false;
            }
            catch(const csv::Out_of_range_error&) {}

            if(!row.end_of_row())
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_stream(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row = r.get_row();
            for(auto & expected_field: expected_row)
            {
                if(row.end_of_row())
                    return false;

                std::string field;
                row>>field;

                if(field != expected_field)
                    return false;
            }

            try
            {
                std::string after_field;
                row>>after_field;
                return false;
            }
            catch(const csv::Out_of_range_error&) {}

            if(!row.end_of_row())
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_range(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row = r.get_row();

            std::size_t expected_col = 0;
            for(auto & col: row.range())
            {
                if(expected_col >= expected_row.size())
                    return false;

                if(col != expected_row[expected_col])
                    return false;
                ++expected_col;
            }
            if(expected_col != expected_row.size())
                return false;

            if(!row.end_of_row())
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_row_vec(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row = r.get_row().read_vec();
            if(row != expected_row)
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_map(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Map_reader_iter r(csv::Reader::input_string, csv_text, {}, {}, delimiter, quote);

        auto & headers = expected_data.at(0);
        if(r.get_headers() != headers)
            return false;

        std::size_t i = 1;
        for(; r != csv::Map_reader_iter{} && i < std::size(expected_data); ++r, ++i)
        {
            if(std::size(expected_data[i]) != std::size(headers))
                throw test::Skip_test{};

            std::remove_reference_t<decltype(*r)> expected_row;
            for(std::size_t j = 0; j < std::size(headers); ++j)
                expected_row.emplace(headers[j], expected_data[i][j]);

            if(*r != expected_row)
                return false;
        }
        if(i != std::size(expected_data) || r != csv::Map_reader_iter{})
            return false;

        return true;
    }
    catch(const csv::Parse_error & e)
    {
        if(e.what() == std::string{"Error parsing CSV at line: 0, col: 0: Can't get header row"} && std::size(expected_data) == 0)
            return true;
        else
        {
            std::cerr<<e.what()<<"\n";
            return false;
        }
    }
    catch(const csv::Out_of_range_error &e)
    {
        if(e.what() == std::string{"Too many columns"})
            throw test::Skip_test{};
        else
            throw;
    }
}
bool test_read_mine_cpp_map_as_int(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    std::vector<std::vector<int>> expected_ints;
    for(auto & row: expected_data)
    {
        expected_ints.emplace_back();
        for(auto & col: row)
        {
            auto skip = [](const std::exception & e)
            {
                using namespace std::string_literals;
                if(e.what() == "stoi"s)
                    throw test::Skip_test();
            };

            try { expected_ints.back().push_back(std::stoi(col)); }
            catch(const std::invalid_argument & e)
            {
                skip(e);
                return false;
            }
            catch(const std::out_of_range & e)
            {
                skip(e);
                return false;
            }
        }
    }

    try
    {
        csv::Map_reader_iter<int, int> r(csv::Reader::input_string, csv_text, {}, {}, delimiter, quote);

        auto headers = expected_ints.at(0);
        if(r.get_headers() != headers)
            return false;

        std::size_t i = 1;
        for(; r != csv::Map_reader_iter{} && i < std::size(expected_ints); ++r, ++i)
        {
            if(std::size(expected_ints[i]) != std::size(headers))
                throw test::Skip_test{};

            std::remove_reference_t<decltype(*r)> expected_row;
            for(std::size_t j = 0; j < std::size(headers); ++j)
                expected_row.emplace(headers[j], expected_ints[i][j]);

            if(*r != expected_row)
                return false;
        }
        if(i != std::size(expected_ints) || r != csv::Map_reader_iter{})
            return false;

        return true;
    }
    catch(const csv::Parse_error & e)
    {
        if(e.what() == std::string{"Error parsing CSV at line: 0, col: 0: Can't get header row"} && std::size(expected_data) == 0)
            return true;
        else
        {
            std::cerr<<e.what()<<"\n";
            return false;
        }
    }
    catch(const csv::Out_of_range_error &e)
    {
        if(e.what() == std::string{"Too many columns"})
            throw test::Skip_test{};
        else
            throw;
    }
}

bool test_read_mine_cpp_variadic(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        CSV_data data;
        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                break;

            if(std::size(expected_row) > 5)
                throw test::Skip_test{};

            std::vector<std::string> row(std::size(expected_row));
            try
            {
                switch(std::size(row))
                {
                case 0:
                    r.read_row_v();
                    break;
                case 1:
                    r.read_row_v(row[0]);
                    break;
                case 2:
                    r.read_row_v(row[0], row[1]);
                    break;
                case 3:
                    r.read_row_v(row[0], row[1], row[2]);
                    break;
                case 4:
                    r.read_row_v(row[0], row[1], row[2], row[3]);
                    break;
                case 5:
                    r.read_row_v(row[0], row[1], row[2], row[3], row[4]);
                    break;
                default:
                    throw test::Skip_test{};
                }
            }
            catch(const csv::Out_of_range_error& e)
            {
                if(e.what() == std::string{"Read past end of row"})
                    return false;
                throw;
            }

            data.push_back(row);
        }
        return r.eof() && data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_tuple(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        CSV_data data;
        for(auto & expected_row: expected_data)
        {
            if(std::size(expected_row) > 5)
                throw test::Skip_test{};

            std::vector<std::string> row;

            try
            {
                switch(std::size(expected_row))
                {
                case 0:
                    // r.read_row_tuple<>(); // Doesn't compile :(
                    break;
                case 1:
                {
                    auto row_tuple = r.read_row_tuple<std::string>().value();
                    row = {std::get<0>(row_tuple)};
                    break;
                }
                case 2:
                {
                    auto row_tuple = r.read_row_tuple<std::string, std::string>().value();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple)};
                    break;
                }
                case 3:
                {
                    auto row_tuple = r.read_row_tuple<std::string, std::string, std::string>().value();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple)};
                    break;
                }
                    break;
                case 4:
                {
                    auto row_tuple = r.read_row_tuple<std::string, std::string, std::string, std::string>().value();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple), std::get<3>(row_tuple)};
                    break;
                }
                    break;
                case 5:
                {
                    auto row_tuple = r.read_row_tuple<std::string, std::string, std::string, std::string, std::string>().value();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple), std::get<3>(row_tuple), std::get<4>(row_tuple)};
                    break;
                }
                    break;
                default:
                    throw test::Skip_test{};
                }
            }
            catch(const csv::Out_of_range_error & e)
            {
                if(e.what() == std::string{"Read past end of row"})
                    return false;
                throw;
            }
            catch(const std::bad_optional_access&)
            {
                break;
            }

            data.push_back(row);
        }
        return r.eof() && data == expected_data;
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_variadic(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row_obj = r.get_row();
            std::vector<std::string> row(std::size(expected_row), "NOT SET");

            try
            {
                switch(std::size(row))
                {
                case 0:
                    row_obj.read_v();
                    break;
                case 1:
                    row_obj.read_v(row[0]);
                    break;
                case 2:
                    row_obj.read_v(row[0], row[1]);
                    break;
                case 3:
                    row_obj.read_v(row[0], row[1], row[2]);
                    break;
                case 4:
                    row_obj.read_v(row[0], row[1], row[2], row[3]);
                    break;
                case 5:
                    row_obj.read_v(row[0], row[1], row[2], row[3], row[4]);
                    break;
                default:
                    throw test::Skip_test{};
                }
            }
            catch(const csv::Out_of_range_error&)
            {
                return false;
            }

            if(row != expected_row)
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

bool test_read_mine_cpp_row_tuple(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote);

        for(auto & expected_row: expected_data)
        {
            if(r.eof())
                return false;

            auto row_obj = r.get_row();
            std::vector<std::string> row;

            try
            {
                switch(std::size(expected_row))
                {
                case 0:
                    break;
                case 1:
                {
                    auto row_tuple = row_obj.read_tuple<std::string>();
                    row = {std::get<0>(row_tuple)};
                    break;
                }
                case 2:
                {
                    auto row_tuple = row_obj.read_tuple<std::string, std::string>();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple)};
                    break;
                }
                case 3:
                {
                    auto row_tuple = row_obj.read_tuple<std::string, std::string, std::string>();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple)};
                    break;
                }
                    break;
                case 4:
                {
                    auto row_tuple = row_obj.read_tuple<std::string, std::string, std::string, std::string>();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple), std::get<3>(row_tuple)};
                    break;
                }
                    break;
                case 5:
                {
                    auto row_tuple = row_obj.read_tuple<std::string, std::string, std::string, std::string, std::string>();
                    row = {std::get<0>(row_tuple), std::get<1>(row_tuple), std::get<2>(row_tuple), std::get<3>(row_tuple), std::get<4>(row_tuple)};
                    break;
                }
                    break;
                default:
                    throw test::Skip_test{};
                }
            }
            catch(const csv::Out_of_range_error&)
            {
                return false;
            }

            if(row != expected_row)
                return false;
        }

        return r.eof();
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return false;
    }
}

#ifdef CSV_ENABLE_PYTHON
const char * test_read_python_code = R"(
def test_read_python(csv_text, expected_data, delimiter, quote):
    infile = io.StringIO(csv_text, newline='')
    r = csv.reader(infile, delimiter = delimiter, quotechar = quote, strict=True)

    data = []
    try:
        for row in r:
            if row:
                data.append(row)
    except csv.Error as e:
        raise Parse_error(r.line_num, e)

    return data == expected_data

def test_read_python_map(csv_text, expected_data, delimiter, quote):
    infile = io.StringIO(csv_text, newline='')
    try:
        r = csv.DictReader(infile, delimiter = delimiter, quotechar = quote, strict=True)

        if not r.fieldnames and not expected_data:
            return True

        headers = expected_data[0]
        if r.fieldnames != headers:
            return False

        i = 1
        for row in r:
            if i >= len(expected_data):
                return False

            if len(expected_data[i]) != len(headers):
                raise Skip_test

            if row != collections.OrderedDict((headers[j], expected_data[i][j]) for j in range(len(headers))):
                return False
            i+=1

        if i != len(expected_data) or next(r, None) is not None:
            return False

    except csv.Error as e:
        raise Parse_error(r.line_num, e)

    return True
)";
#endif

struct libcsv_read_status
{
    CSV_data expected_data;
    size_t row, col;
    bool result;
};

void libcsv_read_cb1(void * field, size_t, void * data)
{
    struct libcsv_read_status * stat = (libcsv_read_status*)data;

    if(stat->result)
    {
        if(stat->row >= stat->expected_data.size() || stat->col >= stat->expected_data[stat->row].size())
        {
            stat->result = false;
            return;
        }

        if(std::string((const char *)field) != stat->expected_data[stat->row][stat->col])
            stat->result = false;

        ++stat->col;
    }
}

void libcsv_read_cb2(int, void * data)
{
    struct libcsv_read_status * stat = (libcsv_read_status*)data;
    if(stat->result)
    {
        if(stat->col < stat->expected_data[stat->row].size())
            stat->result = false;

        stat->col = 0;

        if(++stat->row > stat->expected_data.size())
            stat->result = false;
    }
}

bool test_read_libcsv(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
{
    csv_parser parse;
    if(csv_init(&parse, CSV_APPEND_NULL | CSV_STRICT | CSV_STRICT_FINI) != 0)
        return false;

    csv_set_delim(&parse, delimiter);
    csv_set_quote(&parse, quote);

    libcsv_read_status stat = {expected_data, 0, 0, true};

    auto error = [&parse]()
    {
        std::cerr<<csv_strerror(csv_error(&parse))<<"\n";
        csv_free(&parse);
    };

    if(csv_parse(&parse, csv_text.c_str(), csv_text.size(), libcsv_read_cb1, libcsv_read_cb2, &stat) < csv_text.size())
    {
        error();
        return false;
    }
    csv_fini(&parse, libcsv_read_cb1, libcsv_read_cb2, &stat);

    int err_code = csv_error(&parse);
    if(err_code != CSV_SUCCESS)
    {
        error();
        return false;
    }

    csv_free(&parse);

    if(stat.row < expected_data.size())
        return false;

    return stat.result;
}

bool test_write_mine_c(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    CSV_writer * w_test = CSV_writer_init_from_filename("test.csv");
    if(!w_test)
        return false;

    CSV_writer_set_delimiter(w_test, delimiter);
    CSV_writer_set_quote(w_test, quote);

    for(const auto & row: data)
    {
        std::vector<const char *> col_c_strs;
        for(const auto & col: row)
        {
            col_c_strs.push_back(col.c_str());
        }
        if(CSV_writer_write_record(w_test, col_c_strs.data(), row.size()) != CSV_OK)
        {
            CSV_writer_free(w_test);
            return false;
        }
    }
    CSV_writer_free(w_test);

    std::ifstream out_file("test.csv", std::ifstream::binary);
    return expected_text == static_cast<std::stringstream const &>(std::stringstream() << out_file.rdbuf()).str();
}

bool test_write_mine_cpp_stream(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: data)
        {
            for(const auto & col: row)
                w<<col;
            w<<csv::end_row;
        }
    }
    return str.str() == expected_text;
}

bool test_write_mine_cpp_row(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: data)
        {
            w.write_row(row);
        }
    }
    return str.str() == expected_text;
}

bool test_write_mine_cpp_iter(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: data)
        {
            std::copy(row.begin(), row.end(), w.iterator());
            w.end_row();
        }
    }
    return str.str() == expected_text;
}

bool test_write_mine_cpp_map(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    if(!std::empty(data))
    {
        auto headers =  data[0];
        csv::Map_writer_iter w(str, headers, {}, delimiter, quote);
        for(auto row = std::begin(data) + 1; row != std::end(data); ++row, ++w)
        {
            if(std::size(*row) != std::size(headers))
                throw test::Skip_test{};

            std::map<std::string, std::string> out_row;
            for(std::size_t i = 0; i < std::size(headers); ++i)
                out_row[headers[i]] = (*row)[i];

            *w = out_row;
        }
    }
    return str.str() == expected_text;
}

bool test_write_mine_cpp_variadic(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(auto & row: data)
        {
            switch(std::size(row))
            {
            case 0:
                w.write_row_v();
                break;
            case 1:
                w.write_row_v(row[0]);
                break;
            case 2:
                w.write_row_v(row[0], row[1]);
                break;
            case 3:
                w.write_row_v(row[0], row[1], row[2]);
                break;
            case 4:
                w.write_row_v(row[0], row[1], row[2], row[3]);
                break;
            case 5:
                w.write_row_v(row[0], row[1], row[2], row[3], row[4]);
                break;
            default:
                throw test::Skip_test{};
            }
        }
    }
    return str.str() == expected_text;
}
bool test_write_mine_cpp_tuple(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(auto & row: data)
        {
            switch(std::size(row))
            {
            case 0:
                w.write_row(std::tuple{});
                break;
            case 1:
                w.write_row(std::tuple{row[0]});
                break;
            case 2:
                w.write_row(std::tuple{row[0], row[1]});
                break;
            case 3:
                w.write_row(std::tuple{row[0], row[1], row[2]});
                break;
            case 4:
                w.write_row(std::tuple{row[0], row[1], row[2], row[3]});
                break;
            case 5:
                w.write_row(std::tuple{row[0], row[1], row[2], row[3], row[4]});
                break;
            default:
                throw test::Skip_test{};
            }
        }
    }
    return str.str() == expected_text;
}

#ifdef CSV_ENABLE_PYTHON
const char * test_write_python_code = R"(
def test_write_python(expected_text, data, delimiter, quote):
    out = io.StringIO(newline='')
    w = csv.writer(out, delimiter=delimiter, quotechar=quote)
    for row in data:
        w.writerow(row)
    return out.getvalue() == expected_text

def test_write_python_map(expected_text, data, delimiter, quote):
    out = io.StringIO(newline='')
    if data:
        headers = data[0]

        w = csv.DictWriter(out, headers, delimiter=delimiter, quotechar=quote)
        w.writeheader()

        for row in data[1:]:
            if len(row) != len(headers):
                raise Skip_test
            w.writerow({ headers[i]: row[i] for i in range(len(row)) })

    return out.getvalue() == expected_text
)";
#endif

bool test_write_libcsv(const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
{
    if(delimiter != ',' || quote != '"')
        throw test::Skip_test{};

    std::string output = "";

    for(const auto & row: data)
    {
        for(std::size_t col_num = 0; col_num < row.size(); ++col_num)
        {
            if(col_num > 0)
                output += ',';

            auto & col = row[col_num];

            bool unquote = true;
            for(const auto & c: col)
            {
                if(c == ',' || c == '"' || c == '\n' || c == '\r')
                {
                    unquote = false;
                    break;
                }
            }

            const std::size_t DEST_BUFF_SIZE=1024;
            char dest_buff[DEST_BUFF_SIZE] = {0};

            csv_write(dest_buff, DEST_BUFF_SIZE, col.c_str(), col.size());
            std::string dest(dest_buff);
            if(unquote)
            {
                dest = dest.substr(1, dest.size() - 2);
            }

            output += dest;
        }
        output += "\r\n";
    }

    return output == expected_text;
}

template<typename Test>
void test_quotes(Test test, const std::string & title, const std::string & csv_text, const CSV_data& data)
{
    test(title, csv_text, data, ',', '"');

    std::string modified_csv_text = std::regex_replace(csv_text, std::regex(","), "|");
    CSV_data modified_data = data;
    for(auto & row: modified_data)
        for(auto & col: row)
            col = std::regex_replace(col, std::regex{","}, "|");

    test(title + " w/ pipe delimiter", modified_csv_text, modified_data, '|', '"');

    modified_csv_text = std::regex_replace(csv_text, std::regex("\""), "'");
    modified_data = data;
    for(auto & row: modified_data)
        for(auto & col: row)
            col = std::regex_replace(col, std::regex{"\""}, "'");

    test(title + " w/ single quote", modified_csv_text, modified_data, ',', '\'');

    modified_csv_text = std::regex_replace(csv_text, std::regex(","), "|");
    modified_csv_text = std::regex_replace(modified_csv_text, std::regex("\""), "'");
    modified_data = data;
    for(auto & row: modified_data)
        for(auto & col: row)
        {
            col = std::regex_replace(col, std::regex{","}, "|");
            col = std::regex_replace(col, std::regex{"\""}, "'");
        }

    test(title + " w/ pipe delimiter & single quote", modified_csv_text, modified_data, '|', '\'');
}

int main(int, char *[])
{
    #ifdef CSV_ENABLE_PYTHON
    pybind11::scoped_interpreter interp{};
    pybind11::exec(R"(
        import io, collections, csv
        class Skip_test(Exception): pass
        class Parse_error(Exception):
            def __init__(self, line_num, msg):
                super().__init__("Python CSV error on line {}: {}".format(line_num, msg))
        )");

    auto handle_parse_error = [](const char * what)
    {
        for(const char * c = what; *c && *c != '\n'; ++c)
            std::cout<<*c;
        std::cout<<'\n';
    };

    // python allows fields w/ unescaped quotes. We'll need to pre-parse to catch them
    auto python_too_lenient = [](const std::string & csv, const char delimiter, const char quote)
    {
        bool empty = true;
        bool quoted = false;
        for(auto i = std::begin(csv);; ++i)
        {
            if(i != std::end(csv) && *i == quote)
            {
                if(quoted)
                {
                    ++i;
                    if(i == std::end(csv) || *i == delimiter || *i == '\n' || *i == '\r')
                        quoted = false;
                    else if(*i != quote)
                        return false;
                }
                else
                {
                    if(empty)
                    {
                        quoted = true;
                        continue;
                    }
                    else
                        return true;
                }
            }
            if(i == std::end(csv) && quoted)
            {
                return false;
            }
            else if(!quoted && i != std::end(csv) && *i == delimiter)
            {
                empty = true;
                continue;
            }
            else if(!quoted && (i == std::end(csv) || *i == '\n' || *i == '\r'))
            {
                empty = true;
                for(;i != std::end(csv) && *i != '\r' && *i != '\n'; ++i);

                if(i == std::end(csv))
                    return false;

                empty = true;
                continue;
            }
            empty = false;
        }
    };

    pybind11::exec(test_read_python_code);
    auto test_read_python_fun = pybind11::globals()["test_read_python"];
    auto test_read_python = [&test_read_python_fun, handle_parse_error, python_too_lenient](const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
    {
        if(python_too_lenient(csv_text, delimiter, quote))
            throw test::Skip_test{};

        try
        {
            return test_read_python_fun(csv_text, expected_data, delimiter, quote).cast<bool>();
        }
        catch(pybind11::error_already_set & e)
        {
            if(e.matches(pybind11::globals()["Parse_error"]))
            {
                handle_parse_error(e.what());
                return false;
            }
            else
                throw;
        }
    };

    auto test_read_python_map_fun = pybind11::globals()["test_read_python_map"];
    auto test_read_python_map = [&test_read_python_map_fun, handle_parse_error, python_too_lenient](const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote)
    {
        if(python_too_lenient(csv_text, delimiter, quote))
            throw test::Skip_test{};

        try
        {
            return test_read_python_map_fun(csv_text, expected_data, delimiter, quote).cast<bool>();
        }
        catch(pybind11::error_already_set & e)
        {
            if(e.matches(pybind11::globals()["Skip_test"]))
                throw test::Skip_test{};
            else if(e.matches(pybind11::globals()["Parse_error"]))
            {
                handle_parse_error(e.what());
                return false;
            }
            else
                throw;
        }
    };
    pybind11::exec(test_write_python_code);

    auto test_write_python_fun = pybind11::globals()["test_write_python"];
    auto test_write_python = [&test_write_python_fun](const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
    {
        return test_write_python_fun(expected_text, data, delimiter, quote).cast<bool>();
    };

    auto test_write_python_map_fun = pybind11::globals()["test_write_python_map"];
    auto test_write_python_map = [&test_write_python_map_fun](const std::string & expected_text, const CSV_data data, const char delimiter, const char quote)
    {
        try
        {
            return test_write_python_map_fun(expected_text, data, delimiter, quote).cast<bool>();
        }
        catch(pybind11::error_already_set & e)
        {
            if(e.matches(pybind11::globals()["Skip_test"]))
                throw test::Skip_test{};
            else
                throw;
        }
    };
    #endif

    test::Test<const std::string&, const CSV_data&, const char, const char> test_read{{
        test_read_mine_c,
        test_read_mine_simple_c,
        #ifdef CSV_ENABLE_TINYCSV
        test_read_tinycsv,
        test_read_tinycsv_expanded,
        #endif
        test_read_mine_cpp_read_all,
        test_read_mine_cpp_read_rows,
        test_read_mine_cpp_read_row_vec,
        test_read_mine_cpp_read_all_as_int,
        test_read_mine_cpp_stream,
        test_read_mine_cpp_fields,
        test_read_mine_cpp_iters,
        test_read_mine_cpp_range,
        test_read_mine_cpp_row_fields,
        test_read_mine_cpp_row_stream,
        test_read_mine_cpp_row_range,
        test_read_mine_cpp_row_row_vec,
        test_read_mine_cpp_map,
        test_read_mine_cpp_map_as_int,
        test_read_mine_cpp_variadic,
        test_read_mine_cpp_tuple,
        test_read_mine_cpp_row_variadic,
        test_read_mine_cpp_row_tuple,
        #ifdef CSV_ENABLE_PYTHON
        test_read_python,
        test_read_python_map,
        #endif
        test_read_libcsv
    }};

    test::Test<const std::string, const CSV_data&, const char, const char> test_write{{
        test_write_mine_c,
        test_write_mine_cpp_stream,
        test_write_mine_cpp_row,
        test_write_mine_cpp_iter,
        test_write_mine_cpp_map,
        test_write_mine_cpp_variadic,
        test_write_mine_cpp_tuple,
        #ifdef CSV_ENABLE_PYTHON
        test_write_python,
        test_write_python_map,
        #endif
        test_write_libcsv
    }};

    std::cout<<"Reader Tests:\n";

    using namespace std::placeholders;
    auto test_read_pass = std::bind(&decltype(test_read)::test_pass, &test_read, _1, _2, _3, _4, _5);
    auto test_read_fail = std::bind(&decltype(test_read)::test_fail, &test_read, _1, _2, _3, _4, _5);

    auto test_write_pass = std::bind(&decltype(test_write)::test_pass, &test_write, _1, _2, _3, _4, _5);

    test_quotes(test_read_pass, "Read test: empty file",
            "", {});

    test_quotes(test_read_pass, "Read test: no fields",
            "\r\n", {});

    test_quotes(test_read_pass, "Read test: 1 field",
            "1\r\n", {{"1"}});

    test_quotes(test_read_pass, "Read test: 1 quoted field",
            "\"1\"\r\n", {{"1"}});

    test_quotes(test_read_pass, "Read test: 1 empty quoted field",
            "\"\"\r\n", {{""}});

    test_quotes(test_read_pass, "Read test: 1 row",
            "1,2,3,4\r\n", {{"1", "2", "3", "4"}});

    test_quotes(test_read_pass, "Read test: 1 quoted row",
            "\"1\",\"2\",\"3\",\"4\"\r\n", {{"1", "2", "3", "4"}});

    test_quotes(test_read_pass, "Read test: empty fields",
            ",,,", {{"", "", "", ""}});

    test_quotes(test_read_pass, "Read test: escaped quotes",
            "\"\"\"1\"\"\",\"\"\"2\"\"\",\"\"\"3\"\"\",\"\"\"4\"\"\"\r\n",
            {{"\"1\"", "\"2\"", "\"3\"", "\"4\""}});

    test_quotes(test_read_fail, "Read test: unterminated quote",
            "\"1\r\n", {{""}});

    test_quotes(test_read_fail, "Read test: unescaped quote",
            "12\"3\r\n", {{"12\"3"}});

    test_quotes(test_read_fail, "Read test: unescaped quote at start of field",
            "\"123,234\r\n", {{""}});

    test_quotes(test_read_fail, "Read test: unescaped quote at end of field",
            "123,234\"\r\n", {{"123", "234\""}});

    test_quotes(test_read_fail, "Read test: unescaped quote inside quoted field",
            "\"12\"3\"\r\n", {{"12\"3"}});

    test_quotes(test_read_pass, "Read test: empty quoted fields",
            "\"\",\"\",\"\",\"\"\r\n", {{"", "", "", ""}});

    test_quotes(test_read_pass, "Read test: commas & newlines",
            "\"\n\",\"\r\",\"\r\n\",\",,\"\r\n", {{"\n", "\r", "\r\n", ",,"}});

    test_quotes(test_read_pass, "Read test: no CRLF at EOF",
            "1,2,3,4", {{"1", "2", "3", "4"}});

    test_quotes(test_read_pass, "Read test: last field empty",
            "1,2,3,\r\n", {{"1", "2", "3", ""}});

    test_quotes(test_read_pass, "Read test: last field empty - no CRLF at EOF",
            "1,2,3,", {{"1", "2", "3", ""}});

    test_quotes(test_read_pass, "Read test: 2 CRLFs at EOF",
            "1,2,3\r\n\r\n", {{"1", "2", "3"}});

    test_quotes(test_read_pass, "Read test: multirow",
            "1,2,3\r\n4,5,6\r\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_pass, "Read test: CR w/o LF",
            "1,2,3\r4,5,6\r", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_pass, "Read test: LF w/o CR",
            "1,2,3\n4,5,6\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_pass, "Read test: empty line in middle",
            "1,2,3\r\n\r\n4,5,6\r\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_pass, "Read test: many empty lines in middle",
            "1,2,3\r\n\r\n\r\n\r\n4,5,6\r\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_pass, "Read test: mixed empty lines in middle",
            "1,2,3\r\n\n\r\n\r\r\n\r\n\r4,5,6\r\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_fail, "Read test: mixed empty lines in middle, then parse error",
            "1,2,3\r\n\n\r\n\r\r\n\r\n\r4,5,\"6\r\n", {{"1", "2", "3"}, {"4", "5", "6"}});

    test_quotes(test_read_fail, "Read test: Too many cols",
            "1,2,3,4,5\r\n", {{"1", "2", "3", "4"}});

    test_quotes(test_read_fail, "Read test: Too few cols",
            "1,2,3\r\n", {{"1", "2", "3", "4"}});

    test_quotes(test_read_fail, "Read test: Too many rows",
            "1,2,3\r\n1,2,3\r\n", {{"1", "2", "3"}});

    test_quotes(test_read_fail, "Read test: Too few rows",
            "1,2,3\r\n", {{"1", "2", "3"}, {"1", "2", "3"}});

    test_quotes(test_read_pass, "Read test: multiple rows",
            "1,2,3,4\r\n5,6,7,8\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8"}});

    test_quotes(test_read_pass, "Read test: fewer than header",
            "1,2,3,4\r\n5,6,7\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7"}});

    test_quotes(test_read_pass, "Read test: more than header",
            "1,2,3,4\r\n5,6,7,8,9\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8", "9"}});

    test_quotes(test_read_pass, "Read test: header not sorted",
            "3,1,5\r\n6,2,4\r\n", {{"3", "1", "5"}, {"6", "2", "4"}});

    test_quotes(test_read_pass, "Read test: field reallocation",
            "1,123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412342,3,4",
            {{"1", "123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412341234123412342", "3", "4"}});

    {
    std::string test_str;
    const int test_nums = 42;
    CSV_data data(1);

    for(int i = 0; i < test_nums; ++i)
    {
        auto num = std::to_string(i);
        data[0].push_back(num);

        test_str += num;
        if(i < test_nums - 1)
            test_str += ",";
    }
    test_str += "\r\n";

    test_quotes(test_read_pass, "Read test: fields reallocation", test_str, data);
    }

    std::cout<<"\nWriter Tests:\n";

    test_quotes(test_write_pass, "Write test: empty file",
            "", {});

    test_quotes(test_write_pass, "Write test: 1 field",
            "1\r\n", {{"1"}});

    test_quotes(test_write_pass, "Write test: field with quotes",
            "\"\"\"1\"\"\"\r\n", {{"\"1\""}});

    test_quotes(test_write_pass, "Write test: field with commas & newlines",
            "\",1\r\n\"\r\n", {{",1\r\n"}});

    test_quotes(test_write_pass, "Write test: 1 row",
            "1,2,3,4\r\n", {{"1","2","3","4"}});

    test_quotes(test_write_pass, "Write test: multiple rows",
            "1,2,3,4\r\n5,6,7,8\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8"}});

    test_quotes(test_write_pass, "Write test: empty fields",
            "1,2,3,\r\n,6,7,8\r\n", {{"1", "2", "3", ""}, {"", "6", "7", "8"}});

    test_quotes(test_write_pass, "Write test: fewer than header",
            "1,2,3,4\r\n5,6,7\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7"}});

    test_quotes(test_write_pass, "Write test: more than header",
            "1,2,3,4\r\n5,6,7,8,9\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8", "9"}});

    std::cout<<"\n";

    remove("test.csv");

    if(test_read.passed() && test_write.passed())
    {
        auto num_passed = test_read.get_num_passed() + test_write.get_num_passed();
        std::cout<<"All "<<num_passed<<" tests PASSED\n";
        return EXIT_SUCCESS;
    }
    else
    {
        auto num_failed = test_read.get_num_ran() + test_write.get_num_ran()
            - test_read.get_num_passed() - test_write.get_num_passed();
        std::cout<<num_failed<<" tests FAILED.\n";
        return EXIT_FAILURE;
    }
}
