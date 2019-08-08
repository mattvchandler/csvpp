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

#include <stdlib.h>
#include <string.h>

#include "csv.h"

struct CSV_reader
{
    FILE * file_;
    enum {CSV_STATE_READ, CSV_STATE_QUOTE, CSV_STATE_CONSUME_NEWLINES, CSV_STATE_EOF, CSV_STATE__ERROR} state_;
    bool end_of_row_;
    unsigned int line_no_;
    unsigned int col_no_;
    char delimiter_;
    char quote_;

    CSV_error error_;
    char * error_message_;
};

// CSV writer object
struct CSV_writer
{
    FILE * file_;
    char delimiter_;
    char quote_;
};

struct CSV_record
{
    size_t size_;
    size_t alloc_;
    char ** fields_;
};

char * CSV_strdup(const char * src)
{
    char * ret = (char *)malloc(sizeof(char) * (strlen(src) + 1));
    strcpy(ret, src);
    return ret;
}

enum {CSV_RECORD_ALLOC = 8};
CSV_record * CSV_record_init()
{
    CSV_record * rec = (CSV_record *)malloc(sizeof(CSV_record));

    rec->alloc_ = CSV_RECORD_ALLOC;
    rec->size_ = 0;
    rec->fields_ = (char **)malloc(sizeof(char *) * rec->alloc_);

    return rec;
}

void CSV_record_free(CSV_record * rec)
{
    if(rec)
    {
        for(size_t i = 0; i < rec->size_; ++i)
            free(rec->fields_[i]);

        free(rec->fields_);
        free(rec);
    }
}

// take ownership of field and append it to the record
void CSV_record_append(CSV_record * rec, char * field)
{
    if(!rec)
        return;

    if(rec->size_ == rec->alloc_)
    {
        rec->alloc_ *= 2;
        rec->fields_ = realloc(rec->fields_, sizeof(char *) * rec->alloc_);
    }

    rec->fields_[rec->size_++] = field;
}

size_t CSV_record_size(const CSV_record * rec)
{
    if(!rec)
        return 0;

    return rec->size_;
}

const char * CSV_record_get(const CSV_record * rec, size_t i)
{
    if(!rec || i >= rec->size_)
        return NULL;

    return rec->fields_[i];
}
const char * const * CSV_record_arr(const CSV_record * rec)
{
    return (const char * const *)rec->fields_;
}

// create a new CSV reader object
CSV_reader * CSV_reader_init_from_filename(const char * filename)
{
    CSV_reader * reader = (CSV_reader *)malloc(sizeof(CSV_reader));
    reader->file_ = fopen(filename, "rb");

    if(!reader->file_)
    {
        free(reader);
        return NULL;
    }

    reader->state_ = CSV_STATE_CONSUME_NEWLINES;
    reader->end_of_row_ = false;

    reader->line_no_ = 1;
    reader->col_no_ = 0;

    reader->delimiter_ = ',';
    reader->quote_ = '"';

    reader->error_ = CSV_OK;
    reader->error_message_ = NULL;

    return reader;
}

// delete a CSV reader object
void CSV_reader_free(CSV_reader * reader)
{
    fclose(reader->file_);

    if(reader->error_message_)
        free(reader->error_message_);

    free(reader);
}

void CSV_reader_set_error(CSV_reader * reader, CSV_error error, const char * msg, bool append_line_and_col)
{
    if(!reader)
        return;

    reader->error_ = error;

    if(append_line_and_col)
    {
        const char * format =  "%s at line: %d, col: %d";
        int err_msg_size = snprintf(NULL, 0, format, msg, reader->line_no_, reader->col_no_);
        reader->error_message_ = (char *)realloc(reader->error_message_, sizeof(char) * (err_msg_size + 1));
        sprintf(reader->error_message_, format, msg, reader->line_no_, reader->col_no_);
    }
    else
    {
        free(reader->error_message_);
        reader->error_message_ = CSV_strdup(msg);
    }
}

// set delimiter character
void CSV_reader_set_delimiter(CSV_reader * reader, const char delimiter)
{
    reader->delimiter_ = delimiter;
}

// set quote character
void CSV_reader_set_quote(CSV_reader * reader, const char quote)
{
    reader->quote_ = quote;
}

int CSV_getc(CSV_reader * reader)
{
    if(!reader)
        return '\0';

    int c = fgetc(reader->file_);
    if(c == EOF && !feof(reader->file_))
        CSV_reader_set_error(reader, CSV_IO_ERROR, "I/O Error", false);

    else if(c == '\n')
    {
        ++reader->line_no_;
        reader->col_no_ = 0;
    }
    else if(c != EOF)
        ++reader->col_no_;

    return c;
}

void consume_newlines(CSV_reader * reader)
{
    if(!reader || reader->state_ != CSV_STATE_CONSUME_NEWLINES)
        return;

    while(true)
    {
        int c = CSV_getc(reader);
        if(reader->error_ == CSV_IO_ERROR)
            return;

        if(c == EOF)
        {
            reader->end_of_row_ = true;
            reader->state_ = CSV_STATE_EOF;
            break;
        }
        else if(c != '\r' && c != '\n')
        {
            reader->state_ = CSV_STATE_READ;
            ungetc(c, reader->file_);
            --reader->col_no_;
            break;
        }
    }
}

enum {FIELD_ALLOC_SIZE = 32};
char * field_append(char * field, size_t * field_size, size_t * field_alloc, char c)
{
    if(*field_size == *field_alloc)
    {
        *field_alloc *= 2;
        field = realloc(field, *field_alloc);
    }
    field[(*field_size)++] = c;

    return field;
}

char * CSV_reader_parse(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    consume_newlines(reader);
    if(reader->error_ == CSV_IO_ERROR)
        return NULL;

    if(reader->state_ == CSV_STATE_EOF)
        return NULL;

    bool quoted = false;

    size_t field_size = 0;
    size_t field_alloc = FIELD_ALLOC_SIZE;
    char * field = (char *)malloc(sizeof(char) * field_alloc);

    bool field_done = false;
    while(!field_done)
    {
        int c = CSV_getc(reader);
        if(reader->error_ == CSV_IO_ERROR)
            goto error;

        bool c_done = false;
        while(!c_done)
        {
            switch(reader->state_)
            {
            case CSV_STATE_QUOTE:
                if(c == reader->delimiter_ || c == '\n' || c == '\r' || c == EOF)
                {
                    quoted = false;
                    reader->state_ = CSV_STATE_READ;
                    break;
                }
                else if(c == reader->quote_)
                {
                    field = field_append(field, &field_size, &field_alloc, c);
                    reader->state_ = CSV_STATE_READ;
                    c_done = true;
                    break;
                }
                else
                {
                    CSV_reader_set_error(reader, CSV_PARSE_ERROR, "Unescaped quote", true);
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
                        if(field_size == 0)
                        {
                            quoted = true;
                            c_done = true;
                            break;
                        }
                        else
                        {
                            CSV_reader_set_error(reader, CSV_PARSE_ERROR, "Quote found in unquoted field", true);
                            goto error;
                        }
                    }
                }

                if(c == EOF && quoted)
                {
                    CSV_reader_set_error(reader, CSV_PARSE_ERROR, "Unterminated quoted field - reached end-of-field", true);
                    goto error;
                }
                else if(!quoted && c == reader->delimiter_)
                {
                    field_done = c_done = true;
                    break;
                }
                else if(!quoted && (c == '\n' || c == '\r' || c == EOF))
                {
                    reader->end_of_row_ = field_done = c_done = true;
                    reader->state_ = CSV_STATE_CONSUME_NEWLINES;
                    break;
                }

                field = field_append(field, &field_size, &field_alloc, c);
                c_done = true;
                break;

            default:
                CSV_reader_set_error(reader, CSV_INTERNAL_ERROR, "Internal Error - Illegal state reached", false);
                goto error;
            }
        }
    }

    return field_append(field, &field_size, &field_alloc, '\0');

error:
    free(field);
    return NULL;
}

// read a single field. returns NULL on error, otherwise, return is owned by caller
// end_of_row_out will be set true if this is the last field in a row
char * CSV_reader_read_field(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    reader->end_of_row_ = false;
    return CSV_reader_parse(reader);
}

// read the current record from the CSV file and advance to the next
// returns null on error, otherwise caller should free the return value with CSV_record_free
CSV_record * CSV_reader_read_record(CSV_reader * reader)
{
    if(!reader)
        return NULL;

    CSV_record * rec = CSV_record_init();

    while(true)
    {
        char * field = CSV_reader_read_field(reader);
        if(!field)
        {
            CSV_record_free(rec);
            return NULL;
        }

        CSV_record_append(rec, field);

        if(CSV_reader_end_of_row(reader))
            break;
    }

    return rec;
}

// variadic read. pass char **'s followed by null. caller will own all char *'s returned
// returns true for success, false for failure
bool CSV_reader_read_v(CSV_reader * reader, ...)
{
    if(!reader)
        return false;

    va_list args;
    va_start(args, reader);

    char ** arg = NULL;
    while((arg = va_arg(args, char **)))
    {
        *arg = NULL;
        if(CSV_reader_end_of_row(reader))
        {
            CSV_reader_set_error(reader, CSV_PARSE_ERROR, "Attempted to read past end of row", false);
            goto error;
        }

        *arg = CSV_reader_read_field(reader);

        if(!arg)
            goto error;
    }

    if(CSV_reader_end_of_row(reader))
        reader->end_of_row_ = false;

    va_end(args);
    return true;

error:
    va_end(args);
    return false;
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

// get message for last error. returns null when no error has occurred
const char * CSV_reader_get_error_msg(const CSV_reader * reader)
{
    if(!reader)
        return NULL;

    return reader->error_message_;
}

// get error code for last error
CSV_error CSV_reader_get_error(const CSV_reader * reader)
{
    if(!reader)
        return CSV_INTERNAL_ERROR;

    return reader->error_;
}

// create a new CSV writer object
CSV_writer * CSV_writer_init_from_filename(const char * filename)
{
    CSV_writer * writer = (CSV_writer *)malloc(sizeof(CSV_writer));
    writer->file_ = fopen(filename, "wb");

    writer->delimiter_ = ',';
    writer->quote_ = '"';

    if(!writer->file_)
    {
        free(writer);
        return NULL;
    }

    return (CSV_writer *)writer;
}

// delete a CSV writer object
void CSV_writer_free(CSV_writer * writer)
{
    fclose(writer->file_);
    free(writer);
}

// set delimiter character
void CSV_writer_set_delimiter(CSV_writer * writer, const char delimiter)
{
    writer->delimiter_ = delimiter;
}

// set quote character
void CSV_writer_set_quote(CSV_writer * writer, const char quote)
{
    writer->quote_ = quote;
}

// write a single record
CSV_error CSV_writer_write_record(CSV_writer * writer, char const * const * fields, const size_t num_fields)
{
    for(size_t field_i = 0; field_i < num_fields; ++field_i)
    {
        const char * field = fields[field_i];

        if(field)
        {
            bool quoted = false;
            const char * i = NULL;
            for(i = field; *i; ++i)
            {
                if(*i == writer->quote_ || *i == writer->delimiter_ || *i == '\n' || *i == '\r')
                {
                    quoted = true;
                    break;
                }
            }

            if(quoted)
            {
                fputc(writer->quote_, writer->file_);
                if(ferror(writer->file_))
                    return CSV_IO_ERROR;
            }

            for(i = field; *i; ++i)
            {
                fputc(*i, writer->file_);
                if(ferror(writer->file_))
                    return CSV_IO_ERROR;

                if(*i == writer->quote_)
                {
                    fputc(writer->quote_, writer->file_);
                    if(ferror(writer->file_))
                        return CSV_IO_ERROR;
                }
            }

            if(quoted)
            {
                fputc(writer->quote_, writer->file_);
                if(ferror(writer->file_))
                    return CSV_IO_ERROR;
            }
        }

        if(field_i < num_fields - 1)
        {
            fputc(writer->delimiter_, writer->file_);
            if(ferror(writer->file_))
                return CSV_IO_ERROR;
        }
    }

    fputs("\r\n", writer->file_);
    if(ferror(writer->file_))
        return CSV_IO_ERROR;

    return CSV_OK;
}
