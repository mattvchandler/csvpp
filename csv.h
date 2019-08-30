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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    CSV_OK,                      // no errors, ready to read / write another field
    CSV_EOF,                     // reached end of file
    CSV_PARSE_ERROR,             // parsing error. see CSV_reader_get_error_msg for details
    CSV_IO_ERROR,                // IO error
    CSV_TOO_MANY_FIELDS_WARNING, // more fields exist in one row than will fit in given storage. Non fatal
    CSV_INTERNAL_ERROR           // Illegal reader / writer state reached
} CSV_status;

// CSV reader object
typedef struct CSV_reader CSV_reader;

// CSV writer object
typedef struct CSV_writer CSV_writer;

// dynamic array of strings structure for convenience
typedef struct CSV_row CSV_row;

// strdup implementation, in case it's not implemented in string.h
char * CSV_strdup(const char * src);

CSV_row * CSV_row_init(void);
void CSV_row_free(CSV_row * rec);

// take ownership of field and append it to the row
void CSV_row_append(CSV_row * rec, char * field);

size_t CSV_row_size(const CSV_row * rec);
const char * CSV_row_get(const CSV_row * rec, size_t i);

// get access to char ** within CSV_row (useful for passing to other interfaces)
const char * const * CSV_row_arr(const CSV_row * rec);

// create a new CSV reader object parsing from a file
CSV_reader * CSV_reader_init_from_filename(const char * filename);

// create a new CSV reader object parsing from a FILE *
CSV_reader * CSV_reader_init_from_file(FILE * file);

// create a new CSV reader object parsing from an in-memory string
CSV_reader * CSV_reader_init_from_str(const char * input);

// delete a CSV reader object
void CSV_reader_free(CSV_reader * reader);

// set delimiter character
void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter);

// set quote character
void CSV_reader_set_quote(CSV_reader * reader, const char quote);

// enable / disable lenient parsing
void CSV_reader_set_lenient(CSV_reader * reader, const bool lenient);

// read a single field. returns NULL on error, otherwise, return is owned by caller
// end_of_row_out will be set true if this is the last field in a row
char * CSV_reader_read_field(CSV_reader * reader);

// variadic read. pass char **'s followed by NULL. caller will own all char *'s returned
// discards any fields remaining until the end of the row
// returns CSV_OK on successful read or other error code on failure
CSV_status CSV_reader_read_v(CSV_reader * reader, ...);


// read the current row from the CSV file and advance to the next
// returns null on error, otherwise caller should free the return value with CSV_row_free
CSV_row * CSV_reader_read_row(CSV_reader * reader);

// read a row into an array of fields. if fields is null, this will allocate it (and pass ownership to the caller),
// and return the final size into num_fields
// otherwise, will overwrite fields contents with strings (owned by caller) up to the limit specified in num_fields.
// will discard any fields remaining until the end of the row
// returns CSV_OK on successful read, CSV_TOO_MANY_FIELDS_WARNING fields is not null and there are more fields than num_fields
// or other error code on failure
CSV_status CSV_reader_read_row_ptr(CSV_reader * reader, char *** fields, size_t * num_fields);

bool CSV_reader_eof(const CSV_reader * reader);
bool CSV_reader_end_of_row(const CSV_reader * reader);

// get message for last error. returns null when no error has occurred
const char * CSV_reader_get_error_msg(const CSV_reader * reader);

// get error code for last error
CSV_status CSV_reader_get_error(const CSV_reader * reader);

// create a new CSV writer object writing to a file
CSV_writer * CSV_writer_init_from_filename(const char * filename);

// Create a new CSV_writer object writing to a FILE *
CSV_writer * CSV_writer_init_from_file(FILE * file);

// create a new CSV writer object writing to a string
CSV_writer * CSV_writer_init_to_str(void);

// delete a CSV writer object and close the file
void CSV_writer_free(CSV_writer * writer);

// set delimiter character
void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter);

// set quote character
void CSV_writer_set_quote(CSV_writer * writer, const char quote);

// end the current row (for use w/ CSV_writer_write_field)
CSV_status CSV_writer_end_row(CSV_writer * writer);

// write a single field. Use CSV_writer_end_row to move to the next row
CSV_status CSV_writer_write_field(CSV_writer * writer, const char * field);

// write a CSV_row object
CSV_status CSV_writer_write_row(CSV_writer * writer, const CSV_row * fields);

// write an array of strings as a row
CSV_status CSV_writer_write_row_ptr(CSV_writer * writer, const char *const * fields, const size_t num_fields);

// write const char * arguments as a row. last argument should be NULL
CSV_status CSV_writer_write_row_v(CSV_writer * writer, ...);

// get the string output (when initialized w/ CSV_writer_init_to_str, otherwise returns NULL)
const char * CSV_writer_get_str(CSV_writer * writer);

#ifdef __cplusplus
}
#endif

#endif // CSV_H
