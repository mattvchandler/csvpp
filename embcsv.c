#include "embcsv.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct EMBCSV_reader
{
    char * field;
    size_t field_size;
    size_t field_alloc;

    bool quoted;
    enum {EMBCSV_STATE_READY, EMBCSV_STATE_DOUBLE_QUOTE, EMBCSV_STATE_CONSUME_NEWLINES} state;
};

EMBCSV_reader * EMBCSV_reader_init()
{
    EMBCSV_reader * r = malloc(sizeof(EMBCSV_reader));

    r->field_alloc = EMBCSV_FIELD_BUF_SIZE;
    r->field = malloc(r->field_alloc);
    r->field_size = 0;

    r->quoted = false;
    r->state = EMBCSV_STATE_READY;

    return r;
}

void EMBCSV_reader_free(EMBCSV_reader * r)
{
    free(r->field);
    free(r);
}

void EMBCSV_reader_pushc(EMBCSV_reader * r, char c)
{
    if(r->field_size == r->field_alloc)
    {
        r->field_alloc += EMBCSV_FIELD_BUF_SIZE;
        r->field = realloc(r->field, r->field_alloc);
    }
    r->field[r->field_size++] = c;
}

EMBCSV_result EMBCSV_reader_parse_char(EMBCSV_reader * r, int c, char ** field_out)
{
    while(true)
    {
        switch(r->state)
        {
        case EMBCSV_STATE_CONSUME_NEWLINES:

            if(c != EOF && c != '\0' && c != '\r' && c != '\n')
            {
                r->state = EMBCSV_STATE_READY;
                break;
            }
            else
                return EMBCSV_INCOMPLETE;

        case EMBCSV_STATE_DOUBLE_QUOTE:

            if(c == EMBCSV_DELIMITER || c == '\n' || c == '\r' || c == EOF || c == '\0')
            {
                r->quoted = false;
                r->state = EMBCSV_STATE_READY;
                break;
            }
            else if(c == EMBCSV_QUOTE)
            {
                EMBCSV_reader_pushc(r, c);
                r->state = EMBCSV_STATE_READY;
                return EMBCSV_INCOMPLETE;
            }
            else
                return EMBCSV_PARSE_ERROR;

        case EMBCSV_STATE_READY:

            if(c == EMBCSV_QUOTE)
            {
                if(r->quoted)
                {
                    r->state = EMBCSV_STATE_DOUBLE_QUOTE;
                    return EMBCSV_INCOMPLETE;
                }
                else
                {
                    if(r->field_size == 0)
                    {
                        r->quoted = true;
                        return EMBCSV_INCOMPLETE;
                    }
                    else
                        return EMBCSV_PARSE_ERROR;
                }
            }

            if((c == EOF || c == '\0') && r->quoted)
                return EMBCSV_PARSE_ERROR;

            if(c == EMBCSV_DELIMITER && !r->quoted)
            {
                EMBCSV_reader_pushc(r, '\0');
                *field_out = r->field;
                r->field_size = 0;
                return EMBCSV_FIELD;
            }

            if((c == '\n' || c == '\r' || c == EOF || c == '\0') && !r->quoted)
            {
                r->state = EMBCSV_STATE_CONSUME_NEWLINES;

                EMBCSV_reader_pushc(r, '\0');
                *field_out = r->field;
                r->field_size = 0;
                return EMBCSV_END_OF_ROW;
            }

            EMBCSV_reader_pushc(r, c);
            return EMBCSV_INCOMPLETE;

        }
    }
    // we should never reach this state
    return EMBCSV_PARSE_ERROR;
}
