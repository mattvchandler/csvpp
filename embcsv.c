#include "embcsv.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef EMBCSV_NO_MALLOC
EMBCSV_reader * EMBCSV_reader_init() { return EMBCSV_reader_init_full(',', '"', false); }
EMBCSV_reader * EMBCSV_reader_init_full(char delimiter, char quote, bool lenient)
#else
void EMBCSV_reader_init(EMBCSV_reader *r) { EMBCSV_reader_init_full(r, ',', '"', false); }
void EMBCSV_reader_init_full(EMBCSV_reader *r, char delimiter, char quote, bool lenient)
#endif
{
    #ifndef EMBCSV_NO_MALLOC
    EMBCSV_reader * r = malloc(sizeof(EMBCSV_reader));

    r->field_alloc = EMBCSV_FIELD_BUF_SIZE;
    r->field = malloc(r->field_alloc);
    #endif

    r->field_size = 0;

    r->quoted = false;
    r->state = EMBCSV_STATE_CONSUME_NEWLINES;

    r->delimiter = delimiter;
    r->quote = quote;
    r->lenient = lenient;

    #ifndef EMBCSV_NO_MALLOC
    return r;
    #endif
}

#ifndef EMBCSV_NO_MALLOC
void EMBCSV_reader_free(EMBCSV_reader * r)
{
    free(r->field);
    free(r);
}
#endif

void EMBCSV_reader_pushc(EMBCSV_reader * r, char c)
{
    #ifndef EMBCSV_NO_MALLOC
    if(r->field_size == r->field_alloc)
    {
        r->field_alloc += EMBCSV_FIELD_BUF_SIZE;
        r->field = realloc(r->field, r->field_alloc);
    }
    #else
    if(r->field_size == EMBCSV_FIELD_BUF_SIZE)
    {
        r->field[EMBCSV_FIELD_BUF_SIZE - 1] = '\0';
        return;
    }
    #endif
    r->field[r->field_size++] = c;
}

EMBCSV_result EMBCSV_reader_parse_char(EMBCSV_reader * r, int c, char ** field_out)
{
    *field_out = NULL;

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

            if(c == r->delimiter || c == '\n' || c == '\r' || c == EOF || c == '\0')
            {
                r->quoted = false;
                r->state = EMBCSV_STATE_READY;
                break;
            }
            else if(c == r->quote)
            {
                EMBCSV_reader_pushc(r, c);
                r->state = EMBCSV_STATE_READY;
                return EMBCSV_INCOMPLETE;
            }
            else if(r->lenient)
            {
                EMBCSV_reader_pushc(r, r->quote);
                EMBCSV_reader_pushc(r, c);
                r->state = EMBCSV_STATE_READY;
                return EMBCSV_INCOMPLETE;
            }
            else
                return EMBCSV_PARSE_ERROR;

        case EMBCSV_STATE_READY:

            if(c == r->quote)
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
                    else if(!r->lenient)
                        return EMBCSV_PARSE_ERROR;
                }
            }

            if(r->quoted && (c == EOF || c == '\0'))
            {
                if(r->lenient)
                {
                    r->state = EMBCSV_STATE_CONSUME_NEWLINES;

                    EMBCSV_reader_pushc(r, '\0');
                    *field_out = r->field;
                    r->field_size = 0;
                    return EMBCSV_END_OF_ROW;
                }
                else
                    return EMBCSV_PARSE_ERROR;
            }

            if(!r->quoted && c == r->delimiter)
            {
                EMBCSV_reader_pushc(r, '\0');
                *field_out = r->field;
                r->field_size = 0;
                return EMBCSV_FIELD;
            }

            if(!r->quoted && (c == '\n' || c == '\r' || c == EOF || c == '\0'))
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
