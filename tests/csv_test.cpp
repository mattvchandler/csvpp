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

#ifdef CSV_ENABLE_LIBCSV
#include "libcsv_test.hpp"
#endif

#ifdef CSV_ENABLE_PYTHON
#include "python_test.hpp"
#endif

#ifdef CSV_ENABLE_C_CSV
#include "c_test.hpp"
#endif

#ifdef CSV_ENABLE_EMBCSV
#include "embcsv_test.hpp"
#endif

#ifdef CSV_ENABLE_TINYCSV
#include "tinycsv_test.hpp"
#endif

#ifdef CSV_ENABLE_CPP_CSV
#include "cpp_test.hpp"
#endif

#include "csv_test_suite.hpp"

char * csv_quote(const char * text, const char delimiter, const char quote)
{
    size_t char_count = strlen(text);
    int quoted = 0;

    const char * c;
    for(c = text; *c; ++c)
    {
        if(*c == delimiter || *c == '\r' || *c == '\n')
            quoted = 1;
        else if(*c == quote)
        {
            quoted = 1;
            ++char_count;
        }
    }

    if(!quoted)
        return strdup(text);

    char * quoted_text = (char *)malloc(char_count + 3); // null and 2 quotes
    char * q = quoted_text;
    *q++ = quote;
    for(c = text; *c; ++c)
    {
        *q++ = *c;
        if(*c == quote)
            *q++ = *c;
    }
    *q++ = quote;
    *q = '\0';

    return quoted_text;
}

test::Result test_write_qd(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    std::ostringstream oss;
    for(auto &row: data)
    {
        bool first_col = true;
        for(auto & col: row)
        {
            if(!first_col)
                oss<<delimiter;
            first_col = false;

            auto quoted = csv_quote(col.c_str(), delimiter, quote);

            oss<<quoted;
            free(quoted);
        }
        oss<<"\r\n";
    }

    return CSV_test_suite::common_write_return(data, expected_text, oss.str());
}

int main(int, char *[])
{
    CSV_test_suite tests;

    #ifdef CSV_ENABLE_EMBCSV
    tests.register_tests(Embcsv_test{});
    #endif

    #ifdef CSV_ENABLE_C_CSV
    tests.register_tests(C_test{});
    #endif

    #ifdef CSV_ENABLE_TINYCSV
    tests.register_tests(Tinycsv_test{});
    #endif

    #ifdef CSV_ENABLE_PYTHON
    Python_test py;
    tests.register_tests(py);
    #endif

    #ifdef CSV_ENABLE_LIBCSV
    tests.register_tests(Libcsv_test{});
    #endif

    #ifdef CSV_ENABLE_CPP_CSV
    tests.register_tests(Cpp_test{});
    #endif

    // tests.register_read_test(test_read_intcode);
    tests.register_write_test(test_write_qd);

    tests.run_tests();
}
