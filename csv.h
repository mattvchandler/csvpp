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
    CSV_TOO_MANY_FIELDS_WARNING, // more fields exist in one record than will fit in given storage. Non fatal
    CSV_INTERNAL_ERROR           // Illegal reader / writer state reached
} CSV_status;

// CSV reader object
typedef struct CSV_reader CSV_reader;

// CSV writer object
typedef struct CSV_writer CSV_writer;

// dynamic array of strings structure for convenience
typedef struct CSV_record CSV_record;

// strdup implementation, in case it's not implemented in string.h
char * CSV_strdup(const char * src);

CSV_record * CSV_record_init();
void CSV_record_free(CSV_record * rec);

// take ownership of field and append it to the record
void CSV_record_append(CSV_record * rec, char * field);

size_t CSV_record_size(const CSV_record * rec);
const char * CSV_record_get(const CSV_record * rec, size_t i);

// get access to char ** within CSV_record (useful for passing to other interfaces)
const char * const * CSV_record_arr(const CSV_record * rec);

// create a new CSV reader object
CSV_reader * CSV_reader_init_from_filename(const char * filename);

// delete a CSV reader object
void CSV_reader_free(CSV_reader * reader);

// set delimiter character
void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter);

// set quote character
void CSV_reader_set_quote(CSV_reader * reader, const char quote);

// read a single field. returns NULL on error, otherwise, return is owned by caller
// end_of_row_out will be set true if this is the last field in a row
char * CSV_reader_read_field(CSV_reader * reader);

// read the current record from the CSV file and advance to the next
// returns null on error, otherwise caller should free the return value with CSV_record_free
CSV_record * CSV_reader_read_record(CSV_reader * reader);

// read a record into an array of fields. if fields is null, this will allocate it (and pass ownership to the caller),
// and return the final size into num_fields
// otherwise, will overwrite fields contents with strings (owned by caller) up to the limit specified in num_fields.
// will discard any fields remaining until the end of the row
// returns CSV_OK on successful read, CSV_TOO_MANY_FIELDS_WARNING fields is not null and there are more fields than num_fields
// or other error code on failure
CSV_status CSV_reader_read_record_ptr(CSV_reader * reader, char *** fields, size_t * num_fields);

// variadic read. pass char **'s followed by NULL. caller will own all char *'s returned
// discards any fields remaining until the end of the row
// returns CSV_OK on successful read, CSV_TOO_MANY_FIELDS_WARNING fields than passed in
// or other error code on failure
CSV_status CSV_reader_read_record_v(CSV_reader * reader, ...);

bool CSV_reader_eof(const CSV_reader * reader);
bool CSV_reader_end_of_row(const CSV_reader * reader);

// get message for last error. returns null when no error has occurred
const char * CSV_reader_get_error_msg(const CSV_reader * reader);

// get error code for last error
CSV_status CSV_reader_get_error(const CSV_reader * reader);

// create a new CSV writer object
CSV_writer * CSV_writer_init_from_filename(const char * filename);

// delete a CSV writer object and close the file
void CSV_writer_free(CSV_writer * writer);

// set delimiter character
void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter);

// set quote character
void CSV_writer_set_quote(CSV_writer * writer, const char quote);

// end the current row (for use w/ CSV_writer_write_field)
CSV_status CSV_writer_end_row(CSV_writer * writer);

// write a single field. Use CSV_writer_end_row to move to the next record
CSV_status CSV_writer_write_field(CSV_writer * writer, const char * field);

// write a CSV_record object
CSV_status CSV_writer_write_record(CSV_writer * writer, const CSV_record * fields);

// write an array of strings as a record
CSV_status CSV_writer_write_record_ptr(CSV_writer * writer, const char *const * fields, const size_t num_fields);

// write const char * arguments as a record. last argument should be NULL
CSV_status CSV_writer_write_record_v(CSV_writer * writer, ...);

#ifdef __cplusplus
}
#endif

#endif // CSV_H
