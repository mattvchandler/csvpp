// Copyright 2019 Matthew Chandler

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

// Expanded form of tinycsv.cpp
// Only functional difference is that the expanded version has more specific error
// messages (throws an error string instead of 0)

#include <string>
#include <vector>

#include <cstdio>
using namespace std;

pair<string, int> parse(FILE * in)
{
    if(feof(in))
        return {{}, 0};

    int quoted = 0, end_of_row = 0, c;
    string field;

    while(1)
    {
        c = getc(in);

        if(c==EOF && !feof(in))
            throw "error reading from stream";

        if(c == '"')
        {
            if(quoted)
            {
                c = getc(in);

                // end of the field?
                if(c == ',' || c == '\n' || c == '\r' || feof(in))
                    quoted = 0;
                // if it's not an escaped quote, then it's an error
                else if(c != '"')
                    throw "Unecaped double-quote";
            }
            else
            {
                if(field.empty())
                {
                    quoted = 1;
                    continue;
                }
                else
                {
                    // quotes are not allowed inside of an unquoted field
                    throw "Double-quote found in unquoted field";
                }
            }
        }

        if(feof(in) && quoted)
        {
            throw "Unterminated quoted field - reached end-of-file";
        }
        else if(!quoted && c == ',')
        {
            break;
        }
        else if(!quoted && (c == '\n' || c == '\r' || feof(in)))
        {
            end_of_row = 1;
            // consume newlines
            while(!feof(in))
            {
                c = getc(in);
                if(c != '\r' && c != '\n')
                {
                    if(!feof(in))
                        ungetc(c, in);
                    break;
                }
            }
            break;
        }

        field += c;
    }

    return {field, end_of_row};
}

int main(int argc, char * argv[])
{
    if(argc != 2) return 1;

    auto in = fopen(argv[1], "r");
    try
    {
        if(!in) throw "could not open file";

        vector<vector<string>> data(1);
        vector<int> col_size;
        while(1)
        {
            const auto & [field, end_of_row] = parse(in);
            if(ferror(in)) break;
            data.back().push_back(field);

            if(col_size.size() < data.back().size())
                col_size.emplace_back();

            col_size[data.back().size() - 1] = max((int)col_size[data.back().size() - 1], (int)field.size());
            if(feof(in)) break;

            if(end_of_row)
                data.emplace_back();
        }
        for(auto &row: data)
        {
            for(int i = 0; i < (int)size(row); ++i)
            {
                if(i != 0) printf(" | ");
                printf("%-*s", (int)col_size[i], row[i].c_str());
            }
            putchar('\n');
        }
    }
    catch(const char * what)
    {
        puts(what); return 1;
    }
    return 0;
}
