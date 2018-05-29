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

typedef struct _CSV_reader_private
{
    FILE * _file;
    unsigned int _line_no;
    unsigned int _col_no;
    char ** _fields;
    size_t _fields_alloc;
    size_t _num_fields;
    CSV_status _status;
} CSV_reader_private;

// CSV writer object
typedef struct _CSV_writer_private
{
    FILE * _file;
} CSV_writer_private;

char * CSV_strdup(const char * src)
{
    char * ret = (char *)malloc(sizeof(char) * (strlen(src) + 1));
    strcpy(ret, src);
    return ret;
}

// create a new CSV reader object
CSV_reader * CSV_reader_init_from_filename(const char * filename)
{
    CSV_reader_private * reader = (CSV_reader_private *)malloc(sizeof(CSV_reader_private));
    reader->_file = fopen(filename, "rb");

    if(!reader->_file)
    {
        free(reader);
        return NULL;
    }

    reader->_line_no = 1;
    reader->_col_no = 0;
    reader->_num_fields = 0;
    reader->_status = CSV_OK;

    // init field storage
    reader->_fields_alloc = 10;
    reader->_fields = (char **)malloc(sizeof(char **) * reader->_fields_alloc);
    memset(reader->_fields, 0, sizeof(char **) * reader->_fields_alloc);

    return (CSV_reader *)reader;
}

// delete a CSV reader object
void CSV_reader_free(CSV_reader * reader)
{
    CSV_reader_private * reader_p = (CSV_reader_private *)reader;
    fclose(reader_p->_file);

    if(reader_p->_fields)
    {
        for(size_t i = 0; i < reader_p->_num_fields; ++i)
            free(reader_p->_fields[i]);

        free(reader_p->_fields);
    }

    free(reader_p);
}

typedef enum {PARSE_CSV_FIELD, PARSE_CSV_RECORD, PARSE_CSV_EOF, PARSE_CSV_IO_ERROR, PARSE_CSV_PARSE_ERROR} Parse_csv_ret_t;
char * parse_csv(CSV_reader_private * reader, Parse_csv_ret_t * ret_type, int * empty_row)
{
    if(feof(reader->_file))
    {
        reader->_status = CSV_EOF;
        *ret_type = PARSE_CSV_EOF;
        return NULL;
    }

    size_t field_alloc = 32;
    char * field = (char *)malloc(sizeof(char) * field_alloc);
    char * field_ptr = field;

    *field_ptr = '\0';

    int quoted = 0;

    while(1)
    {
        int c = fgetc(reader->_file);
        ++reader->_col_no;

        if(ferror(reader->_file) && !feof(reader->_file))
        {
            reader->_status = CSV_EOF;
            *ret_type = PARSE_CSV_IO_ERROR;
            break;
        }

        if(!quoted && strlen(field) == 0 && c == '"')
        {
            *empty_row = 0;
            quoted = 1;
        }
        else if(!quoted && c == ',')
        {
            *empty_row = 0;
            *ret_type = PARSE_CSV_FIELD;
            break;
        }
        else if(!quoted && (c == '\n' || c == '\r' || c == EOF))
        {
            if(c == '\r')
            {
                int next_c = fgetc(reader->_file);
                if(next_c != '\n')
                    ungetc(next_c, reader->_file);
                else
                    c = '\n';
            }
            if(c == '\n')
            {
                ++reader->_line_no;
                reader->_col_no = 0;

                int next_c = fgetc(reader->_file);
                if(next_c != EOF)
                    ungetc(next_c, reader->_file);
                else
                    c = EOF;
            }

            if(*empty_row && c == EOF)
            {
                reader->_status = CSV_EOF;
                *ret_type = PARSE_CSV_EOF;
            }
            else
            {
                *ret_type = PARSE_CSV_RECORD;
            }
            break;
        }
        else if(!quoted && c == '"')
        {
            *ret_type = PARSE_CSV_PARSE_ERROR;
            break;
        }
        else if(quoted && c == '"')
        {
            int next_c = fgetc(reader->_file);
            if(next_c == '"')
            {
                if(strlen(field) == field_alloc - 1)
                {
                    field_alloc *= 2;
                    field = (char *)realloc(field, sizeof(char) * field_alloc);
                    field_ptr = field + strlen(field);
                }
                *field_ptr = '"';
                *(++field_ptr) = '\0';
                ++reader->_col_no;
            }
            else if(next_c == ',' || next_c == '\n' || next_c == '\r' || next_c == EOF)
            {
                quoted = 0;
                ungetc(next_c, reader->_file);
            }
            else
            {
                reader->_status = CSV_PARSE_ERROR;
                *ret_type = PARSE_CSV_PARSE_ERROR;
                break;
            }
        }
        else if(quoted && c == EOF)
        {
            reader->_status = CSV_PARSE_ERROR;
            *ret_type = PARSE_CSV_PARSE_ERROR;
            break;
        }
        else
        {
            if(c == '\n')
            {
                ++reader->_line_no;
                reader->_col_no = 0;
            }

            *empty_row = 0;
            if(strlen(field) == field_alloc - 1)
            {
                field_alloc *= 2;
                field = (char *)realloc(field, sizeof(char) * field_alloc);
                field_ptr = field + strlen(field);
            }
            *field_ptr = c;
            *(++field_ptr) = '\0';
        }
    }

    if(*ret_type == PARSE_CSV_IO_ERROR || *ret_type == PARSE_CSV_PARSE_ERROR || *ret_type == PARSE_CSV_EOF)
    {
        switch(*ret_type)
        {
        case PARSE_CSV_IO_ERROR:
            perror("Error reading from csv file");
            reader->_status = CSV_IO_ERROR;
            break;
        case PARSE_CSV_PARSE_ERROR:
            fprintf(stderr, "Error parsing CSV file at line: %u, col: %u\n", reader->_line_no, reader->_col_no);
            reader->_status = CSV_PARSE_ERROR;
            break;
        default:
            break;
        }
        free(field);
        return NULL;
    }

    if(*empty_row)
    {
        free(field);
        return NULL;
    }

    return field;
}

// read the current record from the CSV file and advance to the next
// fields_out is owned by CSV_reader and must not be freed or modified by the caller
CSV_status CSV_reader_read_record(CSV_reader * reader, char *** fields_out, size_t * num_fields_out)
{
    CSV_reader_private * reader_p = (CSV_reader_private *)reader;

    *fields_out = NULL;
    *num_fields_out = 0;

    if(!reader_p)
    {
        return CSV_INTERNAL_ERROR;
    }
    else if(reader_p->_status != CSV_OK)
    {
        return reader_p->_status;
    }

    for(size_t i = 0; i < reader_p->_num_fields; ++i)
    {
        if(reader_p->_fields[i])
            free(reader_p->_fields[i]);
        reader_p->_fields[i] = NULL;
    }

    reader_p->_num_fields = 0;

    int empty_row = 1;
    while(1)
    {
        Parse_csv_ret_t ret_type;
        char * token = parse_csv(reader_p, &ret_type, &empty_row);

        if(ret_type != PARSE_CSV_FIELD && ret_type != PARSE_CSV_RECORD)
            break;

        if(empty_row)
            break;

        // expand field storage as needed
        if(++reader_p->_num_fields > reader_p->_fields_alloc)
        {
            reader_p->_fields_alloc *= 2;
            reader_p->_fields = (char **)realloc(reader_p->_fields, sizeof(char **) * reader_p->_fields_alloc);
        }

        reader_p->_fields[reader_p->_num_fields - 1] = token; // token is now owned by reader obj

        if(ret_type == PARSE_CSV_RECORD)
            break;
    }

    if(reader_p->_status == CSV_OK)
    {
        if(empty_row)
            return CSV_EMPTY_ROW;

        *fields_out = reader_p->_fields;
        *num_fields_out = reader_p->_num_fields;

        if(feof(reader_p->_file))
        {
            reader_p->_status = CSV_EOF;
        }
        else
        {
            // peek at the next char
            int next_c = getc(reader_p->_file);
            fseek(reader_p->_file, -1, SEEK_CUR);

            if(next_c == EOF)
                reader_p->_status = CSV_EOF;
        }

        return CSV_OK;
    }

    return reader_p->_status;
}

// create a new CSV writer object
CSV_writer * CSV_writer_init_from_filename(const char * filename)
{
    CSV_writer_private * writer = (CSV_writer_private *)malloc(sizeof(CSV_writer_private));
    writer->_file = fopen(filename, "wb");

    if(!writer->_file)
    {
        free(writer);
        return NULL;
    }

    return (CSV_writer *)writer;
}

// delete a CSV writer object
void CSV_writer_free(CSV_writer * writer)
{
    CSV_writer_private * writer_p = (CSV_writer_private *)writer;
    fclose(writer_p->_file);
    free(writer_p);
}

// write a single record
CSV_status CSV_writer_write_record(CSV_writer * writer, char const *const * fields, const size_t num_fields)
{
    CSV_writer_private * writer_p = (CSV_writer_private *)writer;

    for(size_t field_i = 0; field_i < num_fields; ++field_i)
    {
        const char * field = fields[field_i];

        if(field)
        {
            int quoted = 0;
            const char * i = NULL;
            for(i = field; *i; ++i)
            {
                if(*i == '"'  || *i == ',' || *i == '\n' || *i == '\r')
                {
                    quoted = 1;
                    break;
                }
            }

            if(quoted)
            {
                fputc('"', writer_p->_file);
                if(ferror(writer_p->_file))
                    return CSV_IO_ERROR;
            }

            for(i = field; *i; ++i)
            {
                fputc(*i, writer_p->_file);
                if(ferror(writer_p->_file))
                    return CSV_IO_ERROR;

                if(*i == '"')
                {
                    fputc('"', writer_p->_file);
                    if(ferror(writer_p->_file))
                        return CSV_IO_ERROR;
                }
            }

            if(quoted)
            {
                fputc('"', writer_p->_file);
                if(ferror(writer_p->_file))
                    return CSV_IO_ERROR;
            }
        }

        if(field_i < num_fields - 1)
        {
            fputc(',', writer_p->_file);
            if(ferror(writer_p->_file))
                return CSV_IO_ERROR;
        }
    }

    fputs("\r\n", writer_p->_file);
    if(ferror(writer_p->_file))
        return CSV_IO_ERROR;

    return CSV_OK;
}
