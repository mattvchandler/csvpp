#ifndef CSV_WRAPPER_HPP
#define CSV_WRAPPER_HPP

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "csv.h"

class CSV_wrapper_parse_error: public std::runtime_error
{
public:
    explicit CSV_wrapper_parse_error(const std::string & what): std::runtime_error{what} {}
};
class CSV_reader_wrapper
{
private:
    CSV_reader * csv_r = nullptr;

public:
    // forwards to C ctor / dtor
    explicit CSV_reader_wrapper(const std::string & filename): csv_r{ CSV_reader_init_from_filename(filename.c_str()) }
    {
        if(!csv_r)
            throw std::runtime_error{"Could not init " + filename};
    }

    // disambiguation tag
    struct input_string_t{};
    static inline constexpr input_string_t input_string{};
    CSV_reader_wrapper(input_string_t, const std::string & str): csv_r{ CSV_reader_init_from_str(str.c_str()) }
    {
        if(!csv_r)
            throw std::runtime_error{"Could not init from str: " + str};
    }

    ~CSV_reader_wrapper() { CSV_reader_free(csv_r); }

    // non-copyable
    CSV_reader_wrapper(const CSV_reader_wrapper &) = delete;
    CSV_reader_wrapper & operator=(const CSV_reader_wrapper &) = delete;

    // moveable
    CSV_reader_wrapper(CSV_reader_wrapper && other)
    {
        using namespace std;
        swap(csv_r, other.csv_r);
    }

    CSV_reader_wrapper & operator=(CSV_reader_wrapper && other)
    {
        if(&other != this)
        {
            using namespace std;
            swap(csv_r, other.csv_r);
        }
       return *this;
    }

    // allows you to use the wrapper object in C library calls (by casting to the C type)
    operator const CSV_reader *() const { return csv_r; }
    operator CSV_reader *() { return csv_r; }

    // wrappers for methods
    void set_delimiter(const char delimiter) { CSV_reader_set_delimiter(csv_r, delimiter); }
    void set_quote(const char quote) { CSV_reader_set_quote(csv_r, quote); }
    void set_lenient(const bool lenient) { CSV_reader_set_lenient(csv_r, lenient); }

    bool end_of_row() const { return CSV_reader_end_of_row(csv_r); }
    bool eof() const { return CSV_reader_eof(csv_r); }

    // make the interface more C++ friendly
    std::optional<std::vector<std::string>> read_row() const
    {
        std::vector<std::string> rec;

        while(true)
        {
            char * field = CSV_reader_read_field(csv_r);
            if(field)
            {
                rec.push_back(field);
                std::free(field);
            }
            else if(eof())
                return {};

            else
            {
                const char * msg = CSV_reader_get_error_msg(csv_r);
                if(!msg)
                    msg = "Internal error";

                switch(CSV_reader_get_error(csv_r))
                {
                case CSV_IO_ERROR:
                    throw std::ios_base::failure{msg};
                case CSV_PARSE_ERROR:
                    throw CSV_wrapper_parse_error{msg};
                default:
                    throw std::runtime_error{msg};
                }
            }

            if(end_of_row())
                break;
        }

        return rec;
    }
};

class CSV_writer_wrapper
{
private:
    CSV_writer * csv_w = nullptr;

public:
    // forwards to C ctor / dtor
    explicit CSV_writer_wrapper(const char * filename): csv_w{ CSV_writer_init_from_filename(filename) }
    {
        if(!csv_w)
            throw std::runtime_error{"Could not init " + std::string{filename}};
    }
    CSV_writer_wrapper(): csv_w{ CSV_writer_init_to_str() }
    {
        if(!csv_w)
            throw std::runtime_error{"Could not init for writing to string"};
    }
    ~CSV_writer_wrapper() { CSV_writer_free(csv_w); }

    // non-copyable
    CSV_writer_wrapper(const CSV_writer_wrapper &) = delete;
    CSV_writer_wrapper & operator=(const CSV_writer_wrapper &) = delete;

    // moveable
    CSV_writer_wrapper(CSV_writer_wrapper && other)
    {
        using namespace std;
        swap(csv_w, other.csv_w);
    }
    CSV_writer_wrapper & operator=(CSV_writer_wrapper && other)
    {
        if(&other != this)
        {
            using namespace std;
            swap(csv_w, other.csv_w);
        }
       return *this;
    }

    // allows you to use the wrapper object in C library calls (by casting to the C type)
    operator const CSV_writer *() const { return csv_w; }
    operator CSV_writer *() { return csv_w; }

    // wrappers for methods
    void set_delimiter(const char delimiter) { CSV_writer_set_delimiter(csv_w, delimiter); }
    void set_quote(const char quote) { CSV_writer_set_quote(csv_w, quote); }
    std::string get_str() { return std::string{ CSV_writer_get_str(csv_w) }; }

    // make the interface more C++ friendly
    void write_row(const std::vector<std::string> & fields) const
    {
        for(auto &i: fields)
        {
            switch(CSV_writer_write_field(csv_w, i.c_str()))
            {
            case CSV_OK:
                break;
            case CSV_IO_ERROR:
                throw std::ios_base::failure{"Error writing CSV"};
            default:
            case CSV_INTERNAL_ERROR:
                throw std::runtime_error{"Error writing CSV"};
            }
        }
        switch(CSV_writer_end_row(csv_w))
        {
        case CSV_OK:
            break;
        case CSV_IO_ERROR:
            throw std::ios_base::failure{"Error writing CSV"};
        default:
        case CSV_INTERNAL_ERROR:
            throw std::runtime_error{"Error writing CSV"};
        }
    }
};

#endif // CSV_WRAPPER_HPP
