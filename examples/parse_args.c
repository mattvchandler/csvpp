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

#include "parse_args.h"

#include <stdio.h>
#include <string.h>

// DIY getopt replacement - to remove dependencies

void usage(const char * prog_name)
{
    fprintf(stderr, "usage: %s [-d DELIMITER] [-q QUOTE_CHAR] [-l] [-h] [CSV_FILE]\n", prog_name);
}

void help(const char * prog_name)
{
    usage(prog_name);
    fputs("\n"
        "CSV pretty-printer\n\n"
        "positional arguments:\n"
        "  CSV_FILE             CSV file to read. omit or pass '-' to read from stdin\n\n"
        "optional arguments:\n"
        "  -d, --delimiter      character to use as field delimiter (default: ,)\n"
        "  -q, --quote          character to use for quoted fields  (default: \")\n"
        "  -l, --lenient        parse incorrectly quoted fields leniently\n"
        "  -h, --help           show this help message and exit\n",
        stderr);
}

bool parse_args(Args * args, int argc, char * argv[])
{
    args->filename = "-";
    args->quote = '"';
    args->delimiter = ',';
    args->lenient = false;

    const char * prog_name = argv[0] + strlen(argv[0]);
    while(prog_name != argv[0] && *(prog_name - 1) != '/' && *(prog_name - 1) != '\\')
        --prog_name;

    enum {GET_OPT, GET_DELIMITER_ARG, GET_QUOTE_ARG} mode = GET_OPT;
    int positional_count = 0;

    for(int i = 1; i < argc; ++i)
    {
        const char * arg = argv[i];
        int arglen = strlen(arg);
        if(arglen == 0)
            continue;

        if(mode == GET_OPT)
        {
            if(arg[0] == '-')
            {
                if(arglen > 1 && arg[1] == '-')
                {
                    if(strcmp(arg + 2, "delimiter") == 0)
                        mode = GET_DELIMITER_ARG;
                    else if(strcmp(arg + 2, "quote") == 0)
                        mode = GET_QUOTE_ARG;
                    else if(strcmp(arg + 2, "lenient") == 0)
                        args->lenient = true;
                    else if(strcmp(arg + 2, "help") == 0)
                    {
                        help(prog_name);
                        return false;
                    }
                    else
                    {
                        fprintf(stderr, "unknown option: %s\n", arg);
                        usage(prog_name);
                        return false;
                    }
                }
                else
                {
                    for(int j = 1; j < arglen; ++j)
                    {
                        switch(arg[j])
                        {
                            case 'd':
                                if(j != arglen - 1)
                                {
                                    usage(prog_name);
                                    fputs("Option ‘d’ requires an argument\n", stderr);
                                    return false;
                                }
                                mode = GET_DELIMITER_ARG;
                                break;
                            case 'q':
                                if(j != arglen - 1)
                                {
                                    fputs("Option ‘q’ requires an argument\n", stderr);
                                    usage(prog_name);
                                    return false;
                                }
                                mode = GET_QUOTE_ARG;
                                break;
                            case 'l':
                                args->lenient = true;
                                break;
                            case 'h':
                                help(prog_name);
                                return false;
                            default:
                                if(arglen > 1)
                                {
                                    fprintf(stderr, "unknown option: -%c\n", arg[j]);
                                    usage(prog_name);
                                    return false;
                                }
                                else
                                {
                                    if(++positional_count == 1)
                                        args->filename = arg;
                                    else
                                    {
                                        fputs("Too many arguments\n", stderr);
                                        usage(prog_name);
                                        return false;
                                    }
                                }
                        }
                    }
                }
            }
            else
            {
                if(++positional_count == 1)
                    args->filename = arg;
                else
                {
                    fputs("Too many arguments\n", stderr);
                    usage(prog_name);
                    return false;
                }
            }
        }
        else if(mode == GET_DELIMITER_ARG)
        {
            args->delimiter = arg[0];
            mode = GET_OPT;
        }
        else if(mode == GET_QUOTE_ARG)
        {
            args->quote = arg[0];
            mode = GET_OPT;
        }
    }

    if(mode == GET_DELIMITER_ARG)
    {
        usage(prog_name);
        fputs("Option ‘--delimiter’ requires an argument\n", stderr);
        return false;
    }
    else if(mode == GET_QUOTE_ARG)
    {
        usage(prog_name);
        fputs("Option ‘--quote’ requires an argument\n", stderr);
        return false;
    }

    return true;
}
