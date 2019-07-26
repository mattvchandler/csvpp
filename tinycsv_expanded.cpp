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

    int quoted = 0, c;
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
                    throw "Unescaped double-quote";
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
        if(!quoted && c == ',')
        {
            break;
        }
        if(!quoted && (c == '\n' || c == '\r' || feof(in)))
        {
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
            return {field, 1};
        }

        field += c;
    }

    return {field, 0};
}

int main(int argc, char * argv[])
{
    if(argc > 2)
    {
        fputs("limit 1 input file\n", stderr);
        return 1;
    }

    FILE * in = NULL;
    if(argc < 2 || argv[1] == string{"-"})
        in = stdin;
    else
        in = fopen(argv[1], "r");

    try
    {
        if(!in) throw "could not open file";

        vector<vector<string>> data(1);
        vector<int> col_size;
        for(int i = 0;;)
        {
            auto [field, end_of_row] = parse(in);
            if(ferror(in)) break;
            data.back().push_back(field);

            col_size.resize(max(i + 1, (int)col_size.size()));

            col_size[i] = max(col_size[i], (int)field.size());
            if(feof(in)) break;

            if(end_of_row)
            {
                i = 0;
                data.push_back({});
            }
            else
                ++i;
        }
        for(auto &row: data)
        {
            for(int i = 0; i < size(row); ++i)
            {
                if(i != 0) printf(" | ");
                printf("%-*s", col_size[i], std::data(row[i]));
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
