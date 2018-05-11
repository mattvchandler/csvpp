// csv.h
// (C) 2014 Matthew Chandler
// Licensed under the MIT license
// Routines for reading from and writing to a .csv file

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

// read the current record from the CSV file and advance to the next
// fields_out is owned by CSV_reader and must not be freed or modified by the caller
CSV_status CSV_reader_read_record(CSV_reader * reader, char *** fields_out, size_t * num_fields_out);

// create a new CSV writer object
CSV_writer * CSV_writer_init_from_filename(const char * filename);

// delete a CSV writer object
void CSV_writer_free(CSV_writer * writer);

// write a record
CSV_status CSV_writer_write_record(CSV_writer * writer, const char *const * fields, const size_t num_fields);

#ifdef __cplusplus
}
#endif

#endif // CSV_H
