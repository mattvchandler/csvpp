#ifndef EMBCSV_H
#define EMBCSV_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef EMBCSV_FIELD_BUF_SIZE
#define EMBCSV_FIELD_BUF_SIZE  16
#endif
#ifndef EMBCSV_QUOTE
#define EMBCSV_QUOTE '"'
#endif
#ifndef EMBCSV_DELIMITER
#define EMBCSV_DELIMITER ','
#endif

typedef struct EMBCSV_reader EMBCSV_reader;
typedef enum {EMBCSV_INCOMPLETE, EMBCSV_FIELD, EMBCSV_END_OF_ROW, EMBCSV_PARSE_ERROR} EMBCSV_result;

EMBCSV_reader * EMBCSV_reader_init();
void EMBCSV_reader_free(EMBCSV_reader * r);
EMBCSV_result EMBCSV_reader_parse_char(EMBCSV_reader * r, int c, char ** field_out);

#ifdef __cplusplus
}
#endif

#endif // EMBCSV_H
