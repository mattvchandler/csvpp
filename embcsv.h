#ifndef EMBCSV_H
#define EMBCSV_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef EMBCSV_FIELD_BUF_SIZE
#define EMBCSV_FIELD_BUF_SIZE  16
#endif

struct EMBCSV_reader
{
    #ifndef EMBCSV_NO_MALLOC
    char * field;
    size_t field_alloc;
    #else
    char field[EMBCSV_FIELD_BUF_SIZE];
    #endif
    size_t field_size;

    char delimiter;
    char quote;

    bool quoted;
    enum {EMBCSV_STATE_READY, EMBCSV_STATE_DOUBLE_QUOTE, EMBCSV_STATE_CONSUME_NEWLINES} state;
};
typedef struct EMBCSV_reader EMBCSV_reader;

typedef enum {EMBCSV_INCOMPLETE, EMBCSV_FIELD, EMBCSV_END_OF_ROW, EMBCSV_PARSE_ERROR} EMBCSV_result;

#ifndef EMBCSV_NO_MALLOC
EMBCSV_reader * EMBCSV_reader_init();
EMBCSV_reader * EMBCSV_reader_init_full(char delimiter, char quote);
void EMBCSV_reader_free(EMBCSV_reader * r);
#else
void EMBCSV_reader_init(EMBCSV_reader *r);
void EMBCSV_reader_init_full(EMBCSV_reader *r, char delimiter, char quote);
#endif
EMBCSV_result EMBCSV_reader_parse_char(EMBCSV_reader * r, int c, char ** field_out);

#ifdef __cplusplus
}
#endif

#endif // EMBCSV_H
