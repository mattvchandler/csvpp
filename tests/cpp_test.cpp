#include "cpp_test.hpp"

#include <optional>
#include <sstream>

#include "csv.hpp"

std::optional<std::vector<std::vector<int>>> convert_to_int(const CSV_data & expected_data)
{
    std::vector<std::vector<int>> expected_ints;
    for(auto & row: expected_data)
    {
        expected_ints.emplace_back();
        for(auto & col: row)
        {
            try
            {
                expected_ints.back().push_back(std::stoi(col));
                if(std::to_string(expected_ints.back().back()) != col)
                    return {};
            }
            catch(const std::invalid_argument & e)
            {
                // TODO; c++20 start_with
                if(std::string_view{e.what(), 4} == "stoi")
                    return {};
                throw;
            }
            catch(const std::out_of_range & e)
            {
                if(std::string_view{e.what(), 4} == "stoi")
                    return {};
                throw;
            }
        }
    }
    return expected_ints;
}

test::Result test_read_mine_cpp_read_all(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        return CSV_test_suite::common_read_return(csv_text, expected_data, csv::Reader(csv::Reader::input_string, csv_text, delimiter, quote, lenient).read_all());
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_read_row_vec(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;
        while(true)
        {
            if(auto row = r.read_row_vec(); row)
                data.push_back(*row);
            else
                break;
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_read_all_as_int(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    auto expected_ints = convert_to_int(expected_data);
    if(!expected_ints)
        return test::skip();

    try
    {
        auto data = csv::Reader(csv::Reader::input_string, csv_text, delimiter, quote, lenient).read_all<int>();

        auto failure_fun = [csv_text, data, expected_data]()
        {
            CSV_data str_data;
            for(auto & row: data)
            {
                str_data.emplace_back();
                for(auto & col: row)
                {
                    str_data.back().push_back(std::to_string(col));
                }
            }
            std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);   std::cout << '\n';
            std::cout << "expected: "; CSV_test_suite::print_data(expected_data); std::cout << '\n';
            std::cout << "got:      "; CSV_test_suite::print_data(str_data);      std::cout << "\n\n";
        };
        return test::pass_fail(data == *expected_ints, failure_fun);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_read_row(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;

        while(true)
        {
            std::vector<std::string> row;
            if(r.read_row(std::back_inserter(row)))
                data.emplace_back(std::move(row));
            else
                break;
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_stream(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;
        std::string field;
        bool start_of_row = true;

        while(r>>field)
        {
            if(start_of_row)
                data.emplace_back();

            data.back().push_back(field);
            start_of_row = r.end_of_row();
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_fields(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;
        std::string field;
        bool start_of_row = true;
        while(field = r.read_field(), !r.eof())
        {
            if(start_of_row)
                data.emplace_back();

            data.back().push_back(field);
            start_of_row = r.end_of_row();
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_iters(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        CSV_data data;

        for(auto & row: r)
        {
            data.emplace_back();
            std::copy(row.begin(), row.end(), std::back_inserter(data.back()));
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_range(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        CSV_data data;

        for(auto & row: r)
        {
            data.emplace_back();
            for(auto & field: row)
                data.back().push_back(field);
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_fields(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;
        while(r)
        {
            auto row = r.get_row();
            if(!row)
                break;

            std::vector<std::string> row_v;

            while(!row.end_of_row())
                row_v.push_back(row.read_field());

            data.emplace_back(std::move(row_v));
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_stream(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);

        CSV_data data;
        while(r)
        {
            auto row = r.get_row();
            if(!row)
                break;
            std::vector<std::string> row_v;

            std::string field;
            while(row>>field)
                row_v.emplace_back(std::move(field));

            data.emplace_back(std::move(row_v));
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_vec(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        CSV_data data;
        for(auto & row: r)
            data.emplace_back(row.read_vec());

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_vec_as_int(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    auto expected_ints = convert_to_int(expected_data);
    if(!expected_ints)
        return test::skip();

    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        std::vector<std::vector<int>> data;
        for(auto & row: r)
            data.emplace_back(row.read_vec<int>());

        auto failure_fun = [csv_text, data, expected_data]()
        {
            CSV_data str_data;
            for(auto & row: data)
            {
                str_data.emplace_back();
                for(auto & col: row)
                {
                    str_data.back().push_back(std::to_string(col));
                }
            }
            std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);   std::cout << '\n';
            std::cout << "expected: "; CSV_test_suite::print_data(expected_data); std::cout << '\n';
            std::cout << "got:      "; CSV_test_suite::print_data(str_data);      std::cout << "\n\n";
        };
        return test::pass_fail(data == *expected_ints, failure_fun);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_map(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Map_reader_iter r{csv::Reader::input_string, csv_text, std::string{"<DEFAULT VALUE>"}, {}, delimiter, quote, lenient};

        auto & headers = expected_data.at(0);
        if(r.get_headers() != headers)
            return test::fail([csv_text, headers, got_headers = r.get_headers()]()
                    {
                        std::cout<<"could not read headers:\n";

                        std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);   std::cout << '\n';
                        std::cout << "expected: "; CSV_test_suite::print_data({headers});     std::cout << '\n';
                        std::cout << "got:      "; CSV_test_suite::print_data({got_headers}); std::cout << "\n\n";
                    });

        std::size_t i = 1;
        for(; r != csv::Map_reader_iter{} && i < std::size(expected_data); ++r, ++i)
        {
            if(std::size(expected_data[i]) != std::size(headers))
                return test::skip();

            std::remove_reference_t<decltype(*r)> expected_row;
            for(std::size_t j = 0; j < std::size(headers); ++j)
                expected_row.emplace(headers[j], expected_data[i][j]);

            if(*r != expected_row)
            {
                return test::fail([csv_text, headers, expected_row = expected_data[i], r = *r]
                        {
                            std::cout<<"row mismatch:\n";
                            std::vector<std::string> row_v;

                            for(auto & h: headers)
                                row_v.emplace_back(std::move(r.at(h)));

                            std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);    std::cout << '\n';
                            std::cout << "expected: "; CSV_test_suite::print_data({expected_row}); std::cout << '\n';
                            std::cout << "got:      "; CSV_test_suite::print_data({row_v});        std::cout << "\n\n";
                        });
            }
        }
        if(i != std::size(expected_data) || r != csv::Map_reader_iter{})
            return test::fail([](){ std::cout<<"wrong # of rows\n"; });

        return test::pass();
    }
    catch(const csv::Parse_error & e)
    {
        if(e.what() == std::string{"Error parsing CSV at line: 0, col: 0: Can't get header row"} && std::size(expected_data) == 0)
            return test::pass();
        else
        {
            // std::cerr<<e.what()<<"\n";
            return test::error();
        }
    }
    catch(csv::Out_of_range_error & e)
    {
        if(e.what() == std::string{"Too many fields"})
            return test::skip([](){ std::cout<<"wrong # of cols\n"; });
        else
            throw;
    }
}

test::Result test_read_mine_cpp_map_as_int(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    auto expected_ints = convert_to_int(expected_data);
    if(!expected_ints)
        return test::skip();

    try
    {
        csv::Map_reader_iter<int, int> r(csv::Reader::input_string, csv_text, {}, {}, delimiter, quote, lenient);

        auto headers = expected_ints->at(0);
        if(r.get_headers() != headers)
            return test::fail([csv_text, headers, got_headers=r.get_headers()]()
                    {
                        std::cout<<"could not read headers:\n";
                        std::vector<std::string> str_headers, str_got_headers;
                        std::transform(std::begin(headers),     std::end(headers),     std::back_inserter(str_headers),     [](const int & i){ return std::to_string(i); });
                        std::transform(std::begin(got_headers), std::end(got_headers), std::back_inserter(str_got_headers), [](const int & i){ return std::to_string(i); });

                        std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);       std::cout << '\n';
                        std::cout << "expected: "; CSV_test_suite::print_data({str_headers});     std::cout << '\n';
                        std::cout << "got:      "; CSV_test_suite::print_data({str_got_headers}); std::cout << "\n\n";
                    });

        std::size_t i = 1;
        for(; r != csv::Map_reader_iter{} && i < std::size(*expected_ints); ++r, ++i)
        {
            if(std::size((*expected_ints)[i]) != std::size(headers))
                return test::skip();

            std::remove_reference_t<decltype(*r)> expected_row;
            for(std::size_t j = 0; j < std::size(headers); ++j)
                expected_row.emplace(headers[j], (*expected_ints)[i][j]);

            if(*r != expected_row)
            {
                return test::fail([csv_text, headers, expected_row = expected_data[i], r = *r]
                        {
                            std::cout<<"row mismatch:\n";
                            std::vector<std::string> row_v;

                            for(auto & h: headers)
                                row_v.emplace_back(std::to_string(r.at(h)));

                            std::cout << "given:    "; CSV_test_suite::print_escapes(csv_text);    std::cout << '\n';
                            std::cout << "expected: "; CSV_test_suite::print_data({expected_row}); std::cout << '\n';
                            std::cout << "got:      "; CSV_test_suite::print_data({row_v});        std::cout << "\n\n";
                        });
            }
        }
        if(i != std::size(*expected_ints) || r != csv::Map_reader_iter{})
                return test::fail([](){ std::cout<<"wrong # of rows\n"; });

        return test::pass();
    }
    catch(const csv::Parse_error & e)
    {
        if(e.what() == std::string{"Error parsing CSV at line: 0, col: 0: Can't get header row"} && std::size(expected_data) == 0)
            return test::pass();
        else
        {
            // std::cerr<<e.what()<<"\n";
            return test::error();
        }
    }
    catch(const csv::Out_of_range_error &e)
    {
        if(e.what() == std::string{"Too many fields"})
            return test::skip([](){ std::cout<<"wrong # of cols\n"; });
        else
            throw;
    }
}

test::Result test_read_mine_cpp_variadic(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        auto parsed_data = csv::Reader{csv::Reader::input_string, csv_text, delimiter, quote, lenient}.read_all(); // this will catch any parse errors, but it gives us how many columns in each row, which we need to run this test.

        CSV_data data;
        for(auto & expected_row: parsed_data)
        {
            std::vector<std::string> row(std::size(expected_row));
            try
            {
                switch(std::size(row))
                {
                case 0:
                    r.read_v();
                    break;
                case 1:
                    r.read_v(row[0]);
                    break;
                case 2:
                    r.read_v(row[0], row[1]);
                    break;
                case 3:
                    r.read_v(row[0], row[1], row[2]);
                    break;
                case 4:
                    r.read_v(row[0], row[1], row[2], row[3]);
                    break;
                case 5:
                    r.read_v(row[0], row[1], row[2], row[3], row[4]);
                    break;
                default:
                    return test::skip();
                }
            }
            catch(const csv::Out_of_range_error& e)
            {
                if(e.what() == std::string{"Read past end of row"})
                    return test::fail();
                throw;
            }

            data.push_back(row);
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_tuple(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        auto parsed_data = csv::Reader{csv::Reader::input_string, csv_text, delimiter, quote, lenient}.read_all(); // this will catch any parse errors, but it gives us how many columns in each row, which we need to run this test.

        CSV_data data;
        for(auto & expected_row: parsed_data)
        {
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
                    return test::skip();
                }
            }
            catch(const csv::Out_of_range_error & e)
            {
                if(e.what() == std::string{"Read past end of row"})
                    return test::fail();
                throw;
            }

            data.push_back(row);
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_variadic(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        auto parsed_data = csv::Reader{csv::Reader::input_string, csv_text, delimiter, quote, lenient}.read_all(); // this will catch any parse errors, but it gives us how many columns in each row, which we need to run this test.

        CSV_data data;
        for(auto & expected_row: parsed_data)
        {
            auto row_obj = r.get_row();
            std::vector<std::string> row(std::size(expected_row));

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
                    return test::skip();
                }
            }
            catch(const csv::Out_of_range_error&)
            {
                return test::fail();
            }

            data.emplace_back(row);
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}

test::Result test_read_mine_cpp_row_tuple(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    try
    {
        csv::Reader r(csv::Reader::input_string, csv_text, delimiter, quote, lenient);
        auto parsed_data = csv::Reader{csv::Reader::input_string, csv_text, delimiter, quote, lenient}.read_all(); // this will catch any parse errors, but it gives us how many columns in each row, which we need to run this test.

        CSV_data data;

        for(auto & expected_row: parsed_data)
        {
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
                    return test::skip();
                }
            }
            catch(const csv::Out_of_range_error&)
            {
                return test::fail();
            }

            data.emplace_back(row);
        }

        return CSV_test_suite::common_read_return(csv_text, expected_data, data);
    }
    catch(const csv::Parse_error & e)
    {
        // std::cerr<<e.what()<<"\n";
        return test::error();
    }
}
test::Result test_write_mine_cpp_stream(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
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
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_row(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: data)
        {
            w.write_row(row);
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_iter(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
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
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_stream_as_int(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    auto int_data = convert_to_int(data);
    if(!int_data)
        return test::skip();

    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: *int_data)
        {
            for(const auto & col: row)
                w<<col;
            w<<csv::end_row;
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_row_as_int(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    auto int_data = convert_to_int(data);
    if(!int_data)
        return test::skip();

    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: *int_data)
        {
            w.write_row(row);
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_iter_as_int(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    auto int_data = convert_to_int(data);
    if(!int_data)
        return test::skip();

    std::ostringstream str;
    { // scoped so dtor is called before checking result
        csv::Writer w(str, delimiter, quote);
        for(const auto & row: *int_data)
        {
            std::copy(row.begin(), row.end(), w.iterator());
            w.end_row();
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_variadic(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
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
                return test::skip();
            }
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}
test::Result test_write_mine_cpp_tuple(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
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
                return test::skip();
            }
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

test::Result test_write_mine_cpp_map(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    std::ostringstream str;
    if(!std::empty(data))
    {
        auto headers =  data[0];
        csv::Map_writer_iter w(str, headers, {}, delimiter, quote);
        for(auto row = std::begin(data) + 1; row != std::end(data); ++row, ++w)
        {
            if(std::size(*row) != std::size(headers))
                return test::skip();

            std::map<std::string, std::string> out_row;
            for(std::size_t i = 0; i < std::size(headers); ++i)
                out_row[headers[i]] = (*row)[i];

            *w = out_row;
        }
    }
    return CSV_test_suite::common_write_return(data, expected_text, str.str());
}

void Cpp_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_mine_cpp_read_all);
    tests.register_read_test(test_read_mine_cpp_read_row_vec);
    tests.register_read_test(test_read_mine_cpp_read_all_as_int);
    tests.register_read_test(test_read_mine_cpp_read_row);
    tests.register_read_test(test_read_mine_cpp_stream);
    tests.register_read_test(test_read_mine_cpp_fields);
    tests.register_read_test(test_read_mine_cpp_iters);
    tests.register_read_test(test_read_mine_cpp_range);
    tests.register_read_test(test_read_mine_cpp_row_fields);
    tests.register_read_test(test_read_mine_cpp_row_stream);
    tests.register_read_test(test_read_mine_cpp_row_vec);
    tests.register_read_test(test_read_mine_cpp_row_vec_as_int);
    tests.register_read_test(test_read_mine_cpp_map);
    tests.register_read_test(test_read_mine_cpp_map_as_int);
    tests.register_read_test(test_read_mine_cpp_variadic);
    tests.register_read_test(test_read_mine_cpp_tuple);
    tests.register_read_test(test_read_mine_cpp_row_variadic);
    tests.register_read_test(test_read_mine_cpp_row_tuple);

    tests.register_write_test(test_write_mine_cpp_stream);
    tests.register_write_test(test_write_mine_cpp_row);
    tests.register_write_test(test_write_mine_cpp_iter);
    tests.register_write_test(test_write_mine_cpp_stream_as_int);
    tests.register_write_test(test_write_mine_cpp_row_as_int);
    tests.register_write_test(test_write_mine_cpp_iter_as_int);
    tests.register_write_test(test_write_mine_cpp_variadic);
    tests.register_write_test(test_write_mine_cpp_tuple);
    tests.register_write_test(test_write_mine_cpp_map);
}
