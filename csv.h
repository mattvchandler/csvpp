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

#ifndef CSV_H
#define CSV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

char * CSV_strdup(const char * src);

// CSV reader object
typedef struct _CSV_reader
{
} CSV_reader;

// CSV writer object
typedef struct _CSV_writer
{
} CSV_writer;

// Record object
typedef enum {CSV_OK, CSV_EOF, CSV_EMPTY_ROW, CSV_IO_ERROR, CSV_PARSE_ERROR, CSV_INTERNAL_ERROR} CSV_status;

// create a new CSV reader object
CSV_reader * CSV_reader_init_from_filename(const char * filename);

// delete a CSV reader object
void CSV_reader_free(CSV_reader * reader);

// set delimiter character
void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter);

// set quote character
void CSV_reader_set_quote(CSV_reader * reader, const char quote);

// read the current record from the CSV file and advance to the next
// fields_out is owned by CSV_reader and must not be freed or modified by the caller
CSV_status CSV_reader_read_record(CSV_reader * reader, char *** fields_out, size_t * num_fields_out);

// create a new CSV writer object
CSV_writer * CSV_writer_init_from_filename(const char * filename);

// delete a CSV writer object
void CSV_writer_free(CSV_writer * writer);

// set delimiter character
void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter);

// set quote character
void CSV_writer_set_quote(CSV_writer * writer, const char quote);

// write a record
CSV_status CSV_writer_write_record(CSV_writer * writer, const char *const * fields, const size_t num_fields);

#ifdef __cplusplus
}
#endif

#endif // CSV_H
