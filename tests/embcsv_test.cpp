#include "embcsv_test.hpp"

#include "../embcsv.h"

test::Result test_read_embedded(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    #ifdef EMBCSV_NO_MALLOC
    for(auto & row: expected_data)
    {
        for(auto & col: row)
        {
            if(std::size(col) >= EMBCSV_FIELD_BUF_SIZE - 1)
                return test::skip();
        }
    }
    #endif

    CSV_data data;

    #ifndef EMBCSV_NO_MALLOC
    EMBCSV_reader * r = EMBCSV_reader_init_full(delimiter, quote, lenient);
    #else
    EMBCSV_reader r_obj;
    EMBCSV_reader_init_full(&r_obj, delimiter, quote, lenient);
    auto r = &r_obj;
    #endif

    bool new_row = true;
    for(const char * c = csv_text.c_str();; ++c)
    {
        const char * field = nullptr;
        switch(EMBCSV_reader_parse_char(r, *c, &field))
        {
            case EMBCSV_INCOMPLETE:
                break;
            case EMBCSV_FIELD:
                if(new_row)
                    data.emplace_back();

                data.back().emplace_back(field);
                new_row = false;
                break;
            case EMBCSV_END_OF_ROW:
                if(new_row)
                    data.emplace_back();

                data.back().emplace_back(field);
                new_row = true;
                break;
            case EMBCSV_PARSE_ERROR:
                #ifndef EMBCSV_NO_MALLOC
                EMBCSV_reader_free(r);
                #endif
                return test::error();
        }
        if(!*c)
            break;
    }

    #ifndef EMBCSV_NO_MALLOC
    EMBCSV_reader_free(r);
    #endif

    return CSV_test_suite::common_read_return(csv_text, expected_data, data);
}

void Embcsv_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_embedded);
}
