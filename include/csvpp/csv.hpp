/// @file
/// @brief C++ CSV library

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

#ifndef CSV_HPP
#define CSV_HPP

#include <exception>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "version.h"

/// @defgroup cpp C++ library

/// CSV library namespace

/// @ingroup cpp
namespace csv
{
    /// @addtogroup cpp
    /// @{

    /// Library version
    static constexpr struct
    {
        int major {CSVPP_VERSION_MAJOR},
            minor {CSVPP_VERSION_MINOR},
            patch {CSVPP_VERSION_PATCH};
    } version;

    /// Error base class

    /// Base class for all library exceptions. Do not use directly
    class Error: virtual public std::exception
    {
    public:
        virtual ~Error() = default;
        /// @returns Exception message
        const char * what() const throw() override { return msg_.c_str(); }
    protected:
        Error() = default;
        explicit Error(const std::string & msg): msg_{msg} {}
    private:
        std::string msg_;
    };

    /// Internal error

    /// Thrown when an illegal state occurs
    struct Internal_error: virtual public Error
    {
        /// @param msg Error message
        explicit Internal_error(const std::string & msg): Error{msg} {}
    };

    /// Parsing error

    /// Thrown when Reader encounters a parsing error
    class Parse_error final: virtual public Error
    {
    public:
        /// @param type Parsing error type (ie. quote found inside of unquoted field)
        /// @param line_no Line that the error occured on
        /// @param col_no Column that the error occured on
        Parse_error(const std::string & type, int line_no, int col_no):
            Error{"Error parsing CSV at line: " +
                std::to_string(line_no) + ", col: " + std::to_string(col_no) + ": " + type},
            type_{type},
            line_no_{line_no},
            col_no_{col_no}
        {}

        /// @returns Paring error type (ie. quote found inside of unquoted field)
        std::string type() const { return type_; }
        /// @returns Line number that the error occured on
        int line_no() const { return line_no_; }
        /// @returns Column that the error occured on
        int col_no() const { return col_no_; }

    private:
        std::string type_;
        int line_no_;
        int col_no_;
    };

    /// Out of range error

    /// Thrown when Reader is read from beyond the end of input
    struct Out_of_range_error final: virtual public Error
    {
        /// @param msg Error message
        explicit Out_of_range_error(const std::string & msg): Error{msg} {}
    };

    /// Type conversion error

    /// Thrown when Reader fails convert a field to requested type
    struct Type_conversion_error final: virtual public Error
    {
    public:
        /// @param field Value of field that failed to convert
        explicit Type_conversion_error(const std::string & field):
            Error{"Could not convert '" + field + "' to requested type"},
            field_(field)
        {}

        /// @returns Value of field that failed to convert
        std::string field() const { return field_; }
    private:
        std::string field_;
    };

    /// IO error

    /// Thrown when an IO error occurs
    class IO_error final: virtual public Error
    {
    public:
        /// @param msg Error message
        /// @param errno_code \c errno code
        explicit IO_error(const std::string & msg, int errno_code):
            Error{msg + ": " + std::strerror(errno_code)},
            errno_code_{errno_code}
        {}
        /// @returns %Error message
        int errno_code() const { return errno_code_; }
        /// @returns \c errno code
        std::string errno_str() const { return std::strerror(errno_code_); }
    private:
        int errno_code_;
    };

    namespace detail
    {
        // SFINAE types to determine the best way to convert a given type to a std::string

        // Does the type support std::to_string?
        template <typename T, typename = void>
        struct has_std_to_string: std::false_type{};
        template <typename T>
        struct has_std_to_string<T, std::void_t<decltype(std::to_string(std::declval<T>()))>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_std_to_string_v = has_std_to_string<T>::value;

        // Does the type support a custom to_string?
        template <typename T, typename = void>
        struct has_to_string: std::false_type{};
        template <typename T>
        struct has_to_string<T, std::void_t<decltype(to_string(std::declval<T>()))>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_to_string_v = has_to_string<T>::value;

        // Does the type support conversion via std::ostream::operator>>
        template <typename T, typename = void>
        struct has_ostr: std::false_type{};
        template <typename T>
        struct has_ostr<T, std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_ostr_v = has_ostr<T>::value;
    };

    /// String conversion

    /// Convert a given type to std::string, using conversion, to_string, or std::ostream insertion
    /// @param t Data to convert to std::string
    /// @returns Input converted to std::string
    template <typename T, typename std::enable_if_t<std::is_convertible_v<T, std::string>, int> = 0>
    std::string str(const T & t)
    {
        return t;
    }

    template <typename T, typename std::enable_if_t<!std::is_convertible_v<T, std::string> && detail::has_std_to_string_v<T>, int> = 0>
    std::string str(const T & t)
    {
        return std::to_string(t);
    }

    template <typename T, typename std::enable_if_t<!std::is_convertible_v<T, std::string> && !detail::has_std_to_string_v<T> && detail::has_to_string_v<T>, int> = 0>
    std::string str(const T & t)
    {
        return to_string(t);
    }

    template <typename T, typename std::enable_if_t<!std::is_convertible_v<T, std::string> && !detail::has_std_to_string_v<T> && !detail::has_to_string_v<T> && detail::has_ostr_v<T>, int> = 0>
    std::string str(const T & t)
    {
        std::ostringstream os;
        os<<t;
        return os.str();
    }

    // special conversion for char using std::string's initializer list ctor
    std::string str(char c)
    {
        return {c};
    }

    /// Parses CSV data

    /// By default, parses according to RFC 4180 rules, and throws a Parse_error
    /// when given non-conformant input. The field delimiter and quote
    /// characters may be changed, and there is a lenient parsing option to ignore
    /// violations.
    ///
    /// Blank rows are ignored and skipped over.
    ///
    /// Most methods operate on rows, but some read field-by-field. Mixing
    /// row-wise and field-wise methods is not recommended, but is possible.
    /// Row-wise methods will act as if the current position is the start of a
    /// row, regardless of any fields that have been read from the current row so
    /// far.
    class Reader
    {
    public:
        /// Represents a single row of CSV data

        /// A Row may be obtained from Reader::get_row or Reader::Iterator
        /// @warning Reader object must not be destroyed or read from during Row lifetime
        class Row
        {
        public:
            /// Iterates over the fields within a Row

            /// @tparam T Type to convert fields to. Defaults to std::string
            template <typename T = std::string> class Iterator
            {
            public:
                using value_type        = T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = const T*;
                using reference         = const T&;
                using iterator_category = std::input_iterator_tag;

                /// Empty constructor

                /// Denotes the end of iteration
                Iterator(): row_{nullptr} {}

                /// Creates an iterator from a Row, and parses the first field

                /// @param row Row to iterate over.
                /// @warning \c row must not be destroyed or read from during iteration
                /// @throws Parse_error if error parsing first field (*only when not parsing in lenient mode*)
                /// @throws IO_error if error reading CSV data
                /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
                explicit Iterator(Reader::Row & row): row_{&row}
                {
                    ++*this;
                }

                /// @returns Current field
                const T & operator*() const { return obj_; }

                /// @returns Pointer to current field
                const T * operator->() const { return &obj_; }

                /// Parse and iterate to next field

                /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
                /// @throws IO_error if error reading CSV data
                /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
                Iterator & operator++()
                {
                    assert(row_);

                    if(end_of_row_)
                    {
                        row_ = nullptr;
                    }
                    else
                    {
                        obj_ = row_->read_field<T>();

                        if(row_->end_of_row())
                            end_of_row_ = true;
                    }

                    return *this;
                }

                /// Compare to another Reader::Row::Iterator
                bool equals(const Iterator<T> & rhs) const
                {
                    return row_ == rhs.row_;
                }

            private:
                Reader::Row * row_; ///< Pointer to parent Row object, or \c nullptr if past end of row
                T obj_{}; ///< Storage for current field
                bool end_of_row_ = false; ///< Keeps track of when the Row has hit end-of-row
            };

            /// Helper class for iterating over a Row. Use Row::range to obtain

            /// @tparam T Type to convert fields to. Defaults to std::string
            template<typename T = std::string>
            class Range
            {
            public:
                /// @returns Iterator to current field in row
                Row::Iterator<T> begin()
                {
                    return Row::Iterator<T>{row_};
                }

                /// @returns Iterator to end of row
                Row::Iterator<T> end()
                {
                    return Row::Iterator<T>{};
                }
            private:
                friend Row;
                explicit Range(Row & row):row_{row} {} ///< Construct a Range. Only for use by Row::range
                Row & row_; ///< Ref to parent Row object
            };

            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @returns Iterator to current field in row
            template<typename T = std::string>
            Row::Iterator<T> begin()
            {
                return Row::Iterator<T>{*this};
            }

            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @returns Iterator to end of row
            template<typename T = std::string>
            Row::Iterator<T> end()
            {
                return Row::Iterator<T>{};
            }

            /// Range helper

            /// Useful when iterating over a row and converting to a specific type
            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @returns A Range object containing begin and end methods corresponding to this Row
            template<typename T = std::string>
            Range<T> range()
            {
                return Range<T>{*this};
            }

            /// Read a single field from the row

            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @returns The next field from the row, or a default-initialized object if past the end of the row
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
            template<typename T = std::string>
            T read_field()
            {
                assert(reader_);

                if(end_of_row_)
                {
                    past_end_of_row_ = true;
                    return T{};
                }

                auto field = reader_->read_field<T>();

                if(reader_->end_of_row())
                    end_of_row_ = true;

                return field;
            }

            /// Read a single field from the row

            /// @param[out] data Variable to to write field to. Will store a
            ///                  default initialized object if past the end of the row
            /// @returns This Row object
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
            template<typename T>
            Row & operator>>(T & data)
            {
                data = read_field<T>();
                return * this;
            }

            /// Reads row into an output iterator

            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @param it an output iterator to receive the row data
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to type T
            template<typename T = std::string, typename OutputIter>
            void read(OutputIter it)
            {
                std::copy(begin<T>(), end<T>(), it);
            }

            /// Reads row into a std::vector

            /// @tparam T Type to convert fields to. Defaults to std::string
            /// @returns std::vector containing the fields from the Row
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to type T
            template<typename T = std::string>
            std::vector<T> read_vec()
            {
                std::vector<T> vec;
                std::copy(begin<T>(), end<T>(), std::back_inserter(vec));
                return vec;
            }

            /// Reads row into a tuple

            /// @tparam Args types to convert fields to
            /// @returns std::tuple containing the fields from the Row.
            /// If Args contains more elements than there are fields in the row,
            /// the remaining elements of the tuple will be default initialized
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to specified types
            template <typename ... Args>
            std::tuple<Args...> read_tuple()
            {
                std::tuple<Args...> ret;
                read_tuple_helper(ret, std::index_sequence_for<Args...>{});
                return ret;
            }

            /// Reads row into variadic arguments

            /// @param[out] data Variables to read into.
            /// If more parameters are passed than there are fields in the row,
            /// the remaining parameters will be default initialized
            /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
            /// @throws IO_error if error reading CSV data
            /// @throws Type_conversion_error if error converting to specified types
            template <typename ... Data>
            void read_v(Data & ... data)
            {
                (void)(*this >> ... >> data);
            }

            /// @returns \c true if the last field in the row has been read
            bool end_of_row() const { return end_of_row_; }

            /// @returns \c true if more fields can be read
            operator bool() { return reader_ && !past_end_of_row_; }

        private:
            friend Reader;

            /// Helper function for read_tuple

            /// Uses a std::index_sequence to generate indexes for std::get
            /// @param t tuple to load data into
            template <typename Tuple, std::size_t ... Is>
            void read_tuple_helper(Tuple & t, std::index_sequence<Is...>)
            {
                ((*this >> std::get<Is>(t)), ...);
            }

            /// Empty constructor

            /// Used to denote that no rows remain
            Row(): reader_{nullptr} {}

            /// Construct a Row from a Reader

            /// For use by Reader::get_row only
            explicit Row(Reader & reader): reader_{&reader} {}

            Reader * reader_ { nullptr }; ///< Pointer to parent Reader object or nullptr if no rows remain
            bool end_of_row_ { false }; ///< Tracks if the end of the current row has been reacehd
            bool past_end_of_row_ { false }; ///< Tracks if past the end of the row, and prevents reading from the next row
        };

        /// Iterates over Rows in CSV data
        class Iterator
        {
        public:
            using value_type        = Row;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const value_type*;
            using reference         = const value_type&;
            using iterator_category = std::input_iterator_tag;

            /// Empty constructor

            /// Denotes the end of iteration
            Iterator(): reader_{nullptr} {}

            /// Creates an iterator from a Reader object

            /// @param r Reader object to iterate over
            /// @warning \c r must not be destroyed or read from during iteration
            explicit Iterator(Reader & r): reader_{&r}
            {
                obj_ = reader_->get_row();

                if(!*reader_)
                    reader_ = nullptr;
            }

            /// @returns Current row
            const value_type & operator*() const { return obj_; }

            /// @returns Current row
            value_type & operator*() { return obj_; }

            /// @returns Pointer to current row
            const value_type * operator->() const { return &obj_; }

            /// @returns pointer to current row
            value_type * operator->() { return &obj_; }

            /// Iterate to next Row
            Iterator & operator++()
            {
                // discard any remaining fields
                while(!obj_.end_of_row())
                    obj_.read_field();

                assert(reader_);
                obj_ = reader_->get_row();

                if(!*reader_)
                    reader_ = nullptr;

                return *this;
            }

            /// Compare to another Reader::Iterator
            bool equals(const Iterator & rhs) const
            {
                return reader_ == rhs.reader_;
            }

        private:
            Reader * reader_ { nullptr }; ///< pointer to parent Reader object or nullptr if no rows remain
            value_type obj_; ///< Storage for the current Row
        };

        /// Use a std::istream for CSV parsing

        /// @param input_stream std::istream to read from
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        /// @warning \c input_stream must not be destroyed or read from during the lifetime of this Reader
        explicit Reader(std::istream & input_stream,
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            input_stream_{&input_stream},
            delimiter_{delimiter},
            quote_{quote},
            lenient_{lenient}
        {}

        /// Open a file for CSV parsing

        /// @param filename Path to a file to parse
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        /// @throws IO_error if there is an error opening the file
        explicit Reader(const std::string & filename,
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            internal_input_stream_{std::make_unique<std::ifstream>(filename)},
            input_stream_{internal_input_stream_.get()},
            delimiter_{delimiter},
            quote_{quote},
            lenient_{lenient}
        {
            if(!(*internal_input_stream_))
                throw IO_error("Could not open file '" + filename + "'", errno);
        }

        /// Disambiguation tag type

        /// Distinguishes opening a Reader with a filename from opening a Reader
        /// with a string
        struct input_string_t{};

        /// Disambiguation tag

        /// Distinguishes opening a Reader with a filename from opening a Reader
        /// with a string
        static inline constexpr input_string_t input_string{};

        /// Parse CSV from memory

        /// Use Reader::input_string to distinguish this constructor from the
        /// constructor accepting a filename
        /// @param input_data \c std::string containing CSV to parse
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        Reader(input_string_t, const std::string & input_data,
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            internal_input_stream_{std::make_unique<std::istringstream>(input_data)},
            input_stream_{internal_input_stream_.get()},
            delimiter_{delimiter},
            quote_{quote},
            lenient_{lenient}
        {}

        ~Reader() = default;

        Reader(const Reader &) = delete;
        Reader & operator=(const Reader &) = delete;
        Reader(Reader &&) = default;
        Reader & operator=(Reader &&) = default;

        /// Check for end of input

        /// @returns \c true if the last field in the row has been read
        bool end_of_row() const { return end_of_row_ || eof(); }

        /// Check for end of row

        /// @returns \c true if no fields remain in the current row
        bool eof() const { return state_ == State::eof; }

        /// @returns \c true if there is more data to be read
        operator bool() { return !eof(); }

        /// Change the delimiter character

        /// @param delimiter New delimiter character
        void set_delimiter(const char delimiter) { delimiter_ = delimiter; }
        /// Change the quote character

        /// @param quote New quote character
        void set_quote(const char quote) { quote_ = quote; }

        /// Enable / disable lenient parsing

        /// Lenient parsing will attempt to ignore syntax errors in CSV input.
        /// @param lenient \c true for lenient parsing
        void set_lenient(const bool lenient) { lenient_ = lenient; }

        /// @returns Iterator to current row
        Iterator begin()
        {
            return Iterator(*this);
        }

        /// @returns Iterator to end of CSV data
        auto end()
        {
            return Iterator();
        }

        /// Read a single field

        /// Check end_of_row() to see if this is the last field in the current row
        /// @tparam T Type to convert fields to. Defaults to std::string
        /// @returns The next field from the row, or a default-initialized object if past the end of the input data
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
        template<typename T = std::string>
        T read_field()
        {
            if(eof())
                return {};

            end_of_row_ = false;

            std::string field;
            if(conversion_retry_)
            {
                field = *conversion_retry_;
                conversion_retry_.reset();
            }
            else
            {
                field = parse();
            }

            // no conversion needed for strings
            if constexpr(std::is_convertible_v<std::string, T>)
            {
                return field;
            }
            else
            {
                T field_val{};
                std::istringstream convert(field);
                convert>>field_val;
                if(!convert || convert.peek() != std::istream::traits_type::eof())
                {
                    conversion_retry_ = field;
                    throw Type_conversion_error(field);
                }

                return field_val;
            }
        }

        /// Read a single field

        /// Check end_of_row() to see if this is the last field in the current row
        /// @param[out] data Variable to to write field to
        /// @returns This Reader object
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type T. Caller may call this again with a different type to try again
        template<typename T>
        Reader & operator>>(T & data)
        {
            data = read_field<T>();
            return * this;
        }

        /// Reads fields into variadic arguments

        /// @warning This may be used to fields from multiple rows at a time.
        /// Use with caution if the number of fields per row is not known
        /// beforehand.
        /// @param[out] data Variables to read into.
        /// If more parameters are passed than there are fields remaining,
        /// the remaining parameters will be default initialized
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to specified types
        template <typename ... Data>
        void read_v(Data & ... data)
        {
            (void)(*this >> ... >> data);
        }

        /// Get the current Row

        /// @returns Row object for the current row
        Row get_row()
        {
            consume_newlines();
            if(eof())
                return Row{};
            else
                return Row{*this};
        }

        /// Reads current row into an output iterator

        /// @tparam T Type to convert fields to. Defaults to std::string
        /// @param it An output iterator to receive the row data
        /// @returns \c false if this is the last row
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type T
        template <typename T = std::string, typename OutputIter>
        bool read_row(OutputIter it)
        {
            auto row = get_row();
            if(!row)
                return false;

            row.read<T>(it);
            return true;
        }

        /// Reads current row into a std::vector

        /// @tparam T Type to convert fields to. Defaults to std::string
        /// @returns std::vector containing the fields from the row or empty optional if no rows remain
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type T
        template <typename T = std::string>
        std::optional<std::vector<T>> read_row_vec()
        {
            auto row = get_row();
            if(!row)
                return {};

            return row.read_vec<T>();
        }

        /// Reads current row into a tuple

        /// @tparam Args types to convert fields to
        /// @returns std::tuple containing the fields from the row or empty
        /// optional if no rows remain. If Args contains more elements than there
        /// are fields in the row, the remaining elements of the tuple will be
        /// default initialized
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to specified types
        template <typename ... Args>
        std::optional<std::tuple<Args...>> read_row_tuple()
        {
            auto row = get_row();
            if(!row)
                return {};

            return row.read_tuple<Args...>();
        }

        /// Read entire CSV data into a vector of vectors

        /// @tparam T Type to convert fields to. Defaults to std::string
        /// @returns 2D vector of CSV data.
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type T
        template <typename T = std::string>
        std::vector<std::vector<T>> read_all()
        {
            std::vector<std::vector<T>> data;
            while(true)
            {
                auto row = read_row_vec<T>();
                if(row)
                    data.push_back(*row);
                else
                    break;
            }
            return data;
        }

    private:

        /// Get next character from input

        /// Updates line and column position, and checks for IO error
        /// @returns Next character
        /// @throws IO_error
        int getc()
        {
            int c = input_stream_->get();
            if(input_stream_->bad() && !input_stream_->eof())
                throw IO_error{"Error reading from input", errno};

            if(c == '\n')
            {
                ++line_no_;
                col_no_ = 0;
            }
            else if(c != std::istream::traits_type::eof())
                ++col_no_;

            return c;
        }

        /// Consume newline characters

        /// Advance stream position until first non-newline character
        /// @throws IO_error
        void consume_newlines()
        {
            if(state_ != State::consume_newlines)
                return;

            while(true)
            {
                if(int c = getc(); c == std::istream::traits_type::eof())
                {
                    end_of_row_ = true;
                    state_ = State::eof;
                    break;
                }
                else if(c != '\r' && c != '\n')
                {
                    state_ = State::read;
                    input_stream_->unget();
                    --col_no_;
                    break;
                }
            }
        }

        /// Core parsing method

        /// Reads and parses character stream to obtain next field
        /// @returns Next field, or empty string if at EOF
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading from stream
        std::string parse()
        {
            consume_newlines();

            if(eof())
                return {};

            bool quoted = false;
            std::string field;

            bool field_done = false;
            while(!field_done)
            {
                int c = getc();
                bool c_done = false;
                while(!c_done)
                {
                    switch(state_)
                    {
                    case State::quote:
                        // end of the field?
                        if(c == delimiter_ || c == '\n' || c == '\r' || c == std::istream::traits_type::eof())
                        {
                            quoted = false;
                            state_ = State::read;
                            break;
                        }
                        // if it's not an escaped quote, then it's an error
                        else if(c == quote_)
                        {
                            field += c;
                            state_ = State::read;
                            c_done = true;
                            break;
                        }
                        else if(lenient_)
                        {
                            field += quote_;
                            field += c;
                            state_ = State::read;
                            c_done = true;
                            break;
                        }
                        else
                            throw Parse_error("Unescaped quote", line_no_, col_no_ - 1);

                    case State::read:
                        // we need special handling for quotes
                        if(c == quote_)
                        {
                            if(quoted)
                            {
                                state_ = State::quote;
                                c_done = true;
                                break;
                            }
                            else
                            {
                                if(field.empty())
                                {
                                    quoted = true;
                                    c_done = true;
                                    break;
                                }
                                else if(!lenient_)
                                {
                                    // quotes are not allowed inside of an unquoted field
                                    throw Parse_error("quote found in unquoted field", line_no_, col_no_);
                                }
                            }
                        }

                        if(quoted && c == std::istream::traits_type::eof())
                        {
                            if(lenient_)
                            {
                                end_of_row_ = field_done = c_done = true;
                                state_ = State::consume_newlines;
                                break;
                            }
                            else
                                throw Parse_error("Unterminated quoted field - reached end-of-file", line_no_, col_no_);
                        }
                        else if(!quoted && c == delimiter_)
                        {
                            field_done = c_done = true;
                            break;
                        }
                        else if(!quoted && (c == '\n' || c == '\r' || c == std::istream::traits_type::eof()))
                        {
                            end_of_row_ = field_done = c_done = true;
                            state_ = State::consume_newlines;
                            break;
                        }

                        field += c;
                        c_done = true;
                        break;

                    default:
                        // It should not be possible to reach this state
                        throw Internal_error{"Illegal state"};
                    }
                }
            }
            return field;
        }

        /// Owns an istream created by this Reader (either an ifstream when
        /// constructed by filename or a istringstream when constructed by input
        /// string)
        std::unique_ptr<std::istream> internal_input_stream_;

        /// Points to input istream. Will point to *internal_input_stream_ if
        /// constructed by filename or input string, or the istream passed when
        /// constructed by istream
        std::istream * input_stream_;

        char delimiter_ {','};   ///< Delimiter character
        char quote_ {'"'};       ///< Quote character
        bool lenient_ { false }; ///< Lenient parsing enabled / disabled

        std::optional<std::string> conversion_retry_; ///< Contains last field after type conversion error. Allows retrying conversion
        bool end_of_row_ { false }; ///< \c true if parsing is at the end of a row

        /// Parsing states
        enum class State
        {
            read,             ///< Ready to read a character into current field
            quote,            ///< Checking for escaped quote character or end of quoted field
            consume_newlines, ///< Discarding any newline characters
            eof               ///< At end of input stream
        };

        /// Current parser state
        State state_ { State::consume_newlines };

        unsigned int line_no_ { 1 }; ///< Current line number within input
        unsigned int col_no_ { 0 };  ///< Current column number within input
    };

    /// Compare two Reader::Iterator objects
    inline bool operator==(const Reader::Iterator & lhs, const Reader::Iterator & rhs)
    {
        return lhs.equals(rhs);
    }

    /// Compare two Reader::Iterator objects
    inline bool operator!=(const Reader::Iterator & lhs, const Reader::Iterator & rhs)
    {
        return !lhs.equals(rhs);
    }

    /// Compare two Reader::Row::Iterator objects
    template <typename T>
    inline bool operator==(const Reader::Row::Iterator<T> & lhs, const Reader::Row::Iterator<T> & rhs)
    {
        return lhs.equals(rhs);
    }

    /// Compare two Reader::Row::Iterator objects
    template <typename T>
    inline bool operator!=(const Reader::Row::Iterator<T> & lhs, const Reader::Row::Iterator<T> & rhs)
    {
        return !lhs.equals(rhs);
    }

    /// Map-based Reader iterator

    /// Iterates through a Reader, returning rows as a std::map.
    /// Map keys (headers) come from the first row unless specified by the header
    /// parameter to the constructor. If a row has more fields than the header,
    /// Out_of_range_error error will be thrown
    template <typename Header = std::string, typename Value = std::string>
    class Map_reader_iter
    {
    private:
        std::unique_ptr<Reader> reader_; ///< Reader object
        Value default_val_;              ///< Default value
        std::vector<Header> headers_;    ///< Headers
        std::map<Header, Value> obj_;    ///< Current row storage

        /// Read header row from input (or use headers parameter)

        /// @param headers Use this as headers. Pass an empty vector to use the
        /// first row of the input as the headers
        /// @returns Headers
        /// @throws Parse_error if error parsing field (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting to type Header.
        std::vector<Header> get_header_row(const std::vector<Header> & headers)
        {
            if(!std::empty(headers))
                return headers;

            auto header_row = reader_->read_row_vec<Header>();
            if(header_row)
                return *header_row;
            else
                throw Parse_error("Can't get header row", 0, 0);
        }

        template <typename Header2, typename Value2>
        friend class Map_reader_iter;

    public:
        /// Empty constructor

        /// Denotes the end of iteration
        Map_reader_iter() {}

        /// Open a std::istream for CSV parsing

        /// @param input_stream std::istream to read from
        /// @param default_val Value to use if a row has fewer fields than the header
        /// @param headers List of field names to use. Pass an empty vector to
        ///        use first (header) row
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        /// @warning input_stream must not be destroyed or read from during the
        ///          lifetime of this Map_reader_iter
        /// @throws Parse_error if error parsing fields (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting 1st row to type
        /// Header (if headers param is empty), or 1st row to type Value (if
        /// header param is specified)
        explicit Map_reader_iter(std::istream & input_stream, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            reader_{std::make_unique<Reader>(input_stream, delimiter, quote, lenient)},
            default_val_{default_val},
            headers_{get_header_row(headers)}
        {
            ++(*this);
        }

        /// Open a file for CSV parsing

        /// @param filename Path to a file to parse
        /// @param default_val Value to use if a row has fewer fields than the header
        /// @param headers List of field names to use. Pass an empty vector to
        ///        use first (header) row
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        /// @throws Parse_error if error parsing fields (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting 1st row to type
        /// Header (if headers param is empty), or 1st row to type Value (if
        /// header param is specified)
        explicit Map_reader_iter(const std::string & filename, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            reader_{std::make_unique<Reader>(filename, delimiter, quote, lenient)},
            default_val_{default_val},
            headers_{get_header_row(headers)}
        {
            ++(*this);
        }

        /// Parse CSV from memory

        /// Use Reader::input_string to distinguish this constructor from the
        /// constructor accepting a filename
        /// @param input_data String containing CSV to parse
        /// @param default_val Value to use if a row has fewer fields than the header
        /// @param headers List of field names to use. Pass an empty vector to
        ///        use first (header) row
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @param lenient Enable lenient parsing (will attempt to read past syntax errors)
        /// @throws Parse_error if error parsing fields (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting 1st row to type
        /// Header (if headers param not specified), or 1st row to type Value (if
        /// header param is specified)
        Map_reader_iter(Reader::input_string_t, const std::string & input_data, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"',
                const bool lenient = false):
            reader_{std::make_unique<Reader>(Reader::input_string, input_data, delimiter, quote, lenient)},
            default_val_{default_val},
            headers_{get_header_row(headers)}
        {
            ++(*this);
        }

        using value_type        = std::map<Header, Value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;
        using iterator_category = std::input_iterator_tag;

        /// @returns Current row as a map
        const value_type & operator*() const { return obj_; }
        value_type & operator*() { return obj_; }

        /// @returns Pointer to current row map
        const value_type * operator->() const { return &obj_; }
        value_type * operator->() { return &obj_; }

        /// Get a field from the current row

        /// This is a shortcut for `map_reader_iter->at(key)`
        /// @param key Header for the desired field
        /// @returns The requested field
        /// @throws std::out_or_range if the key is not a valid header
        const typename value_type::mapped_type & operator[](const typename value_type::key_type & key) const { return obj_.at(key); }
        typename value_type::mapped_type & operator[](const typename value_type::key_type & key) { return obj_.at(key); }

        /// Iterate to next field

        /// @throws Parse_error if error parsing fields (*only when not parsing in lenient mode*)
        /// @throws IO_error if error reading CSV data
        /// @throws Type_conversion_error if error converting fields to Value type
        Map_reader_iter & operator++()
        {
            auto row = reader_->read_row_vec<Value>();
            if(!row)
                reader_.reset();
            else
            {
                if(std::size(*row) > std::size(headers_))
                    throw Out_of_range_error("Too many fields");

                for(std::size_t i = 0; i < std::size(*row); ++i)
                    obj_[headers_[i]] = (*row)[i];

                for(std::size_t i = std::size(*row); i < std::size(headers_); ++i)
                    obj_[headers_[i]] = default_val_;
            }

            return *this;
        }

        /// Compare to another Map_reader_iter
        template <typename Header2, typename Value2>
        bool equals(const Map_reader_iter<Header2, Value2> & rhs) const
        {
            return reader_ == rhs.reader_;
        }

        /// Get the headers

        /// @returns Headers as a vector
        std::vector<Header> get_headers() const
        {
            return headers_;
        }
    };

    /// Compare two Map_reader_iter objects
    template <typename Header1, typename Value1, typename Header2, typename Value2>
    inline bool operator==(const Map_reader_iter<Header1, Value1> & lhs, const Map_reader_iter<Header2, Value2> & rhs)
    {
        return lhs.equals(rhs);
    }

    /// compare two Map_reader_iter objects
    template <typename Header1, typename Value1, typename Header2, typename Value2>
    inline bool operator!=(const Map_reader_iter<Header1, Value1> & lhs, const Map_reader_iter<Header2, Value2> & rhs)
    {
        return !lhs.equals(rhs);
    }

    /// CSV writer

    /// Writes data in CSV format, with correct escaping as needed, according to RFC 4180 rules.
    /// Allows writing by rows or field-by-field. Mixing these is not
    /// recommended, but is possible. Row-wise methods will append to the row
    /// started by any field-wise methods.
    class Writer
    {
    public:
        /// Output iterator for writing CSV data field-by-field

        /// This iterator has no mechanism for ending a row. Use Writer::end_row instead
        class Iterator
        {
        public:
            using value_type        = void;
            using difference_type   = void;
            using pointer           = void;
            using reference         = void;
            using iterator_category = std::output_iterator_tag;

            /// Creates an iterator from a Writer object

            /// @warning \c w must not be destroyed during iteration
            explicit Iterator(Writer & w): writer_{w} {}

            /// No-op
            Iterator & operator*() { return *this; }
            /// No-op
            Iterator & operator++() { return *this; }
            /// No-op
            Iterator & operator++(int) { return *this; }

            /// Writes a field to the CSV output

            /// @param field Data to write. Type must be convertible to std::string
            ///        either directly, by \c to_string, or by `ostream::operator<<`
            /// @throws IO_error if there is an error writing
            template <typename T>
            Iterator & operator=(const T & field)
            {
                writer_.write_field(field);
                return *this;
            }

        private:
            Writer & writer_; ///< Ref to parent Writer object
        };

        /// Use a std::ostream for CSV output

        /// @param output_stream std::ostream to write to
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @warning \c output_stream must not be destroyed or written to during the lifetime of this Writer
        explicit Writer(std::ostream & output_stream,
                const char delimiter = ',', const char quote = '"'):
            output_stream_{&output_stream},
            delimiter_{delimiter},
            quote_{quote}
        {}

        /// Open a file for CSV output

        /// @param filename Path to file to write to. Any existing file will be overwritten
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @warning \c output_stream must not be destroyed or written to during the lifetime of this Writer
        /// @throws IO_error if there is an error opening the file
        explicit Writer(const std::string& filename,
                const char delimiter = ',', const char quote = '"'):
            internal_output_stream_{std::make_unique<std::ofstream>(filename, std::ios::binary)},
            output_stream_{internal_output_stream_.get()},
            delimiter_{delimiter},
            quote_{quote}
        {
            if(!(*internal_output_stream_))
                throw IO_error("Could not open file '" + filename + "'", errno);
        }

        /// Destructor

        /// Writes a final newline sequence if needed to close current row
        ~Writer()
        {
            if(!start_of_row_)
            {
                // try to end the row, but ignore any IO errors
                try { end_row(); }
                catch(const IO_error & e) {}
            }
        }

        Writer(const Writer &) = delete;
        Writer(Writer &&) = default;
        Writer & operator=(const Writer &) = delete;
        Writer & operator=(Writer &&) = default;

        /// Get iterator

        /// @returns An iterator on this Writer
        /// @throws IO_error if there is an error writing
        Iterator iterator()
        {
            return Iterator(*this);
        }

        /// Change the delimiter character

        /// @param delimiter New delimiter character
        void set_delimiter(const char delimiter) { delimiter_ = delimiter; }

        /// Change the quote character

        /// @param quote New quote character
        void set_quote(const char quote) { quote_ = quote; }

        /// Writes a field to the CSV output

        /// @param field Data to write. Type must be convertible to std::string
        /// either directly, by \c to_string, or by `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename T>
        void write_field(const T & field)
        {
            if(!start_of_row_)
            {
                (*output_stream_)<<delimiter_;
                if(output_stream_->bad())
                    throw IO_error{"Error writing to output", errno};
            }

            (*output_stream_)<<quote(field);
            if(output_stream_->bad())
                throw IO_error{"Error writing to output", errno};

            start_of_row_ = false;
        }

        /// Writes a field to the CSV output

        /// @param field Data to write. Type must be convertible to std::string
        /// either directly, by \c to_string, or by `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename T>
        Writer & operator<<(const T & field)
        {
            write_field(field);
            return *this;
        }

        /// Apply a stream manipulator to the CSV output

        /// Currently only csv::end_row is supported
        /// @param manip Stream manipulator to apply
        Writer & operator<<(Writer & (*manip)(Writer &))
        {
            manip(*this);
            return *this;
        }

        /// End the current row

        /// @throws IO_error if there is an error writing
        void end_row()
        {
            (*output_stream_)<<"\r\n";
            if(output_stream_->bad())
                throw IO_error{"Error writing to output", errno};
            start_of_row_ = true;
        }

        /// Write fields from iterators, without ending the row
        /// @param first Iterator to start of data to write. Dereferenced type
        /// must be convertible to std::string either directly, by \c to_string,
        /// or by `ostream::operator<<`
        /// @param last Iterator to end of data to write
        /// @throws IO_error if there is an error writing
        template<typename Iter>
        void write_fields(Iter first, Iter last)
        {
            for(; first != last; ++first)
                write_field(*first);
        }

        /// Write fields from an initializer_list, without ending the row
        /// @tparam T Type of data. Must be convertible to std::string either
        /// directly, by \c to_string, or by `ostream::operator<<`
        /// @param data list of fields to write
        /// @throws IO_error if there is an error writing
        template<typename T>
        void write_fields(const std::initializer_list<T> & data)
        {
            write_fields(std::begin(data), std::end(data));
        }

        /// Write fileds from a range, without ending the row

        /// A range in this context must support std::begin and std::end as at
        /// least input iterators. Most STL containers, such as std::vector will
        /// work. The ranges type must be convertible to std::string either
        /// directly, by \c to_string, or by `ostream::operator<<`
        /// @param data %Range of fields to write
        /// @throws IO_error if there is an error writing
        template<typename Range>
        void write_fields(const Range & data)
        {
            write_fields(std::begin(data), std::end(data));
        }

        /// Write fileds from the given variadic parameters, without ending the row

        /// @param data Fields to write. Each must be convertible to std::string
        /// either directly, by \c to_string, or by `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename ...Data>
        void write_fields_v(const Data & ...data)
        {
            (void)(*this << ... << data);
        }

        /// Write fileds from a tuple, without ending the row

        /// @param data tuple of fields to write. Each element must be convertible to
        /// std::string either directly, by \c to_string, or by
        /// `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename ...Args>
        void write_fields(const std::tuple<Args...> & data)
        {
            std::apply(&Writer::write_fields_v<Args...>, std::tuple_cat(std::tuple(std::ref(*this)), data));
        }

        /// Write a row from iterators
        /// @param first Iterator to start of data to write. Dereferenced type
        /// must be convertible to std::string either directly, by \c to_string,
        /// or by `ostream::operator<<`
        /// @param last Iterator to end of data to write
        /// @throws IO_error if there is an error writing
        template<typename Iter>
        void write_row(Iter first, Iter last)
        {
            write_fields(first, last);

            end_row();
        }

        /// Write a row from an initializer_list
        /// @tparam T Type of data. Must be convertible to std::string either
        /// directly, by \c to_string, or by `ostream::operator<<`
        /// @param data list of fields to write
        /// @throws IO_error if there is an error writing
        template<typename T>
        void write_row(const std::initializer_list<T> & data)
        {
            write_row(std::begin(data), std::end(data));
        }

        /// Write a row from a range

        /// A range in this context must support std::begin and std::end as at
        /// least input iterators. Most STL containers, such as std::vector will
        /// work. The ranges type must be convertible to std::string either
        /// directly, by \c to_string, or by `ostream::operator<<`
        /// @param data %Range of fields to write
        /// @throws IO_error if there is an error writing
        template<typename Range>
        void write_row(const Range & data)
        {
            write_row(std::begin(data), std::end(data));
        }

        /// Write a row from the given variadic parameters

        /// @param data Fields to write. Each must be convertible to std::string
        /// either directly, by \c to_string, or by `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename ...Data>
        void write_row_v(const Data & ...data)
        {
            (void)(*this << ... << data);

            end_row();
        }

        /// Write a row from a tuple

        /// @param data tuple of fields to write. Each element must be convertible to
        /// std::string either directly, by \c to_string, or by
        /// `ostream::operator<<`
        /// @throws IO_error if there is an error writing
        template<typename ...Args>
        void write_row(const std::tuple<Args...> & data)
        {
            std::apply(&Writer::write_row_v<Args...>, std::tuple_cat(std::tuple(std::ref(*this)), data));
        }

    private:
        /// Quote a field, if quotation is needed

        /// @oaram field Field to quote. Must be convertible to std::string by
        /// csv::str
        /// @returns \c field as a std::string, quoted if necessary
        template<typename T>
        std::string quote(const T & field)
        {
            std::string field_str = str(field);

            std::string ret{quote_};
            ret.reserve(std::size(field_str) + 2);
            bool quoted = false;
            for(auto c: field_str)
            {
                if(c == quote_)
                {
                    quoted = true;
                    ret += quote_;
                }
                else if(c == delimiter_ || c == '\r' || c == '\n')
                    quoted = true;

                ret += c;
            }

            if(!quoted)
                return field_str;

            ret += quote_;
            return ret;
        }

        friend Writer &end_row(Writer & w);

        /// Owns an ofstream created by this Reader when constructed by filename
        std::unique_ptr<std::ostream> internal_output_stream_;

        /// Points to output ostream. Will point to *internal_output_stream_ if
        /// constructed by filename or the ostream passed when constructed by
        /// ostream
        std::ostream * output_stream_;
        bool start_of_row_ {true}; ///< for keeping track if when a row needs to be ended

        char delimiter_ {','};
        char quote_ {'"'};
    };

    /// End row stream manipulator for Writer

    /// Ends the current row, as in: `writer << field << csv::end_row; `
    /// @throws IO_error if there is an error writing
    inline Writer & end_row(Writer & w)
    {
        w.end_row();
        return w;
    }

    /// Map-based Writer iterator

    /// Output iterator accepting a std::map to write as a CSV row
    template <typename Header, typename Default_value = std::string>
    class Map_writer_iter
    {
    private:
        std::unique_ptr<Writer> writer_; ///< Writer object
        std::vector<Header> headers_;    ///< Headers
        Default_value default_val_;      ///< Default value

    public:
        /// Use a std::ostream for CSV output

        /// @param output_stream std::ostream to write to
        /// @param headers Field headers to use. This specifies the header row and order
        /// @param default_val Default value to write to a field if not specified in row input
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @warning \c output_stream must not be destroyed or written to during the lifetime of this Writer
        Map_writer_iter(std::ostream & output_stream, const std::vector<Header> & headers, const Default_value & default_val = {},
                const char delimiter = ',', const char quote = '"'):
            writer_{std::make_unique<Writer>(output_stream, delimiter, quote)}, headers_{headers}, default_val_{default_val}
        {
            writer_->write_row(headers);
        }

        /// Open a file for CSV output

        /// @param filename Path to file to write to. Any existing file will be overwritten
        /// @param headers Field headers to use. This specifies the header row and order
        /// @param default_val Default value to write to a field if not specified in row input
        /// @param delimiter Delimiter character
        /// @param quote Quote character
        /// @warning \c output_stream must not be destroyed or written to during the lifetime of this Writer
        /// @throws IO_error if there is an error opening the file
        Map_writer_iter(const std::string& filename, const std::vector<Header> & headers, const Default_value & default_val = {},
                const char delimiter = ',', const char quote = '"'):
            writer_{std::make_unique<Writer>(filename, delimiter, quote)}, headers_{headers}, default_val_{default_val}
        {
            writer_->write_row(headers);
        }

        using value_type        = void;
        using difference_type   = void;
        using pointer           = void;
        using reference         = void;
        using iterator_category = std::output_iterator_tag;

        /// No-op
        Map_writer_iter & operator*() { return *this; }
        /// No-op
        Map_writer_iter & operator++() { return *this; }
        /// No-op
        Map_writer_iter & operator++(int) { return *this; }

        /// Write a row

        /// @param row std::map containing header to field pairs. If row
        /// contains keys not in the specified header, the associated values will
        /// be ignored. If the map is missing headers, their values will be filled
        /// with default_val
        /// @throws IO_error if there is an error writing
        template <typename K, typename T, typename std::enable_if_t<std::is_convertible_v<Header, K>, int> = 0>
        Map_writer_iter & operator=(const std::map<K, T> & row)
        {
            for(auto & h: headers_)
            {
                try
                {
                    (*writer_)<<row.at(h);
                }
                catch(std::out_of_range&)
                {
                    (*writer_)<<default_val_;
                }
            }

            writer_->end_row();
            return *this;
        }
    };
    /// @} // end doxygen group
};

#endif // CSV_HPP
