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

#include <stdlib.h>
#include <string.h>

#include "csv.h"

/// @cond INTERNAL

char * CSV_strdup(const char * src)
{
    char * ret = (char *)malloc(sizeof(char) * (strlen(src) + 1));
    strcpy(ret, src);
    return ret;
}

/// @defgroup c_str CSV_string
/// @ingroup c
/// @brief Dynamic string
/// @details Dynamically reallocating string

/// String object

/// @ingroup c_str
struct CSV_string
{
    char * str;   ///< String storage. May not be null-terminated. Use CSV_string_null_terminate() if needed
    size_t size;  ///< Size of string
    size_t alloc; ///< Allocated size of string
};

/// @ingroup c_str
typedef struct CSV_string CSV_string;

/// @brief Initial size of dynamic string allocation
/// @ingroup c_str
enum {CSV_STR_ALLOC = 32};

/// @brief Initial size of CSV_row allocation
/// @ingroup c_str
enum {CSV_RECORD_ALLOC = 8};

/// Free a CSV_string object

/// @returns New CSV_string. Free with CSV_string_free
/// @ingroup c_str
static CSV_string * CSV_string_init(void)
{
    CSV_string * str = (CSV_string *) malloc(sizeof(CSV_string));

    str->alloc = CSV_STR_ALLOC;
    str->size = 0;
    str->str = (char *)malloc(sizeof(char) * str->alloc);

    return str;
}

/// Free a CSV_string

/// @ingroup c_str
static void CSV_string_free(CSV_string * str)
{
    if(!str)
        return;

    free(str->str);
    free(str);
}

/// Null-terminate the string

/// Ensures that `str->str` is null terminated
/// @returns Null-terminated string.
///          This is owned by the CSV_string, so do not free
/// @ingroup c_str
static const char * CSV_string_null_terminate(CSV_string * str)
{
    if(!str)
        return NULL;

    if(str->size == str->alloc)
    {
        str->alloc += CSV_STR_ALLOC;
        str->str = (char *)realloc(str->str, sizeof(char) * str->alloc);
    }

    str->str[str->size] = '\0';

    return str->str;
}

/// Take ownership of the internal string

/// Extract the internal string data (null-terminated) and free the CSV_string object
/// @returns Null-terminated string. Caller must free with \c free
/// @ingroup c_str
static char * CSV_string_steal(CSV_string * str)
{
    if(!str)
        return NULL;

    CSV_string_null_terminate(str);
    char * ret = str->str;

    free(str);
    return ret;
}

/// Append a char to the string

/// @param c Character to append
/// @ingroup c_str
static void CSV_string_append(CSV_string * str, const char c)
{
    if(!str)
        return;

    if(str->size == str->alloc)
    {
        str->alloc += CSV_STR_ALLOC;
        str->str = (char *)realloc(str->str, sizeof(char) * str->alloc);
    }

    str->str[str->size++] = c;
}

/// @brief CSV row
/// @ingroup c_row
struct CSV_row
{
    char ** fields_; ///< Array of string
    size_t size_;    ///< Size of array
    size_t alloc_;   ///< Allocated size of array
};

CSV_row * CSV_row_init(void)
{
    CSV_row * rec = (CSV_row *)malloc(sizeof(CSV_row));

    rec->alloc_ = CSV_RECORD_ALLOC;
    rec->size_ = 0;
    rec->fields_ = (char **)malloc(sizeof(char *) * rec->alloc_);

    return rec;
}

void CSV_row_free(CSV_row * rec)
{
    if(rec)
    {
        for(size_t i = 0; i < rec->size_; ++i)
            free(rec->fields_[i]);

        free(rec->fields_);
        free(rec);
    }
}

void CSV_row_append(CSV_row * rec, char * field)
{
    if(!rec)
        return;

    if(rec->size_ == rec->alloc_)
    {
        rec->alloc_ += CSV_RECORD_ALLOC;
        rec->fields_ = realloc(rec->fields_, sizeof(char *) * rec->alloc_);
    }

    rec->fields_[rec->size_++] = field;
}

size_t CSV_row_size(const CSV_row * rec)
{
    if(!rec)
        return 0;

    return rec->size_;
}

const char * CSV_row_get(const CSV_row * rec, size_t i)
{
    if(!rec || i >= rec->size_)
        return NULL;

    return rec->fields_[i];
}

const char * const * CSV_row_arr(const CSV_row * rec)
{
    return (const char * const *)rec->fields_;
}

/// @brief CSV Reader
/// @ingroup c_reader
struct CSV_reader
{
    union
    {
        FILE * file_;      ///< Contains input file if initialized by CSV_reader_init_from_file() or CSV_reader_init_from_filename()
        const char * str_; ///< Contains input string if initialized by CSV_reader_init_from_str()
    };

    /// Records how this CSV_reader was initialized
    enum {
        CSV_SOURCE_FILENAME, ///< Initialized by CSV_reader_init_from_filename()
        CSV_SOURCE_FILE,     ///< Initialized by CSV_reader_init_from_file()
        CSV_SOURCE_STR       ///< Initialized by CSV_reader_init_from_str()
    } source_;

    /// Parsing states
    enum {
        CSV_STATE_READ,             ///< Ready to read a character into current field
        CSV_STATE_QUOTE,            ///< Checking for escaped quote character or end of quoted field
        CSV_STATE_CONSUME_NEWLINES, ///< Discarding any newline characters
        CSV_STATE_EOF               ///< At end of input
    } state_;

    char delimiter_;       ///< Delimiter character (default ',')
    char quote_;           ///< Quote character (default '"')
    bool lenient_;         ///< Lenient parsing enabled / disabled (default \c false)
    bool end_of_row_;      ///< \c true if parsing is at the end of a row
    unsigned int line_no_; ///< Current line number within input
    unsigned int col_no_;  ///< Current column number within input

    CSV_status error_;     ///< Current error state
    char * error_message_; ///< Error details. NULL if no error has occurred
};

/// @name Private Functions
/// @{

/// Common CSV_reader initialization details

/// Initializes state and default settings
/// @returns CSV_reader without an input source set
/// @ingroup c_reader
static CSV_reader * CSV_reader_init_common(void)
{
    CSV_reader * reader = (CSV_reader *)malloc(sizeof(CSV_reader));

    reader->state_ = CSV_STATE_CONSUME_NEWLINES;
    reader->end_of_row_ = false;

    reader->line_no_ = 1;
    reader->col_no_ = 0;

    reader->delimiter_ = ',';
    reader->quote_ = '"';

    reader->lenient_ = false;

    reader->error_ = CSV_OK;
    reader->error_message_ = NULL;

    return reader;
}

/// Set the parser status

/// Record an status and message
/// @param error New status
/// @param msg Message associated with the status, or NULL to clear stored message
/// @param append_line_and_col Append the line and column number to the message
/// @ingroup c_reader
static void CSV_reader_set_status(CSV_reader * reader, CSV_status error, const char * msg, bool append_line_and_col)
{
    if(!reader)
        return;

    reader->error_ = error;

    free(reader->error_message_);
    if(!msg)
    {
        reader->error_message_ = NULL;
    }
    else
    {
        if(append_line_and_col)
        {
            const char * format =  "%s at line: %u, col: %u";
            int err_msg_size = snprintf(NULL, 0, format, msg, reader->line_no_, reader->col_no_);
            reader->error_message_ = (char *)malloc(sizeof(char) * (err_msg_size + 1));
            sprintf(reader->error_message_, format, msg, reader->line_no_, reader->col_no_);
        }
        else
        {
            reader->error_message_ = CSV_strdup(msg);
        }
    }
}

/// Get next character from input

/// Updates line and column position, and checks for IO error
/// @returns Next character
/// @ingroup c_reader
static int CSV_reader_getc(CSV_reader * reader)
{
    if(!reader)
        return '\0';

    int c = '\0';

    switch(reader->source_)
    {
    case CSV_SOURCE_FILENAME:
    case CSV_SOURCE_FILE:
        c = fgetc(reader->file_);
        break;
    case CSV_SOURCE_STR:
        if(*reader->str_)
            c = *(reader->str_++);
        break;
    }

    if((reader->source_ == CSV_SOURCE_FILENAME || reader->source_ == CSV_SOURCE_FILE) && c == EOF  && !feof(reader->file_))
        CSV_reader_set_status(reader, CSV_IO_ERROR, "I/O Error", false);

    else if(c == '\n')
    {
        ++reader->line_no_;
        reader->col_no_ = 0;
    }
    else if(c != EOF && c != '\0')
        ++reader->col_no_;

    return c;
}

/// Consume newline characters

/// Advance position until first non-newline character
/// @ingroup c_reader
static void CSV_reader_consume_newlines(CSV_reader * reader)
{
    if(!reader || reader->state_ != CSV_STATE_CONSUME_NEWLINES)
        return;

    while(true)
    {
        int c = CSV_reader_getc(reader);
        if(reader->error_ == CSV_IO_ERROR)
            return;

        if(c == EOF || c == '\0')
        {
            reader->end_of_row_ = true;
            reader->state_ = CSV_STATE_EOF;
            CSV_reader_set_status(reader, CSV_EOF, "End of file", false);
            break;
        }
        else if(c != '\r' && c != '\n')
        {
            reader->state_ = CSV_STATE_READ;
            switch(reader->source_)
            {
            case CSV_SOURCE_FILENAME:
            case CSV_SOURCE_FILE:
                ungetc(c, reader->file_);
                break;
            case CSV_SOURCE_STR:
                --reader->str_;
                break;
            }
            --reader->col_no_;
            break;
        }
    }
}

/// Core parsing method

/// Reads and parses character stream to obtain next field
/// @returns Next field, or NULL if at EOF or other error occurred
/// @ingroup c_reader
static char * CSV_reader_parse(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    // fail if we've encountered an error
    if(reader->error_ != CSV_OK)
    {
        // check for and clear any warnings
        if(reader->error_ == CSV_TOO_MANY_FIELDS_WARNING)
            CSV_reader_set_status(reader, CSV_OK, NULL, false);
        else
            return NULL;
    }
    CSV_reader_consume_newlines(reader);

    if(reader->error_ != CSV_OK)
        return NULL;

    bool quoted = false;

    CSV_string * field = CSV_string_init();

    bool field_done = false;
    while(!field_done)
    {
        int c = CSV_reader_getc(reader);
        if(reader->error_ == CSV_IO_ERROR)
            goto error;

        bool c_done = false;
        while(!c_done)
        {
            switch(reader->state_)
            {
            case CSV_STATE_QUOTE:
                if(c == reader->delimiter_ || c == '\n' || c == '\r' || c == EOF || c == '\0')
                {
                    quoted = false;
                    reader->state_ = CSV_STATE_READ;
                    break;
                }
                else if(c == reader->quote_)
                {
                    CSV_string_append(field, c);
                    reader->state_ = CSV_STATE_READ;
                    c_done = true;
                    break;
                }
                else if(reader->lenient_)
                {
                    CSV_string_append(field, reader->quote_);
                    CSV_string_append(field, c);
                    reader->state_ = CSV_STATE_READ;
                    c_done =true;
                    break;
                }
                else
                {
                    CSV_reader_set_status(reader, CSV_PARSE_ERROR, "Unescaped quote", true);
                    goto error;
                }

            case CSV_STATE_READ:
                if(c == reader->quote_)
                {
                    if(quoted)
                    {
                        reader->state_ = CSV_STATE_QUOTE;
                        c_done = true;
                        break;
                    }
                    else
                    {
                        if(field->size == 0)
                        {
                            quoted = true;
                            c_done = true;
                            break;
                        }
                        else if(!reader->lenient_)
                        {
                            CSV_reader_set_status(reader, CSV_PARSE_ERROR, "Quote found in unquoted field", true);
                            goto error;
                        }
                    }
                }

                if(quoted && (c == EOF || c == '\0'))
                {
                    if(reader->lenient_)
                    {
                        reader->end_of_row_ = field_done = c_done = true;
                        reader->state_ = CSV_STATE_CONSUME_NEWLINES;
                        break;
                    }
                    else
                    {
                        CSV_reader_set_status(reader, CSV_PARSE_ERROR, "Unterminated quoted field - reached end-of-field", true);
                        goto error;
                    }
                }
                else if(!quoted && c == reader->delimiter_)
                {
                    field_done = c_done = true;
                    break;
                }
                else if(!quoted && (c == '\n' || c == '\r' || c == EOF || c == '\0'))
                {
                    reader->end_of_row_ = field_done = c_done = true;
                    reader->state_ = CSV_STATE_CONSUME_NEWLINES;
                    break;
                }

                CSV_string_append(field, c);
                c_done = true;
                break;

            default:
                CSV_reader_set_status(reader, CSV_INTERNAL_ERROR, "Internal Error - Illegal state reached", false);
                goto error;
            }
        }
    }

    return CSV_string_steal(field);

error:
    CSV_string_free(field);
    return NULL;
}

/// @}

CSV_reader * CSV_reader_init_from_filename(const char * filename)
{
    FILE * file = fopen(filename, "rb");
    if(!file)
        return NULL;

    CSV_reader * reader = CSV_reader_init_common();

    reader->source_ = CSV_SOURCE_FILENAME;
    reader->file_ = file;

    return reader;
}

CSV_reader * CSV_reader_init_from_file(FILE * file)
{
    if(!file)
        return NULL;

    CSV_reader * reader = CSV_reader_init_common();

    reader->source_ = CSV_SOURCE_FILE;
    reader->file_ = file;

    return reader;
}

CSV_reader * CSV_reader_init_from_str(const char * input)
{
    if(!input)
        return NULL;

    CSV_reader * reader = CSV_reader_init_common();

    reader->source_ = CSV_SOURCE_STR;
    reader->str_ = input;

    return reader;
}

void CSV_reader_free(CSV_reader * reader)
{
    if(!reader)
        return;

    if(reader->source_ == CSV_SOURCE_FILENAME)
        fclose(reader->file_);

    if(reader->error_message_)
        free(reader->error_message_);

    free(reader);
}

void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter)
{
    if(!reader)
        return;

    reader->delimiter_ = delimiter;
}

void CSV_reader_set_quote(CSV_reader * reader, const char quote)
{
    if(!reader)
        return;

    reader->quote_ = quote;
}

void CSV_reader_set_lenient(CSV_reader * reader, const bool lenient)
{
    if(!reader)
        return;

    reader->lenient_ = lenient;
}

char * CSV_reader_read_field(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    reader->end_of_row_ = false;
    return CSV_reader_parse(reader);
}

CSV_status CSV_reader_read_v(CSV_reader * reader, ...)
{
    if(!reader)
        return CSV_INTERNAL_ERROR;

    va_list args;
    va_start(args, reader);

    char ** arg = NULL;
    while((arg = va_arg(args, char **)))
    {
        *arg = NULL;
        *arg = CSV_reader_read_field(reader);
    }

    va_end(args);
    return reader->error_;
}

CSV_row * CSV_reader_read_row(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    CSV_row * rec = CSV_row_init();

    while(true)
    {
        char * field = CSV_reader_read_field(reader);
        if(!field)
        {
            CSV_row_free(rec);
            return NULL;
        }

        CSV_row_append(rec, field);

        if(CSV_reader_end_of_row(reader))
            break;
    }

    return rec;
}

CSV_status CSV_reader_read_row_ptr(CSV_reader * reader, char *** fields, size_t * num_fields)
{
    if(!reader)
        return CSV_INTERNAL_ERROR;

    size_t fields_alloc = 0;
    size_t fields_size = 0;
    if(!*fields)
    {
        fields_alloc = CSV_RECORD_ALLOC;
        *fields = (char **)malloc(sizeof(char *) * fields_alloc);
    }

    bool too_many_fields = false;

    while(true)
    {
        char * field = CSV_reader_read_field(reader);
        if(!field)
        {
            for(size_t i = 0; i < fields_size; ++i)
            {
                free((*fields)[i]);
                (*fields)[i] = NULL;
            }

            if(fields_alloc)
            {
                free(*fields);
                *fields = NULL;
            }

            return reader->error_;
        }

        if(fields_alloc)
        {
            if(fields_size == fields_alloc)
            {
                fields_alloc *= 2;
                *fields = (char **)realloc(*fields, sizeof(char *) * fields_alloc);
            }

            (*fields)[fields_size++] = field;
        }
        else
        {
            if(fields_size < *num_fields)
                (*fields)[fields_size++] = field;
            else
            {
                too_many_fields = true;
                free(field);
            }
        }

        if(CSV_reader_end_of_row(reader))
            break;
    }

    *num_fields = fields_size;
    return too_many_fields? CSV_TOO_MANY_FIELDS_WARNING : reader->error_;
}

bool CSV_reader_eof(const CSV_reader * reader)
{
    if(!reader)
        return true;
    return reader->state_ == CSV_STATE_EOF;
}

bool CSV_reader_end_of_row(const CSV_reader * reader)
{
    if(!reader)
        return true;
    return reader->end_of_row_ || reader->state_ == CSV_STATE_EOF;
}

const char * CSV_reader_get_error_msg(const CSV_reader * reader)
{
    if(!reader)
        return NULL;

    return reader->error_message_;
}

CSV_status CSV_reader_get_error(const CSV_reader * reader)
{
    if(!reader)
        return CSV_INTERNAL_ERROR;

    return reader->error_;
}

/// @brief CSV Writer
struct CSV_writer
{
    union
    {
        FILE * file_;      ///< Contains output file if initialized by CSV_writer_init_from_file() or CSV_writer_init_from_filename()
        CSV_string * str_; ///< Contains output string if initialized by CSV_writer_init_to_str()
    };

    /// Records how this CSV_writer was initialized
    enum {
        CSV_DEST_FILENAME, ///< Initialized by CSV_writer_init_from_filename()
        CSV_DEST_FILE,     ///< Initialized by CSV_writer_init_from_file()
        CSV_DEST_STR       ///< Initialized by CSV_writer_init_to_str()
    } dest_;

    char delimiter_;    ///< Delimiter character (default ',')
    char quote_;        ///< Quote character (default '"')
    bool start_of_row_; ///< for keeping track if when a row needs to be ended
};

/// @name Private Functions
/// @{

/// Common CSV_writer initialization details

/// Initializes state and default settings
/// @returns CSV_writer without an output source set
/// @ingroup c_writer
static CSV_writer * CSV_writer_init_common(void)
{
    CSV_writer * writer = (CSV_writer *)malloc(sizeof(CSV_writer));
    writer->delimiter_ = ',';
    writer->quote_ = '"';

    writer->start_of_row_ = true;

    return writer;
}

/// Append character

/// Append a character to output and check for errors
/// @param c character to append
/// @returns #CSV_OK if successful
/// @returns #CSV_IO_ERROR if an error occurs while writing
/// @ingroup c_writer
static CSV_status CSV_writer_putc(CSV_writer * writer, const char c)
{
    switch(writer->dest_)
    {
    case CSV_DEST_FILENAME:
    case CSV_DEST_FILE:
        fputc(c, writer->file_);
        if(ferror(writer->file_))
            return CSV_IO_ERROR;
        break;
    case CSV_DEST_STR:
        CSV_string_append(writer->str_, c);
        break;
    }

    return CSV_OK;
}

/// @}

CSV_writer * CSV_writer_init_from_filename(const char * filename)
{
    FILE * file = fopen(filename, "wb");

    if(!file)
        return NULL;

    CSV_writer * writer = CSV_writer_init_common();

    writer->dest_ = CSV_DEST_FILENAME;
    writer->file_ = file;

    return writer;
}

CSV_writer * CSV_writer_init_from_file(FILE * file)
{
    if(!file)
        return NULL;
    CSV_writer * writer = CSV_writer_init_common();

    writer->dest_ = CSV_DEST_FILE;
    writer->file_ = file;

    return writer;
}

CSV_writer * CSV_writer_init_to_str(void)
{
    CSV_writer * writer = CSV_writer_init_common();
    writer->dest_ = CSV_DEST_STR;
    writer->str_ = CSV_string_init();

    return writer;
}

void CSV_writer_free(CSV_writer * writer)
{
    if(!writer)
        return;

    switch(writer->dest_)
    {
    case CSV_DEST_FILENAME:
        fclose(writer->file_);
        break;
    case CSV_DEST_FILE:
        break;
    case CSV_DEST_STR:
        CSV_string_free(writer->str_);
        break;
    }

    free(writer);
}

void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter)
{
    writer->delimiter_ = delimiter;
}

void CSV_writer_set_quote(CSV_writer * writer, const char quote)
{
    writer->quote_ = quote;
}

CSV_status CSV_writer_end_row(CSV_writer * writer)
{
    if(!writer)
        return CSV_INTERNAL_ERROR;

    CSV_status status = CSV_OK;
    if((status = CSV_writer_putc(writer, '\r')) != CSV_OK)
        return status;
    if((status = CSV_writer_putc(writer, '\n')) != CSV_OK)
        return status;

    writer->start_of_row_ = true;

    return CSV_OK;
}

CSV_status CSV_writer_write_field(CSV_writer * writer, const char * field)
{
    if(!writer)
        return CSV_INTERNAL_ERROR;

    CSV_status status;
    if(!writer->start_of_row_)
    {
        if((status = CSV_writer_putc(writer, writer->delimiter_)) != CSV_OK)
            return status;
    }

    if(field)
    {
        bool quoted = false;
        for(const char * i = field; *i; ++i)
        {
            if(*i == writer->quote_ || *i == writer->delimiter_ || *i == '\n' || *i == '\r')
            {
                quoted = true;
                break;
            }
        }

        if(quoted)
        {
            if((status = CSV_writer_putc(writer, writer->quote_)) != CSV_OK)
                return status;
        }

        for(const char * i = field; *i; ++i)
        {
            if((status = CSV_writer_putc(writer, *i)) != CSV_OK)
                return status;

            if(*i == writer->quote_)
            {
                if((status = CSV_writer_putc(writer, writer->quote_)) != CSV_OK)
                    return status;
            }
        }

        if(quoted)
        {
            if((status = CSV_writer_putc(writer, writer->quote_)) != CSV_OK)
                return status;
        }
    }

    writer->start_of_row_ = false;

    return CSV_OK;
}

CSV_status CSV_writer_write_row(CSV_writer * writer, const CSV_row * fields)
{
    if(!writer)
        return CSV_INTERNAL_ERROR;

    for(size_t i = 0; i < CSV_row_size(fields); ++i)
    {
        CSV_status stat = CSV_writer_write_field(writer, CSV_row_get(fields, i));
        if(stat != CSV_OK)
            return stat;
    }

    return CSV_writer_end_row(writer);
}

CSV_status CSV_writer_write_row_ptr(CSV_writer * writer, char const * const * fields, const size_t num_fields)
{
    if(!writer)
        return CSV_INTERNAL_ERROR;

    for(size_t i = 0; i < num_fields; ++i)
    {
        CSV_status stat = CSV_writer_write_field(writer, fields[i]);
        if(stat != CSV_OK)
            return stat;
    }

    return CSV_writer_end_row(writer);
}

CSV_status CSV_writer_write_row_v(CSV_writer * writer, ...)
{
    if(!writer)
        return CSV_INTERNAL_ERROR;

    va_list args;
    va_start(args, writer);

    const char * arg = NULL;
    while((arg = va_arg(args, const char *)))
    {
        CSV_status stat = CSV_writer_write_field(writer, arg);
        if(stat != CSV_OK)
        {
            va_end(args);
            return stat;
        }
    }

    CSV_writer_end_row(writer);

    va_end(args);
    return CSV_OK;
}

const char * CSV_writer_get_str(CSV_writer * writer)
{
    if(!writer || writer->dest_ != CSV_DEST_STR)
        return NULL;

    return CSV_string_null_terminate(writer->str_);
}

/// @endcond INTERNAL
