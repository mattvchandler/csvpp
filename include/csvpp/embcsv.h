/// @file
/// @brief CSV library for embedded environments

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

/// @defgroup emb Embedded CSV library
/// @details CSV parser designed for use in embedded environments.
///
/// CSV input is parsed character-by-character, allowing reading from unbuffered
/// input sources.
///
/// If malloc is not available or desired, compile with \c EMBCSV_NO_MALLOC set.
/// The parser will then used a fixed-length buffer internally. This buffers size
/// can be adjusted by setting #EMBCSV_FIELD_BUF_SIZE at compilation. Any field
/// exceeding this length will be truncated. The default size is 16. Note that
/// fields will be at most #EMBCSV_FIELD_BUF_SIZE - 1 characters
/// due to the null-terminator.
///
/// EMBCSV_NO_MALLOC also changes the init function to accept an EMBCSV_reader
/// to initialize rather than allocating a new one

#ifndef EMBCSV_H
#define EMBCSV_H

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "version.h"

#ifndef EMBCSV_FIELD_BUF_SIZE
/// @brief Field buffer size. Any fields at or over this limit will be truncated
/// @ingroup emb
#define EMBCSV_FIELD_BUF_SIZE 16
#endif

/// CSV Reader
/// @ingroup emb
struct EMBCSV_reader
{
    #ifndef EMBCSV_NO_MALLOC
    char * field;                      ///< Field storage
    size_t field_alloc;                ///< Field allocated size
    #else
    char field[EMBCSV_FIELD_BUF_SIZE]; ///< Field storage
    #endif
    size_t field_size;                 ///< Field size

    char delimiter;                    ///< Delimiter character (default ',')
    char quote;                        ///< Quote character (default '"')
    bool lenient;                      ///< \c true if parsing is at the end of a row

    bool quoted;                       ///< \c true if parsing a quoted field


    /// Parsing states
    enum {
        EMBCSV_STATE_READY,           ///< Ready to read a character into current field
        EMBCSV_STATE_DOUBLE_QUOTE,    ///< Checking for escaped quote character or end of quoted field
        EMBCSV_STATE_CONSUME_NEWLINES ///< Discarding any newline characters
    } state;
};

/// @ingroup emb
typedef struct EMBCSV_reader EMBCSV_reader;

/// Parser result type

/// @ingroup emb
typedef enum
{
    EMBCSV_INCOMPLETE, ///< Parsing is incomplete. Parse more characters
    EMBCSV_FIELD,      ///< A field has been parsed
    EMBCSV_END_OF_ROW, ///< A field has been parsed, and parsing has reached the end of a row
    EMBCSV_PARSE_ERROR ///< Syntax error found in input
} EMBCSV_result;

#ifndef EMBCSV_NO_MALLOC

    /// Create a new EMBCSV_reader

    /// @param delimiter Delimiter character
    /// @param quote Quote character
    /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
    /// @returns New EMBCSV_reader. Free with EMBCSV_reader_free()
    /// @ingroup emb
    EMBCSV_reader * EMBCSV_reader_init_full(char delimiter, char quote, bool lenient);

    /// Create a new EMBCSV_reader with default settings

    /// Equivalent to <code>EMBCSV_reader_init_full(',', '"', false)</code>
    /// @returns New EMBCSV_reader. Free with EMBCSV_reader_free()
    /// @ingroup emb
    EMBCSV_reader * EMBCSV_reader_init(void);

    /// Free an EMBCSV_reader

    /// @ingroup emb
    void EMBCSV_reader_free(EMBCSV_reader * r);

#else

    /// Create a new EMBCSV_reader

    /// @param r pointer to EMBCSV_reader to initialize
    /// @param delimiter Delimiter character
    /// @param quote Quote character
    /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
    /// @ingroup emb
    void EMBCSV_reader_init_full(EMBCSV_reader *r, char delimiter, char quote, bool lenient);


    /// Create a new EMBCSV_reader with default settings

    /// Equivalent to <code>EMBCSV_reader_init_full(r, ',', '"', false)</code>
    /// @param r pointer to EMBCSV_reader to initialize
    /// @ingroup emb
    void EMBCSV_reader_init(EMBCSV_reader *r);

#endif

/// Parse a character

/// @param c Character to parse
/// @param[out] field_out Pointer to string, in which will be stored:
/// * NULL if no field has been parsed. (call returned #EMBCSV_INCOMPLETE or #EMBCSV_PARSE_ERROR)
/// * The parsed field if a field has been parsed (call returned #EMBCSV_FIELD or #EMBCSV_END_OF_ROW).
/// This string is owned by the EMBCSV_reader. Do not free this value. Contents will change
/// on next call to EMBCSV_reader_parse_char(), so copy if needed
/// @returns Result of parsing as an #EMBCSV_result
/// @ingroup emb
EMBCSV_result EMBCSV_reader_parse_char(EMBCSV_reader * r, int c, const char ** field_out);

#ifdef __cplusplus
}
#endif

#endif // EMBCSV_H
