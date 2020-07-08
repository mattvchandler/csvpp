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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <getopt.h>

#include "csv.h"

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

bool parse_args(int argc, char ** argv,
                FILE ** file, char * delimiter, char * quote, bool * lenient)
{
    *file = stdin;
    *delimiter = ',';
    *quote = '"';
    *lenient = false;

    int opt = 0;
    extern char * optarg;
    extern int optind, optopt;

    struct option longopts[] =
    {
        {"delimiter", required_argument, NULL, 'd'},
        {"quote",     required_argument, NULL, 'q'},
        {"lenient",   no_argument,       NULL, 'l'},
        {"help",      no_argument,       NULL, 'h'},
        {NULL,        0,                 NULL,  0}
    };

    const char * prog_name = argv[0] + strlen(argv[0]);
    while(prog_name != argv[0] && *(prog_name - 1) != '/' && *(prog_name - 1) != '\\')
        --prog_name;

    int ind = 0;
    while((opt = getopt_long(argc, argv, ":d:q:lh", longopts, &ind)) != -1)
    {
        switch(opt)
        {
        case 'd':
            *delimiter = optarg[0];
            break;
        case 'q':
            *quote = optarg[0];
            break;
        case 'l':
            *lenient = true;
            break;
        case 'h':
            help(prog_name);
            return false;
        case ':':
            fprintf(stderr, "Argument required for -%c\n", (char)(optopt));
            usage(prog_name);
            return false;
        case '?':
        default:
            fprintf(stderr, "unknown option: -%c\n", (char)(optopt));
            usage(prog_name);
            return false;
        }
    }

    if(argc - optind > 1)
    {
        fputs("Too many arguments\n", stderr);
        usage(prog_name);
        return false;
    }
    else if(argc - optind == 1 && strcmp(argv[optind], "-") != 0)
    {
        *file = fopen(argv[optind], "rb");
        if(!*file)
        {
            fprintf(stderr, "Unable to open input file %s: ", argv[optind]);
            perror(NULL);
            return false;
        }
    }

    return true;
}

typedef struct vector
{
    char * data_;
    size_t alloc_;
    size_t size_;
    size_t elem_size_;
    void(*free_fun_)(void *);
} vector;

vector * vector_init(size_t elem_size, void(*free_fun)(void *))
{
    assert(sizeof(char) == 1);

    vector * v = (vector *)malloc(sizeof(vector));
    v->alloc_ = 2;
    v->size_  = 0;
    v->elem_size_ = elem_size;
    v->free_fun_ = free_fun;

    v->data_ = (char *)malloc(v->elem_size_ * v->alloc_);

    return v;
}

void vector_append(vector *v, void * element)
{
    if(v->size_ == v->alloc_)
    {
        v->alloc_ *= 2;
        v->data_ = (char *)realloc(v->data_, v->elem_size_ * v->alloc_);
    }

    memcpy(&v->data_[v->size_++ * v->elem_size_], element, v->elem_size_);
}

void * vector_at(vector * v, size_t i)
{
    return &v->data_[i * v->elem_size_];
}

size_t vector_size(const vector * v)
{
    return v->size_;
}

void vector_free(vector * v)
{
    if(v)
    {
        if(v->free_fun_)
        {
            for(size_t i = 0; i < v->size_; ++i)
                v->free_fun_(*(void **)vector_at(v, i));
        }
        free(v->data_);
        free(v);
    }
}

int main(int argc, char ** argv)
{
    FILE * file = NULL;
    char delimiter, quote;
    bool lenient;

    if(!parse_args(argc, argv, &file, &delimiter, &quote, &lenient))
        return EXIT_FAILURE;

    CSV_reader * input = CSV_reader_init_from_file(file);
    CSV_reader_set_delimiter(input, delimiter);
    CSV_reader_set_quote(input, quote);
    CSV_reader_set_lenient(input, lenient);

    vector * data = vector_init(sizeof(CSV_row *), (void(*)(void*))CSV_row_free);
    vector * col_size = vector_init(sizeof(int), NULL);

    int ret_val = EXIT_SUCCESS;

    CSV_row * row;
    while((row = CSV_reader_read_row(input)))
    {
        vector_append(data, &row);

        for(size_t i = 0; i < CSV_row_size(row); ++i)
        {
            const char * field = CSV_row_get(row, i);
            int field_size = strlen(field);

            if(i == vector_size(col_size))
                vector_append(col_size, &field_size);
            else
            {
                int * current_size = (int*)vector_at(col_size, i);
                if(field_size > *current_size)
                    *current_size = field_size;
            }
        }
    }

    if(CSV_reader_get_error(input) == CSV_EOF)
    {
        for(size_t i = 0; i < vector_size(data); ++i)
        {
            row = *(CSV_row **)vector_at(data, i);
            for(size_t j = 0; j < CSV_row_size(row); ++j)
            {
                if(j != 0)
                    printf(" | ");

                printf("%-*s", *(int*)vector_at(col_size, j), CSV_row_get(row, j));
            }
            putchar('\n');
        }
    }
    else
    {
        fprintf(stderr, "Error reading CSV: %s\n", CSV_reader_get_error_msg(input));
        ret_val = EXIT_FAILURE;
    }

    vector_free(data);
    vector_free(col_size);
    CSV_reader_free(input);

    if(file != stdin)
        fclose(file);

    return ret_val;
}
