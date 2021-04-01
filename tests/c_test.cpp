#include "c_test.hpp"

#include <array>

#include "csvpp/csv.h"

test::Result test_read_c_field(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    CSV_reader * r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data data;

    bool start_of_row = true;
    char * field = nullptr;
    while(field = CSV_reader_read_field(r), !CSV_reader_eof(r))
    {
        if(field)
        {
            if(start_of_row)
                data.emplace_back();
            data.back().emplace_back(field);
            free(field);
        }
        else
        {
            auto msg = CSV_reader_get_error_msg(r);
            switch(CSV_reader_get_error(r))
            {
            case CSV_PARSE_ERROR:
                std::cout<<msg<<'\n';
                CSV_reader_free(r);
                return test::error();

            case CSV_IO_ERROR:
                CSV_reader_free(r);
                throw std::runtime_error{msg};

            default:
                CSV_reader_free(r);
                throw std::runtime_error{std::string{"bad error for CSV_reader: "} + msg};
            }
        }

        start_of_row = CSV_reader_end_of_row(r);
    }

    CSV_reader_free(r);

    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_read_c_row(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    CSV_reader * r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data data;

    while(true)
    {
        auto rec = CSV_reader_read_row(r);
        if(rec)
        {
            data.emplace_back(CSV_row_arr(rec), CSV_row_arr(rec) + CSV_row_size(rec));
            CSV_row_free(rec);
        }

        else if(CSV_reader_eof(r))
            break;

        else
        {
            auto msg = CSV_reader_get_error_msg(r);
            switch(CSV_reader_get_error(r))
            {
            case CSV_PARSE_ERROR:
                // std::cout<<msg<<'\n';
                CSV_reader_free(r);
                return test::error();

            case CSV_IO_ERROR:
                CSV_reader_free(r);
                throw std::runtime_error{msg};

            default:
                CSV_reader_free(r);
                throw std::runtime_error{std::string{"bad error for CSV_reader: "} + msg};
            }
        }
    }

    CSV_reader_free(r);

    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_read_c_ptr(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    CSV_reader * r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data data;

    while(true)
    {
        std::array<char *, 5> rec;
        std::fill(std::begin(rec), std::end(rec), nullptr);

        std::size_t num_fields = std::size(rec);
        auto rec_data = std::data(rec);

        auto free_rec = [&rec]()
        {
            for(auto &i: rec)
                std::free(i);
        };

        auto status = CSV_reader_read_row_ptr(r, &rec_data, &num_fields);
        if(status == CSV_OK)
        {
            data.emplace_back(std::begin(rec), std::begin(rec) + num_fields);
            free_rec();
        }
        else if(CSV_reader_eof(r))
        {
            free_rec();
            break;
        }
        else
        {
            free_rec();
            auto msg = CSV_reader_get_error_msg(r);
            CSV_reader_free(r);
            switch(status)
            {
            case CSV_TOO_MANY_FIELDS_WARNING:
                return test::skip();

            case CSV_PARSE_ERROR:
                // std::cout<<msg<<'\n';
                return test::error();

            case CSV_IO_ERROR:
                throw std::runtime_error{msg};

            default:
                throw std::runtime_error{std::string{"bad error for CSV_reader: "} + msg};
            }
        }
    }
    CSV_reader_free(r);

    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_read_c_ptr_dyn(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    CSV_reader * r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data data;

    while(true)
    {
        char ** rec = NULL;
        std::size_t num_fields = 0;
        auto status = CSV_reader_read_row_ptr(r, &rec, &num_fields);
        if(status == CSV_OK)
        {
            data.emplace_back(rec, rec + num_fields);

            for(std::size_t i = 0; i < num_fields; ++i)
                std::free(rec[i]);
            std::free(rec);
        }

        else if(CSV_reader_eof(r))
            break;

        else
        {
            auto msg = CSV_reader_get_error_msg(r);
            CSV_reader_free(r);

            switch(status)
            {
            case CSV_PARSE_ERROR:
                // std::cout<<msg<<'\n';
                return test::error();

            case CSV_IO_ERROR:
                throw std::runtime_error{msg};

            default:
                throw std::runtime_error{std::string{"bad error for CSV_reader: "} + msg};
            }
        }
    }
    CSV_reader_free(r);

    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_read_c_variadic(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    CSV_reader * r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data parsed_data;

    // pre-parse the data (using the same logic as read_row)
    while(true)
    {
        auto rec = CSV_reader_read_row(r);
        if(rec)
        {
            parsed_data.emplace_back(CSV_row_arr(rec), CSV_row_arr(rec) + CSV_row_size(rec));
            CSV_row_free(rec);
        }

        else if(CSV_reader_eof(r))
            break;

        else
        {
            auto msg = CSV_reader_get_error_msg(r);
            switch(CSV_reader_get_error(r))
            {
            case CSV_PARSE_ERROR:
                // std::cout<<msg<<'\n';
                CSV_reader_free(r);
                return test::error();

            case CSV_IO_ERROR:
                CSV_reader_free(r);
                throw std::runtime_error{msg};

            default:
                CSV_reader_free(r);
                throw std::runtime_error{std::string{"bad error for CSV_reader: "} + msg};
            }
        }
    }

    CSV_reader_free(r);

    r = CSV_reader_init_from_str(csv_text.c_str());
    if(!r)
        throw std::runtime_error("could not init CSV_reader");

    CSV_reader_set_delimiter(r, delimiter);
    CSV_reader_set_quote(r, quote);
    CSV_reader_set_lenient(r, lenient);

    CSV_data data;

    for(auto & expected_row: parsed_data)
    {
        auto expected_size = std::size(expected_row);
        if(expected_size > 5)
        {
            CSV_reader_free(r);
            return test::skip();
        }

        std::vector<char *> row(expected_size, nullptr);

        auto free_row = [&row](){ for(auto & i: row) std::free(i); };

        CSV_status status = CSV_OK;
        switch(expected_size)
        {
        case 0:
            status = CSV_reader_read_v(r, NULL);
            break;
        case 1:
            status = CSV_reader_read_v(r, &row[0], NULL);
            break;
        case 2:
            status = CSV_reader_read_v(r, &row[0], &row[1], NULL);
            break;
        case 3:
            status = CSV_reader_read_v(r, &row[0], &row[1], &row[2], NULL);
            break;
        case 4:
            status = CSV_reader_read_v(r, &row[0], &row[1], &row[2], &row[3], NULL);
            break;
        case 5:
            status = CSV_reader_read_v(r, &row[0], &row[1], &row[2], &row[3], &row[4], NULL);
            break;
        }
        if(status != CSV_OK)
        {
            if(status == CSV_EOF)
                break;

            else
            {
                free_row();
                CSV_reader_free(r);

                switch(status)
                {
                case CSV_TOO_MANY_FIELDS_WARNING:
                    return test::skip();
                default:
                    throw std::runtime_error{"Got an error after checking for errors somehow"};
                }
            }
        }

        data.emplace_back(std::begin(row), std::end(row));
        free_row();
    }

    CSV_reader_free(r);
    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

test::Result test_write_c_field(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    CSV_writer * w = CSV_writer_init_to_str();
    if(!w)
        throw std::runtime_error{"Could not init CSV_writer"};

    CSV_writer_set_delimiter(w, delimiter);
    CSV_writer_set_quote(w, quote);

    for(const auto & row: data)
    {
        for(auto &i: row)
        {
            if(CSV_writer_write_field(w, i.c_str()) != CSV_OK)
            {
                CSV_writer_free(w);
                throw std::runtime_error{"error writing CSV"};
            }
        }
        if(CSV_writer_end_row(w) != CSV_OK)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"error writing CSV"};
        }
    }
    auto output = std::string{ CSV_writer_get_str(w) };
    CSV_writer_free(w);

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

test::Result test_write_c_fields(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    CSV_writer * w = CSV_writer_init_to_str();
    if(!w)
        throw std::runtime_error{"Could not init CSV_writer"};

    CSV_writer_set_delimiter(w, delimiter);
    CSV_writer_set_quote(w, quote);

    for(const auto & row: data)
    {
        auto rec = CSV_row_init();
        if(!rec)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"Could not init CSV_row"};
        }
        for(auto & i: row)
        {
            auto field = CSV_strdup(i.c_str());
            CSV_row_append(rec, field);
        }

        if(CSV_writer_write_fields(w, rec) != CSV_OK)
        {
            CSV_writer_free(w);
            CSV_row_free(rec);
            throw std::runtime_error{"error writing CSV"};
        }

        if(CSV_writer_end_row(w) != CSV_OK)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"error writing CSV"};
        }

        CSV_row_free(rec);
    }
    auto output = std::string{ CSV_writer_get_str(w) };
    CSV_writer_free(w);

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

test::Result test_write_c_row(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    CSV_writer * w = CSV_writer_init_to_str();
    if(!w)
        throw std::runtime_error{"Could not init CSV_writer"};

    CSV_writer_set_delimiter(w, delimiter);
    CSV_writer_set_quote(w, quote);

    for(const auto & row: data)
    {
        auto rec = CSV_row_init();
        if(!rec)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"Could not init CSV_row"};
        }
        for(auto & i: row)
        {
            auto field = CSV_strdup(i.c_str());
            CSV_row_append(rec, field);
        }

        if(CSV_writer_write_row(w, rec) != CSV_OK)
        {
            CSV_writer_free(w);
            CSV_row_free(rec);
            throw std::runtime_error{"error writing CSV"};
        }

        CSV_row_free(rec);
    }
    auto output = std::string{ CSV_writer_get_str(w) };
    CSV_writer_free(w);

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

test::Result test_write_c_ptr(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    CSV_writer * w = CSV_writer_init_to_str();
    if(!w)
        throw std::runtime_error{"Could not init CSV_writer"};

    CSV_writer_set_delimiter(w, delimiter);
    CSV_writer_set_quote(w, quote);

    for(const auto & row: data)
    {
        std::vector<const char *> col_c_strs;
        for(const auto & col: row)
            col_c_strs.push_back(col.c_str());

        if(CSV_writer_write_row_ptr(w, col_c_strs.data(), row.size()) != CSV_OK)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"error writing CSV"};
        }
    }
    auto output = std::string{ CSV_writer_get_str(w) };
    CSV_writer_free(w);

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

test::Result test_write_c_variadic(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    CSV_writer * w = CSV_writer_init_to_str();
    if(!w)
        throw std::runtime_error{"Could not init CSV_writer"};

    CSV_writer_set_delimiter(w, delimiter);
    CSV_writer_set_quote(w, quote);

    for(const auto & row: data)
    {
        CSV_status status = CSV_OK;

        switch(std::size(row))
        {
        case 0:
            status = CSV_writer_write_row_v(w, NULL);
            break;
        case 1:
            status = CSV_writer_write_row_v(w, row[0].c_str(), NULL);
            break;
        case 2:
            status = CSV_writer_write_row_v(w, row[0].c_str(), row[1].c_str(), NULL);
            break;
        case 3:
            status = CSV_writer_write_row_v(w, row[0].c_str(), row[1].c_str(), row[2].c_str(), NULL);
            break;
        case 4:
            status = CSV_writer_write_row_v(w, row[0].c_str(), row[1].c_str(), row[2].c_str(), row[3].c_str(), NULL);
            break;
        case 5:
            status = CSV_writer_write_row_v(w, row[0].c_str(), row[1].c_str(), row[2].c_str(), row[3].c_str(), row[4].c_str(), NULL);
            break;
        default:
            CSV_writer_free(w);
            return test::skip();
        }
        if(status != CSV_OK)
        {
            CSV_writer_free(w);
            throw std::runtime_error{"error writing CSV"};
        }
    }
    auto output = std::string{ CSV_writer_get_str(w) };
    CSV_writer_free(w);

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

void C_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_c_field);
    tests.register_read_test(test_read_c_row);
    tests.register_read_test(test_read_c_ptr);
    tests.register_read_test(test_read_c_ptr_dyn);
    tests.register_read_test(test_read_c_variadic);

    tests.register_write_test(test_write_c_field);
    tests.register_write_test(test_write_c_fields);
    tests.register_write_test(test_write_c_row);
    tests.register_write_test(test_write_c_ptr);
    tests.register_write_test(test_write_c_variadic);
}
