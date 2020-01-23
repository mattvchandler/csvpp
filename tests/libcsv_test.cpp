#include "libcsv_test.hpp"

#include <csv.h>

struct libcsv_read_status
{
    CSV_data data;
    bool start_of_row{true};
};

void libcsv_read_cb1(void * field, size_t, void * data)
{
    libcsv_read_status * stat = reinterpret_cast<libcsv_read_status*>(data);

    if(stat->start_of_row)
        stat->data.emplace_back();

    stat->data.back().push_back(reinterpret_cast<const char *>(field));
    stat->start_of_row = false;
}

void libcsv_read_cb2(int, void * data)
{
    libcsv_read_status * stat = reinterpret_cast<libcsv_read_status*>(data);
    stat->start_of_row = true;
}

test::Result test_read_libcsv(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    for(auto & row: expected_data)
    {
        for(auto & col: row)
        {
            if(col.front() == ' ' || col.back() == ' ')
                return test::skip();
        }
    }

    auto lenient_flags = lenient ? 0 : CSV_STRICT | CSV_STRICT_FINI;

    csv_parser parse;
    if(csv_init(&parse, CSV_APPEND_NULL | lenient_flags) != 0)
        throw std::runtime_error("Could not init libcsv");

    csv_set_delim(&parse, delimiter);
    csv_set_quote(&parse, quote);

    libcsv_read_status stat;

    auto error = [&parse]()
    {
        std::cerr<<csv_strerror(csv_error(&parse))<<"\n";
        csv_free(&parse);
    };

    if(csv_parse(&parse, csv_text.c_str(), csv_text.size(), libcsv_read_cb1, libcsv_read_cb2, &stat) < csv_text.size())
    {
        error();
        return test::error();
    }
    csv_fini(&parse, libcsv_read_cb1, libcsv_read_cb2, &stat);

    int err_code = csv_error(&parse);
    if(err_code != CSV_SUCCESS)
    {
        error();
        return test::error();
    }

    csv_free(&parse);

    return CSV_test_suite::common_read_return(csv_text, expected_data, stat.data);
}

test::Result test_write_libcsv(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    if(delimiter != ',' || quote != '"')
        return test::skip();

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

    return CSV_test_suite::common_write_return(data, expected_text, output);
}

void Libcsv_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_libcsv);

    tests.register_write_test(test_write_libcsv);
}
