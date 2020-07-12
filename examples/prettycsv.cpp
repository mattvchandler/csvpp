// Copyright 2020 Matthew Chandler

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

#include <iostream>
#include <iomanip>
#include <optional>

#include <cxxopts.hpp>

#include "csvpp/csv.hpp"

std::optional<std::tuple<std::string, char, char, bool>> parse_args(int argc, char * argv[])
{
    cxxopts::Options options{"prettycsv", "Pretty-Print CSV files"};
    try
    {
        options.add_options()
            ("d,delimiter", "delimiter character",        cxxopts::value<char>()->default_value(","),  "DELIMITER")
            ("q,quote",     "quote character",            cxxopts::value<char>()->default_value("\""), "QUOTE")
            ("l,lenient",   "ignore parsing errors")
            ("h,help",      "Show this message and quit")
            ("filename",    "CSV input file to display",  cxxopts::value<std::string>());

        options.parse_positional("filename");
        options.positional_help("CSV_FILE");

        auto args = options.parse(argc, argv);

        if(args.count("help"))
        {
            std::cerr<<options.help()<<'\n';
            return {};
        }

        std::string filename { args.count("filename") ? args["filename"].as<std::string>() : std::string{"-"} };
        char delimiter = args["delimiter"].as<char>();
        char quote = args["quote"].as<char>();
        bool lenient = args.count("lenient");

        return std::tuple{filename, delimiter, quote, lenient};
    }
    catch(const cxxopts::OptionException & e)
    {
        std::cerr<<options.help()<<'\n'<<e.what()<<'\n';
        return {};
    }
}

int main(int argc, char * argv[])
{
    auto args = parse_args(argc, argv);
    if(!args)
        return 1;

    try
    {
        auto & [filename, delimiter, quote, lenient] = *args;
        auto input{ filename == "-" ? csv::Reader{ std::cin, delimiter, quote, lenient }
                                    : csv::Reader{ filename, delimiter, quote, lenient }};

        std::vector<std::vector<std::string>> data;
        std::vector<std::size_t> col_size(4);

        for(auto & row: input)
        {
            data.emplace_back();
            std::size_t i = 0;
            for(auto & field: row)
            {
                if(i == std::size(col_size))
                    col_size.resize(std::size(col_size) * 2);

                col_size[i] = std::max(col_size[i], std::size(field));

                data.back().emplace_back(std::move(field));
                ++i;
            }
        }

        for(auto & row: data)
        {
            for(std::size_t i = 0; i < std::size(row); ++i)
            {
                if(i != 0)
                    std::cout << " | ";

                std::cout << std::left << std::setw(col_size[i]) << row[i];
            }
            std::cout << '\n';
        }
    }
    catch(const csv::Parse_error & e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    catch(const csv::IO_error & e)
    {
        std::cout << "I/O error: " << e.what()<<'\n';
        return 1;
    }

    return 0;
}
