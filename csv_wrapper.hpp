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
    CSV_wrapper_parse_error(const std::string & what): std::runtime_error{what} {}
};
class CSV_reader_wrapper
{
private:
    CSV_reader * csv_r = nullptr;

public:
    // forwards to C ctor / dtor
    explicit CSV_reader_wrapper(const char * filename): csv_r{ CSV_reader_init_from_filename(filename) }
    {
        if(!csv_r)
            throw std::runtime_error{"Could not init " + std::string{filename}};
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
    void set_delimiter(const char delimiter) const { CSV_reader_set_delimiter(csv_r, delimiter); }
    void set_quote(const char quote) const { CSV_reader_set_quote(csv_r, quote); }
    bool end_of_row() const { return CSV_reader_end_of_row(csv_r); }
    bool eof() const { return CSV_reader_eof(csv_r); }

    // make the interface more C++ friendly
    std::optional<std::vector<std::string>> read_record() const
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
    void set_delimiter(const char delimiter) const { CSV_writer_set_delimiter(csv_w, delimiter); }
    void set_quote(const char quote) const { CSV_writer_set_quote(csv_w, quote); }

    // make the interface more C++ friendly
    void write_record(const std::vector<std::string> & fields) const
    {
        for(auto &i: fields)
        {
            if(CSV_writer_write_field(csv_w, i.c_str()) != CSV_OK)
                throw std::runtime_error{"Error writing csv"};
        }
        if(CSV_writer_end_row(csv_w) != CSV_OK)
            throw std::runtime_error{"Error writing csv"};
    }
};

#endif // CSV_WRAPPER_HPP
