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

#include "csvpp/csv.hpp"

#include "parse_args.h"

int main(int argc, char * argv[])
{
    Args args;
    if(!parse_args(&args, argc, argv))
        return EXIT_FAILURE;

    try
    {
        auto input{ std::string{args.filename} == "-" ? csv::Reader{ std::cin,      args.delimiter, args.quote, args.lenient }
                                                      : csv::Reader{ args.filename, args.delimiter, args.quote, args.lenient }};

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
        return EXIT_FAILURE;
    }
    catch(const csv::IO_error & e)
    {
        std::cout << "I/O error: " << e.what()<<'\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
