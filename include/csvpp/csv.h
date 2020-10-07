/// @file
/// @brief C CSV library

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

/// @defgroup c C Library

#ifndef CSV_H
#define CSV_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define CSVPP_VERSION_MAJOR 1
#define CSVPP_VERSION_MINOR 0
#define CSVPP_VERSION_PATCH 0

/// Status codes

/// Status codes returned by CSV_reader and CSV_writer methods, or available
/// from CSV_reader_get_error()
/// @ingroup c
typedef enum {
    CSV_OK,                      ///< No errors, ready to read / write another field
    CSV_EOF,                     ///< Reached end of file
    CSV_PARSE_ERROR,             ///< Parsing error. See CSV_reader_get_error_msg for details
    CSV_IO_ERROR,                ///< IO error
    CSV_TOO_MANY_FIELDS_WARNING, ///< More fields exist in one row than will fit in given storage. Non fatal
    CSV_INTERNAL_ERROR           ///< Illegal reader / writer state reached
} CSV_status;

/// strdup implementation, in case it's not implemented in string.h

/// @param src String to duplicate
/// @returns Duplicated string. Caller is responsible for freeing it
/// @ingroup c
char * CSV_strdup(const char * src);

/// @defgroup c_row CSV_row
/// @ingroup c
/// @brief A dynamic array of strings
/// @details Used as input to CSV_writer_write_row() and output from CSV_reader_read_row()

/// @ingroup c_row
typedef struct CSV_row CSV_row;

/// Create a new CSV_row

/// @returns New CSV_row. Free with CSV_row_free()
/// @ingroup c_row
CSV_row * CSV_row_init(void);

/// Free a CSV_row

/// Will also free all strings added by CSV_row_append()
/// @ingroup c_row
void CSV_row_free(CSV_row * rec);

/// Append a new field

/// @param field String to append to the array. \c rec will take ownership of the
///              \c field and will free it when CSV_row_free() is called on it
/// @ingroup c_row
void CSV_row_append(CSV_row * rec, char * field);

/// Get CSV_Record array size

/// @returns Size of array
/// @ingroup c_row
size_t CSV_row_size(const CSV_row * rec);

/// CSV_row array element access

/// @param i index to access
/// @returns string at index \c i (read-only - use CSV_strdup if you need a permanent copy)
/// @ingroup c_row
const char * CSV_row_get(const CSV_row * rec, size_t i);

/// CSV_row array access

/// @returns `char *` array within CSV_row (read only). May be useful for
/// passing to other libraries
/// @ingroup c_row
const char * const * CSV_row_arr(const CSV_row * rec);

/// @defgroup c_reader CSV_reader
/// @ingroup c
/// @brief CSV Reader / parser
/// @details By default, parses according to RFC 4180 rules, and will stop with
/// an error when given non-conformant input. The field delimiter and quote
/// characters may be changed, and there is a lenient parsing option to ignore
/// violations.
///
/// Blank rows are ignored and skipped over.
///
/// Contains both row-wise and field-wise methods. Mixing these is not
/// recommended, but is possible. Row-wise methods will act as if the current
/// position is the start of a row, regardless of any fields that have been read
/// from the current row so far.

/// @ingroup c_reader
typedef struct CSV_reader CSV_reader;

/// Create a new CSV_reader object parsing from a file

/// @param filename Path to file
/// @returns New CSV_reader object. Free with CSV_reader_free()
/// @returns NULL if unable to open the file. Use strerror / perror for details
/// @ingroup c_reader
CSV_reader * CSV_reader_init_from_filename(const char * filename);

/// Create a new CSV_reader object parsing from a FILE *

/// @param file FILE * opened in read mode. Caller remains responsible to call \c
/// fclose on the file use
/// @returns New CSV_reader object. Free with CSV_reader_free()
/// @warning Do close the input file until finished reading from it
/// @ingroup c_reader
CSV_reader * CSV_reader_init_from_file(FILE * file);

/// Create a new CSV_reader object parsing from an in-memory string

/// @param input String to read from. Caller remains responsible to free the string
/// after use
/// @returns New CSV_reader object. Free with CSV_reader_free()
/// @warning Do not free the input string until finished reading from it
/// @ingroup c_reader
CSV_reader * CSV_reader_init_from_str(const char * input);

/// Free a CSV_reader object

/// Closes the file if created with CSV_reader_init_from_filename
/// @ingroup c_reader
void CSV_reader_free(CSV_reader * reader);

/// Change delimiter character

/// @param delimiter New delimiter character
/// @ingroup c_reader
void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter);

/// Change the quote character

/// @param quote New quote character
/// @ingroup c_reader
void CSV_reader_set_quote(CSV_reader * reader, const char quote);

/// Enable / disable lenient parsing

/// Lenient parsing will attempt to ignore syntax errors in CSV input.
/// @ingroup c_reader
void CSV_reader_set_lenient(CSV_reader * reader, const bool lenient);

/// Read a single field.

/// Check CSV_reader_end_of_row() to see if this is the last field in the current row
/// @returns The next field from the row. Caller should free this with `free()`
/// @returns NULL if past the end of the input or an error occurred.
/// Check CSV_reader_get_error() to distinguish
/// @ingroup c_reader
char * CSV_reader_read_field(CSV_reader * reader);

/// Reads fields into variadic arguments

/// @warning This may be used to fields from multiple rows at a time.
/// Use with caution if the number of fields per row is not known
/// beforehand.
/// @param[out] ... \c char** variables to read into. Last argument must be NULL
/// If more parameters are passed than there are fields remaining,
/// the remaining parameters will be set to NULL
/// @ingroup c_reader
CSV_status CSV_reader_read_v(CSV_reader * reader, ...);

/// Read the current row from the CSV file and advance to the next

/// @returns CSV_row array containing fields for this row. Free with CSV_row_free().
/// @returns NULL if past the end of the input or an error occurred.
/// Check CSV_reader_get_error() to distinguish
/// @ingroup c_reader
CSV_row * CSV_reader_read_row(CSV_reader * reader);

/// Read a row into an array of strings.

/// If \c fields is null, this will allocate a new array (and pass ownership to the caller),
/// and return the final size into \c num_fields.
///
/// Otherwise, it overwrites \c fields contents with strings (owned by caller)
/// up to the limit specified in num_fields, discarding any fields remaining
/// until the end of the row
/// @param[in,out] fields Pointer to string array to write fields into. Caller
///                       must free all strings returned in the array.\n
///                       If NULL is passed, a new array will be allocated and
///                       returned by this call. Caller should free this array
///                       as well as the strings within
/// @param[in,out] num_fields Pointer to size of \c fields.\n
///                           If NULL is passed to \c fields, the size of the
///                           new array returned into \c fields will be returned
///                           into num_fields
/// @returns #CSV_OK on successful read
/// @returns #CSV_TOO_MANY_FIELDS_WARNING when fields is not null and there are more fields than num_fields
/// @returns #CSV_EOF if no rows remain to be read
/// @returns Other #CSV_status error code if an error occurred when reading
/// @ingroup c_reader
CSV_status CSV_reader_read_row_ptr(CSV_reader * reader, char *** fields, size_t * num_fields);

/// Check for end of input

/// @returns \c true if no data remains to be read
/// @ingroup c_reader
bool CSV_reader_eof(const CSV_reader * reader);

/// Check for end of current row

/// @returns \c true if no fields remain in the current row
/// @ingroup c_reader
bool CSV_reader_end_of_row(const CSV_reader * reader);

/// Get message for last error.

/// @returns a string with details about the last error
/// @returns NULL if no error has occurred
/// @ingroup c_reader
const char * CSV_reader_get_error_msg(const CSV_reader * reader);

/// Get error code for last error

/// @returns #CSV_status for last error that occurred
/// @ingroup c_reader
CSV_status CSV_reader_get_error(const CSV_reader * reader);

/// @defgroup c_writer CSV_writer
/// @ingroup c
/// @brief CSV Writer
/// @details Writes data in CSV format, with correct escaping as needed, according to RFC 4180 rules.
/// Allows writing by rows or field-by-field. Mixing these is not
/// recommended, but is possible. Row-wise methods will append to the row
/// started by any field-wise methods.

/// @ingroup c_writer
typedef struct CSV_writer CSV_writer;

/// Create a new CSV_writer object writing to a file

/// @param filename Path to write to. If the file already exists,
///                 it will be overwritten
/// @returns New CSV_writer. Free with CSV_writer_free()
/// @returns NULL if unable to open the file. Use strerror / perror for details
/// @ingroup c_writer
CSV_writer * CSV_writer_init_from_filename(const char * filename);

/// Create a new CSV_writer object writing to a FILE *

/// @param file FILE * opened in write mode
/// @returns New CSV_writer. Free with CSV_writer_free()
/// @ingroup c_writer
CSV_writer * CSV_writer_init_from_file(FILE * file);

/// Create a new CSV_writer object writing to a string

/// When finished writing, retrieve the string with CSV_writer_get_str()
/// @returns New CSV_writer. Free with CSV_writer_free()
/// @ingroup c_writer
CSV_writer * CSV_writer_init_to_str(void);

/// Free a CSV_writer object

/// Closes the file if created with CSV_writer_init_from_filename
/// @ingroup c_writer
void CSV_writer_free(CSV_writer * writer);

/// Change delimiter character

/// @param delimiter New delimiter character
/// @ingroup c_writer
void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter);

/// Change the quote character

/// @param quote New quote character
/// @ingroup c_writer
void CSV_writer_set_quote(CSV_writer * writer, const char quote);

/// End the current row

/// @ingroup c_writer
/// @returns #CSV_OK if successful
/// @returns #CSV_IO_ERROR if an error occurs while writing
CSV_status CSV_writer_end_row(CSV_writer * writer);

/// Write a single field

/// Use CSV_writer_end_row() to move to the next row
/// @param field data to write
/// @returns #CSV_OK if successful
/// @returns #CSV_IO_ERROR if an error occurs while writing
/// @ingroup c_writer
CSV_status CSV_writer_write_field(CSV_writer * writer, const char * field);

/// Write a CSV_row

/// @param fields CSV_row to write
/// @ingroup c_writer
CSV_status CSV_writer_write_row(CSV_writer * writer, const CSV_row * fields);

/// Write an array of strings as a row

/// @param fields Array of strings to write
/// @param num_fields Size of \c fields array
/// @returns #CSV_OK if successful
/// @returns #CSV_IO_ERROR if an error occurs while writing
/// @ingroup c_writer
CSV_status CSV_writer_write_row_ptr(CSV_writer * writer, const char *const * fields, const size_t num_fields);

/// Write a row from the given variadic parameters

/// @param ... `const char *` fields to write. Last parameter must be NULL
/// @returns #CSV_OK if successful
/// @returns #CSV_IO_ERROR if an error occurs while writing
/// @ingroup c_writer
CSV_status CSV_writer_write_row_v(CSV_writer * writer, ...);

/// Get the string output

/// Only valid when initialized w/ CSV_writer_init_to_str()
/// @returns CSV data as a string. Do not free.
/// @returns NULL if not initialized with CSV_writer_init_to_str()
/// @ingroup c_writer
const char * CSV_writer_get_str(CSV_writer * writer);

#ifdef __cplusplus
}
#endif

#endif // CSV_H
