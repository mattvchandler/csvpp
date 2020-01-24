#ifndef CSV_TEST_SUITE_HPP
#define CSV_TEST_SUITE_HPP

#include <functional>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "test.hpp"

using CSV_data = std::vector<std::vector<std::string>>;

class CSV_test_suite
{
public:
    class CSV_test_group
    {
    public:
        virtual ~CSV_test_group() = default;
        virtual void register_tests(CSV_test_suite & tests) const = 0;
        friend CSV_test_suite;
    };

    static test::Result common_read_return(const std::string & csv_text, const CSV_data & expected_data, const CSV_data & data)
    {
        return test::pass_fail(data == expected_data, [csv_text, expected_data, data]()
        {
            std::cout << "given:    "; print_escapes(csv_text);   std::cout << '\n';
            std::cout << "expected: "; print_data(expected_data); std::cout << '\n';
            std::cout << "got:      "; print_data(data);          std::cout << "\n\n";
        });
    }

    static test::Result common_write_return(const CSV_data & data, const std::string & expected_text, const std::string & csv_text)
    {
        return test::pass_fail(csv_text == expected_text, [data, expected_text, csv_text]()
        {
            std::cout << "given:    "; print_data(data);             std::cout << '\n';
            std::cout << "expected: "; print_escapes(expected_text); std::cout << '\n';
            std::cout << "got:      "; print_escapes(csv_text);      std::cout << "\n\n";
        });
    }

    static void print_escapes(const std::string & text, bool escape_quote = false)
    {
        for(auto & c: text)
        {
            switch(c)
            {
            case '\r':
                std::cout<<"\\r";
                break;
            case '\n':
                std::cout<<"\\n";
                break;
            case '"':
                if(escape_quote)
                {
                    std::cout<<"\\\"";
                    break;
                }
                [[fallthrough]];
            default:
                std::cout<<c;
                break;
            }
        }
    }

    static void print_data(const CSV_data & data)
    {
        bool first_row = true;
        std::cout<<'{';
        for(auto & row: data)
        {
            if(!first_row)
                std::cout<<", ";
            first_row = false;

            std::cout<<'{';
            bool first_col = true;
            for(auto & col: row)
            {
                if(!first_col)
                    std::cout<<", ";
                first_col = false;

                std::cout<<'"';
                print_escapes(col, true);
                std::cout<<'"';
            }
            std::cout<<'}';
        }
        std::cout<<'}';
    };

    using Read_test_fun = std::function<test::Result(const std::string&, const CSV_data&, const char, const char, const bool)>;
    using Write_test_fun = std::function<test::Result(const std::string&, const CSV_data&, const char, const char)>;

    void register_read_test(Read_test_fun test_fun)
    {
        read_tests_.emplace_back(test_fun);
    }

    void register_write_test(Write_test_fun test_fun)
    {
        write_tests_.emplace_back(test_fun);
    }

    void register_tests(const CSV_test_group & group)
    {
        group.register_tests(*this);
    }


    bool run_tests() const
    {
        test::Test<const std::string&, const CSV_data&, const char, const char, const bool> test_read(read_tests_);
        test::Test<const std::string&, const CSV_data&, const char, const char> test_write(write_tests_);

        // create a bound function obj for test_read & test_write's pass & fail methods
        using namespace std::placeholders;
        auto test_read_pass  = std::bind(&decltype(test_read)::test_pass,  &test_read,  _1, _2, _3, _4, _5, _6);
        auto test_read_fail  = std::bind(&decltype(test_read)::test_fail,  &test_read,  _1, _2, _3, _4, _5, _6);
        auto test_read_error = std::bind(&decltype(test_read)::test_error, &test_read,  _1, _2, _3, _4, _5, _6);
        auto test_write_pass = std::bind(&decltype(test_write)::test_pass, &test_write, _1, _2, _3, _4, _5);

        if(!std::empty(test_read))
        {
            std::cout<<"Reader Tests:\n";

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

            test_quotes(test_read_pass, "Read test: leading space",
                    " 1, 2, 3, 4\r\n", {{" 1", " 2", " 3", " 4"}});

            test_quotes(test_read_pass, "Read test: trailing space",
                    "1 ,2 ,3 ,4 \r\n", {{"1 ", "2 ", "3 ", "4 "}});

            test_quotes(test_read_pass, "Read test: leading & trailing space",
                    " 1 , 2 , 3 , 4 \r\n", {{" 1 ", " 2 ", " 3 ", " 4 "}});

            test_quotes(test_read_pass, "Read test: 1 quoted row",
                    "\"1\",\"2\",\"3\",\"4\"\r\n", {{"1", "2", "3", "4"}});

            test_quotes(test_read_pass, "Read test: empty fields",
                    ",,,", {{"", "", "", ""}});

            test_quotes(test_read_pass, "Read test: escaped quotes",
                    "\"\"\"1\"\"\",\"\"\"2\"\"\",\"\"\"3\"\"\",\"\"\"4\"\"\"\r\n",
                    {{"\"1\"", "\"2\"", "\"3\"", "\"4\""}});

            test_quotes(test_read_error, "Read test: unterminated quote",
                    "\"1\r\n", {{"<parse error>"}});

            test_quotes(test_read_pass, "Read test: unterminated quote (lenient)",
                    "\"1\r\n", {{"1\r\n"}}, true);

            test_quotes(test_read_error, "Read test: unescaped quote",
                    "12\"3\r\n", {{"<parse error>"}});

            test_quotes(test_read_pass, "Read test: unescaped quote (lenient)",
                    "12\"3\r\n", {{"12\"3"}}, true);

            test_quotes(test_read_error, "Read test: unescaped quote at start of field",
                    "\"123,234\r\n", {{"<parse error>"}});

            test_quotes(test_read_pass, "Read test: unescaped quote at start of field (lenient)",
                    "\"123,234\r\n", {{"123,234\r\n"}}, true);

            test_quotes(test_read_error, "Read test: unescaped quote at end of field",
                    "123,234\"\r\n", {{"<parse error>"}});

            test_quotes(test_read_pass, "Read test: unescaped quote at end of field (lenient)",
                    "123,234\"\r\n", {{"123", "234\""}}, true);

            test_quotes(test_read_error, "Read test: unescaped quote inside quoted field",
                    "\"12\"3\"\r\n", {{"<parse error>"}});

            test_quotes(test_read_pass, "Read test: unescaped quote inside quoted field (lenient)",
                    "\"12\"3\"\r\n", {{"12\"3"}}, true);

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

            test_quotes(test_read_pass, "Read test: mixed empty lines in middle, then parse error (lenient)",
                    "1,2,3\r\n\n\r\n\r\r\n\r\n\r4,5,\"6\r\n", {{"1", "2", "3"}, {"4", "5", "6\r\n"}}, true);

            test_quotes(test_read_fail, "Read test: Too many cols",
                    "1,2,3,4,5\r\n", {{"1", "2", "3", "4"}});

            test_quotes(test_read_fail, "Read test: Too few cols",
                    "1,2,3\r\n", {{"1", "2", "3", "4"}});

            test_quotes(test_read_fail, "Read test: Too many rows",
                    "1,2,3\r\n1,2,3\r\n", {{"1", "2", "3"}});

            test_quotes(test_read_fail, "Read test: Too few rows",
                    "1,2,3\r\n", {{"1", "2", "3"}, {"1", "2", "3"}});

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
        }

        if(!std::empty(test_write))
        {
            std::cout<<"\nWriter Tests:\n";

            test_quotes(test_write_pass, "Write test: empty file",
                    "", {});

            test_quotes(test_write_pass, "Write test: 1 field",
                    "1\r\n", {{"1"}});

            test_quotes(test_write_pass, "Write test: field with quotes",
                    "\"\"\"1\"\"\"\r\n", {{"\"1\""}});

            test_quotes(test_write_pass, "Write test: 1 row",
                    "1,2,3,4\r\n", {{"1","2","3","4"}});

            test_quotes(test_write_pass, "Write test: fields with commas",
                    "\"1,2,3\",\"4,5,6\"\r\n", {{"1,2,3", "4,5,6"}});

            test_quotes(test_write_pass, "Write test: fields with newlines",
                    "\"1\r2\n3\",\"4\r\n5\n\r6\"\r\n", {{"1\r2\n3", "4\r\n5\n\r6"}});

            test_quotes(test_write_pass, "Write test: field with commas & newlines",
                    "\",1\r\n\"\r\n", {{",1\r\n"}});

            test_quotes(test_write_pass, "Write test: fields with commas & newlines & quotes",
                    "\",1\r\n\"\"\"\r\n", {{",1\r\n\""}});

            test_quotes(test_write_pass, "Write test: multiple rows",
                    "1,2,3,4\r\n5,6,7,8\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8"}});

            test_quotes(test_write_pass, "Write test: empty fields",
                    "1,2,3,\r\n,6,7,8\r\n", {{"1", "2", "3", ""}, {"", "6", "7", "8"}});

            test_quotes(test_write_pass, "Write test: fewer than header",
                    "1,2,3,4\r\n5,6,7\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7"}});

            test_quotes(test_write_pass, "Write test: more than header",
                    "1,2,3,4\r\n5,6,7,8,9\r\n", {{"1", "2", "3", "4"}, {"5", "6", "7", "8", "9"}});

            std::cout<<"\n";
        }

        if(test_read.passed() && test_write.passed())
        {
            auto num_passed = test_read.get_num_passed() + test_write.get_num_passed();
            auto num_skipped = test_read.get_num_skipped() + test_write.get_num_skipped();
            std::cout<<"All "<<num_passed<<" tests PASSED. ("<<num_skipped<<" tests skipped)\n";
            return true;
        }
        else
        {
            auto num_passed = test_read.get_num_passed() + test_write.get_num_passed();
            auto num_failed = test_read.get_num_ran() + test_write.get_num_ran() - num_passed;
            auto num_skipped = test_read.get_num_skipped() + test_write.get_num_skipped();
            std::cout<<num_passed<<" tests PASSED, "<<num_failed<<" tests FAILED. ("<<num_skipped<<" tests skipped)\n";
            return false;
        }
    }

private:
    // helper to run a test for each combination of delimiter and quote char
    template<typename Test>
    static void test_quotes(Test test, const std::string & title, const std::string & csv_text, const CSV_data& data, const bool lenient = false)
    {
        test(title, csv_text, data, ',', '"', lenient);

        std::string modified_csv_text = std::regex_replace(csv_text, std::regex(","), "|");
        CSV_data modified_data = data;
        for(auto & row: modified_data)
            for(auto & col: row)
                col = std::regex_replace(col, std::regex{","}, "|");

        test(title + " w/ pipe delimiter", modified_csv_text, modified_data, '|', '"', lenient);

        modified_csv_text = std::regex_replace(csv_text, std::regex("\""), "'");
        modified_data = data;
        for(auto & row: modified_data)
            for(auto & col: row)
                col = std::regex_replace(col, std::regex{"\""}, "'");

        test(title + " w/ single quote", modified_csv_text, modified_data, ',', '\'', lenient);

        modified_csv_text = std::regex_replace(csv_text, std::regex(","), "|");
        modified_csv_text = std::regex_replace(modified_csv_text, std::regex("\""), "'");
        modified_data = data;
        for(auto & row: modified_data)
            for(auto & col: row)
            {
                col = std::regex_replace(col, std::regex{","}, "|");
                col = std::regex_replace(col, std::regex{"\""}, "'");
            }

        test(title + " w/ pipe delimiter & single quote", modified_csv_text, modified_data, '|', '\'', lenient);
    }

    std::vector<Read_test_fun> read_tests_;
    std::vector<Write_test_fun> write_tests_;
};

#endif // CSV_TEST_SUITE_HPP
