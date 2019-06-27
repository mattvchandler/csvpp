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

#ifndef CSV_HPP
#define CSV_HPP

#include <exception>
#include <fstream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include <cassert>

namespace csv
{
    class Parse_error: public std::runtime_error
    {
    public:
        Parse_error(const char * msg, int line_no, int col_no):
            std::runtime_error("Error parsing CSV at line: " +
                std::to_string(line_no) + ", col: " + std::to_string(col_no) + ": " + msg)
        {}
    };

    class Out_of_range_error: public std::out_of_range
    {
    public:
        Out_of_range_error(const char * msg): std::out_of_range(msg) {}
    };

    class Type_conversion_error: public std::runtime_error
    {
    public:
        Type_conversion_error(const std::string & field):
            std::runtime_error("Could not convert '" + field + "' to requested type")
        {}
    };

    namespace detail
    {
        template <typename T, typename = void>
        struct has_std_to_string: std::false_type{};
        template <typename T>
        struct has_std_to_string<T, std::void_t<decltype(std::to_string(std::declval<T>()))>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_std_to_string_v = has_std_to_string<T>::value;

        template <typename T, typename = void>
        struct has_to_string: std::false_type{};
        template <typename T>
        struct has_to_string<T, std::void_t<decltype(to_string(std::declval<T>()))>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_to_string_v = has_to_string<T>::value;

        template <typename T, typename = void>
        struct has_ostr: std::false_type{};
        template <typename T>
        struct has_ostr<T, std::void_t<decltype(std::declval<std::ostringstream&>() << std::declval<T>())>> : std::true_type{};
        template <typename T>
        inline constexpr bool has_ostr_v = has_ostr<T>::value;
    };

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

    class Reader
    {
    public:
        class Row
        {
        public:
            template <typename T = std::string> class Iterator
            {
            public:
                using value_type        = T;
                using difference_type   = std::ptrdiff_t;
                using pointer           = const T*;
                using reference         = const T&;
                using iterator_category = std::input_iterator_tag;

                Iterator(): row(nullptr)
                {}
                explicit Iterator(Reader::Row & row): row(&row)
                {
                    ++*this;
                }

                const T & operator*() const { return obj; }
                const T * operator->() const { return &obj; }

                Iterator & operator++()
                {
                    assert(row);

                    if(end_of_row_)
                    {
                        row = nullptr;
                    }
                    else
                    {
                        obj = row->read_field<T>();

                        if(row->end_of_row())
                            end_of_row_ = true;
                    }

                    return *this;
                }
                Iterator & operator++(int)
                {
                    return ++*this;
                }

                bool equals(const Iterator<T> & rhs) const
                {
                    return row == rhs.row;
                }

            private:
                Reader::Row * row;
                T obj{};
                bool end_of_row_ = false;
            };

            template<typename T = std::string>
            class Range
            {
            public:
                auto begin()
                {
                    return Row::Iterator<T>(row);
                }

                auto end()
                {
                    return Row::Iterator<T>();
                }
            private:
                friend Row;
                Range(Row & row):row(row) {}
                Row & row;
            };

            template<typename T = std::string>
            auto begin()
            {
                return Row::Iterator<T>(*this);
            }

            template<typename T = std::string>
            auto end()
            {
                return Row::Iterator<T>();
            }

            template<typename T = std::string>
            auto range()
            {
                return Range<T>(*this);
            }

            template<typename T>
            Row & operator>>(T & data)
            {
                assert(reader);
                if(end_of_row())
                    throw Out_of_range_error("Read past end of row");

                data = reader->read_field<T>();
                end_of_row_ = reader->end_of_row();
                return * this;
            }

            template<typename T = std::string>
            auto read_field()
            {
                assert(reader);
                if(end_of_row())
                    throw Out_of_range_error("Read past end of row");

                auto field = reader->read_field<T>();
                end_of_row_ = reader->end_of_row();
                return field;
            }

            template<typename T = std::string>
            std::vector<T> read_vec()
            {
                std::vector<T> vec;
                std::copy(begin(), end(), std::back_inserter(vec));
                return vec;
            }

            template <typename ... Args>
            std::tuple<Args...> read_tuple()
            {
                std::tuple<Args...> ret;
                read_tuple_helper(ret, std::index_sequence_for<Args...>{});
                return ret;
            }

            template <typename ... Data>
            void read_v(Data & ... data)
            {
                (void)(*this >> ... >> data);
            }

            bool end_of_row() const { return end_of_row_; }

            operator bool() { return reader && !end_of_row(); }

        private:
            friend Reader;

            template <typename Tuple, std::size_t ... Is>
            void read_tuple_helper(Tuple & t, std::index_sequence<Is...>)
            {
                ((*this >> std::get<Is>(t)), ...);
            }

            Row(): reader(nullptr) {}
            explicit Row(Reader & reader): reader(&reader) {}

            Reader * reader;
            bool end_of_row_ = false;
        };

        class Iterator
        {
        public:
            using value_type        = Row;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const value_type*;
            using reference         = const value_type&;
            using iterator_category = std::input_iterator_tag;

            Iterator(): reader(nullptr)
            {}
            explicit Iterator(Reader & r): reader(&r)
            {
                obj = reader->get_row();

                if(reader->eof())
                    reader = nullptr;
            }

            const value_type & operator*() const { return obj; }
            const value_type * operator->() const { return &obj; }

            value_type & operator*() { return obj; }
            value_type * operator->() { return &obj; }

            Iterator & operator++()
            {
                while(!obj.end_of_row())
                    obj.read_field();

                assert(reader);
                obj = reader->get_row();

                if(reader->eof())
                    reader = nullptr;

                return *this;
            }
            Iterator & operator++(int)
            {
                return ++*this;
            }

            bool equals(const Iterator & rhs) const
            {
                return reader == rhs.reader;
            }

        private:
            Reader * reader;
            value_type obj;
        };

        explicit Reader(std::istream & input_stream,
                const char delimiter = ',', const char quote = '"'):
            input_stream_(&input_stream),
            delimiter_(delimiter),
            quote_(quote)
        {}

        explicit Reader(const std::string & filename,
                const char delimiter = ',', const char quote = '"'):
            internal_input_stream_(std::make_unique<std::ifstream>(filename)),
            input_stream_(internal_input_stream_.get()),
            delimiter_(delimiter),
            quote_(quote)
        {
            if(!(*internal_input_stream_))
                throw std::ios_base::failure{"Could not open file: " + filename};
        }

        // disambiguation tag
        struct input_string_t{};
        static inline constexpr input_string_t input_string{};

        Reader(input_string_t, const std::string & input_data,
                const char delimiter = ',', const char quote = '"'):
            internal_input_stream_(std::make_unique<std::istringstream>(input_data)),
            input_stream_(internal_input_stream_.get()),
            delimiter_(delimiter),
            quote_(quote)
        {}

        ~Reader() = default;

        Reader(const Reader &) = delete;
        Reader(Reader &&) = default;
        Reader & operator=(const Reader &) = delete;
        Reader & operator=(Reader &&) = default;

        bool end_of_row() const { return end_of_row_ || eof_; }
        bool eof()
        {
            if(eof_)
                return true;

            // consume empty rows
            while(true)
            {
                auto c = input_stream_->peek();
                if(!eof_ && c == std::istream::traits_type::eof())
                {
                    eof_ = true;
                    return true;
                }
                else if(c == '\r' || c == '\n')
                {
                    parse();
                }
                else
                {
                    return eof_;
                }
            }
        }

        operator bool() { return !eof(); }

        auto begin()
        {
            return Iterator(*this);
        }

        auto end()
        {
            return Iterator();
        }

        template<typename T>
        Reader & operator>>(T & data)
        {
            data = read_field<T>();
            return * this;
        }

        template<typename T = std::string>
        T read_field()
        {
            if(end_of_row_)
            {
                start_of_row_ = true;
                end_of_row_ = false;
            }

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

            if(eof_)
                throw Out_of_range_error("Read past end-of-file");

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

        template <typename T = std::string, typename OutputIter>
        bool read_row(OutputIter it)
        {
            while(true)
            {
                auto field = read_field<T>();
                *it++ = field;

                if(end_of_row())
                    break;
                else if(eof_)
                    return false;
            }
            return true;
        }

        Row get_row()
        {
            if(eof())
                return Row();
            else
                return Row(*this);
        }

        template <typename T = std::string>
        std::optional<std::vector<T>> read_row_vec()
        {
            if(eof())
                return {};

            std::vector<T> data;
            read_row<T>(std::back_inserter(data));
            return data;
        }

        template <typename ... Args>
        std::optional<std::tuple<Args...>> read_row_tuple()
        {
            if(eof())
                return {};

            std::tuple<Args...> ret;
            read_row_tuple_helper(ret, std::index_sequence_for<Args...>{});

            if(end_of_row())
            {
                start_of_row_ = true;
                end_of_row_ = false;
            }

            return ret;
        }

        template <typename ... Data>
        void read_row_v(Data & ... data)
        {
            (read_row_variadic_helper(data), ...);
            if(end_of_row())
            {
                start_of_row_ = true;
                end_of_row_ = false;
            }
        }

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
        // essentially the same as operator<<, but throws if caller tries to read beyond the end of a row
        template <typename T>
        void read_row_variadic_helper(T & t)
        {
            if(end_of_row())
            {
                start_of_row_ = true;
                end_of_row_ = false;
                throw Out_of_range_error("Read past end of row");
            }

            *this >> t;
        }

        template <typename Tuple, std::size_t ... Is>
        void read_row_tuple_helper(Tuple & t, std::index_sequence<Is...>)
        {
            (read_row_variadic_helper(std::get<Is>(t)), ...);
        }

        std::string parse()
        {
            if(input_stream_->eof())
            {
                end_of_row_ = true;
                eof_ = true;
                return {};
            }

            bool quoted = false;
            std::string field;

            while(true)
            {
                char c = '\0';
                input_stream_->get(c);
                if(c == '\n')
                {
                    ++line_no_;
                    col_no_ = 0;
                }
                else
                    ++col_no_;

                if(input_stream_->bad() && !input_stream_->eof())
                    throw Parse_error("Error reading from stream", line_no_, col_no_);

                // we need special handling for quotes
                if(c == quote_)
                {
                    if(quoted)
                    {
                        start_of_row_ = false;
                        input_stream_->get(c);
                        if(c == '\n')
                        {
                            ++line_no_;
                            col_no_ = 0;
                        }
                        else
                            ++col_no_;

                        // end of the field?
                        if(c == delimiter_ || c == '\n' || c == '\r' || input_stream_->eof())
                            quoted = false;
                        // if it's not an escaped quote, then it's an error
                        else if(c != quote_)
                            throw Parse_error("Unescaped double-quote", line_no_, col_no_ - 1);
                    }
                    else
                    {
                        if(field.empty())
                        {
                            start_of_row_ = false;
                            quoted = true;
                            continue;
                        }
                        else
                        {
                            // quotes are not allowed inside of an unquoted field
                            throw Parse_error("Double-quote found in unquoted field", line_no_, col_no_);
                        }
                    }
                }

                if(input_stream_->eof() && quoted)
                {
                    throw Parse_error("Unterminated quoted field - reached end-of-file", line_no_, col_no_);
                }
                else if(!quoted && c == delimiter_)
                {
                    break;
                }
                else if(!quoted && (c == '\n' || c == '\r' || input_stream_->eof()))
                {
                    end_of_row_ = true;
                    // consume newlines
                    while(input_stream_->good())
                    {
                        input_stream_->get(c);
                        if(c != '\r' && c != '\n')
                        {
                            if(input_stream_->good())
                                input_stream_->unget();
                            break;
                        }
                        if(c == '\n')
                        {
                            ++line_no_;
                            col_no_ = 0;
                        }
                        else
                            ++col_no_;
                    }
                    break;
                }

                start_of_row_ = false;
                field += c;
            }

            return field;
        }

        std::unique_ptr<std::istream> internal_input_stream_;
        std::istream * input_stream_;

        char delimiter_ {','};
        char quote_ {'"'};
        std::optional<std::string> line_terminator_ {};

        std::optional<std::string> conversion_retry_;
        bool start_of_row_ = true;
        bool end_of_row_ = false;
        bool eof_ = false;

        int line_no_ = 1;
        int col_no_ = 0;
    };

    inline bool operator==(const Reader::Iterator & lhs, const Reader::Iterator & rhs)
    {
        return lhs.equals(rhs);
    }

    inline bool operator!=(const Reader::Iterator & lhs, const Reader::Iterator & rhs)
    {
        return !lhs.equals(rhs);
    }

    template <typename T>
    inline bool operator==(const Reader::Row::Iterator<T> & lhs, const Reader::Row::Iterator<T> & rhs)
    {
        return lhs.equals(rhs);
    }

    template <typename T>
    inline bool operator!=(const Reader::Row::Iterator<T> & lhs, const Reader::Row::Iterator<T> & rhs)
    {
        return !lhs.equals(rhs);
    }

    template <typename Header = std::string, typename Value = std::string>
    class Map_reader_iter
    {
    private:

        std::unique_ptr<Reader> reader;
        Value default_val;
        std::vector<Header> headers;
        std::map<Header, Value> obj;

        auto get_header_row(const std::vector<Header> & headers)
        {
            if(!std::empty(headers))
                return headers;

            auto header_row = reader->read_row_vec<Header>();
            if(header_row)
                return *header_row;
            else
                throw Parse_error("Can't get header row", 0, 0);
        }

        template <typename Header2, typename Value2>
        friend class Map_reader_iter;

    public:
        Map_reader_iter() {}
        explicit Map_reader_iter(std::istream & input_stream, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"'):
            reader(std::make_unique<Reader>(input_stream, delimiter, quote)),
            default_val(default_val),
            headers(get_header_row(headers))
        {
            ++(*this);
        }

        explicit Map_reader_iter(const std::string & filename, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"'):
            reader(std::make_unique<Reader>(filename, delimiter, quote)),
            default_val(default_val),
            headers(get_header_row(headers))
        {
            ++(*this);
        }

        Map_reader_iter(Reader::input_string_t, const std::string & input_data, const Value & default_val = {}, const std::vector<Header> & headers = {},
                const char delimiter = ',', const char quote = '"'):
            reader(std::make_unique<Reader>(Reader::input_string, input_data, delimiter, quote)),
            default_val(default_val),
            headers(get_header_row(headers))
        {
            ++(*this);
        }

        using value_type        = decltype(obj);
        using difference_type   = std::ptrdiff_t;
        using pointer           = const value_type*;
        using reference         = const value_type&;
        using iterator_category = std::input_iterator_tag;

        const value_type & operator*() const { return obj; }
        const value_type * operator->() const { return &obj; }

        value_type & operator*() { return obj; }
        value_type * operator->() { return &obj; }

        typename value_type::mapped_type & operator[](const typename value_type::key_type & key) { return obj.at(key); }
        const typename value_type::mapped_type & operator[](const typename value_type::key_type & key) const { return obj.at(key); }

        Map_reader_iter & operator++()
        {
            auto row = reader->read_row_vec<Value>();
            if(!row)
                reader.reset();
            else
            {
                if(std::size(*row) > std::size(headers))
                    throw Out_of_range_error("Too many columns");

                for(std::size_t i = 0; i < std::size(*row); ++i)
                    obj[headers[i]] = (*row)[i];

                for(std::size_t i = std::size(*row); i < std::size(headers); ++i)
                    obj[headers[i]] = default_val;
            }

            return *this;
        }
        Map_reader_iter & operator++(int)
        {
            return ++*this;
        }

        template <typename Header2, typename Value2>
        bool equals(const Map_reader_iter<Header2, Value2> & rhs) const
        {
            return reader == rhs.reader;
        }

        auto get_headers() const
        {
            return headers;
        }
    };

    template <typename Header1, typename Value1, typename Header2, typename Value2>
    inline bool operator==(const Map_reader_iter<Header1, Value1> & lhs, const Map_reader_iter<Header2, Value2> & rhs)
    {
        return lhs.equals(rhs);
    }

    template <typename Header1, typename Value1, typename Header2, typename Value2>
    inline bool operator!=(const Map_reader_iter<Header1, Value1> & lhs, const Map_reader_iter<Header2, Value2> & rhs)
    {
        return !lhs.equals(rhs);
    }

    class Writer
    {
    public:
        class Iterator
        {
        public:
            using value_type        = void;
            using difference_type   = void;
            using pointer           = void;
            using reference         = void;
            using iterator_category = std::output_iterator_tag;

            explicit Iterator(Writer & w): writer(&w)
            {}

            Iterator & operator*() { return *this; }
            Iterator & operator++() { return *this; }
            Iterator & operator++(int) { return *this; }

            template <typename T>
            Iterator & operator=(const T & value)
            {
                writer->write_field(value);
                return *this;
            }

        private:
            Writer * writer;
        };

        explicit Writer(std::ostream & output_stream,
                const char delimiter = ',', const char quote = '"'):
            output_stream_(&output_stream),
            delimiter_(delimiter),
            quote_(quote)
        {}
        explicit Writer(const std::string& filename,
                const char delimiter = ',', const char quote = '"'):
            internal_output_stream_(std::make_unique<std::ofstream>(filename, std::ios::binary)),
            output_stream_(internal_output_stream_.get()),
            delimiter_(delimiter),
            quote_(quote)
        {
            if(!(*internal_output_stream_))
                throw std::ios_base::failure{"Could not open file: " + filename};
        }
        ~Writer()
        {
            if(!start_of_row_)
                end_row();
        }

        Writer(const Writer &) = delete;
        Writer(Writer &&) = default;
        Writer & operator=(const Writer &) = delete;
        Writer & operator=(Writer &&) = default;

        Iterator iterator()
        {
            return Iterator(*this);
        }

        template<typename Iter>
        void write_row(Iter first, Iter last)
        {
            if(!start_of_row_)
                end_row();

            bool first_item = true;

            for(; first != last; ++ first)
            {
                if(!first_item)
                    (*output_stream_)<<delimiter_;

                (*output_stream_)<<quote(*first);

                first_item = false;
            }
            end_row();
        }

        template<typename T>
        void write_row(const std::initializer_list<T> & data)
        {
            write_row(std::begin(data), std::end(data));
        }

        template<typename Range>
        void write_row(const Range & data)
        {
            write_row(std::begin(data), std::end(data));
        }

        template<typename ...Args>
        void write_row(const std::tuple<Args...> & data)
        {
            std::apply(&Writer::write_row_v<Args...>, std::tuple_cat(std::tuple(std::ref(*this)), data));
        }

        template<typename ...Data>
        void write_row_v(const Data & ...data)
        {
            (void)(*this << ... << data);
            end_row();
        }

        template<typename T>
        void write_field(const T & field)
        {
            if(!start_of_row_)
                (*output_stream_)<<delimiter_;

            (*output_stream_)<<quote(field);

            start_of_row_ = false;
        }

        template<typename T>
        Writer & operator<<(const T & field)
        {
            write_field(field);
            return *this;
        }

        Writer & operator<<(Writer & (*manip)(Writer &))
        {
            manip(*this);
            return *this;
        }

        void end_row()
        {
            (*output_stream_)<<"\r\n";
            start_of_row_ = true;
        }
    private:

        template<typename T>
        std::string quote(const T & field)
        {
            std::string field_str = str(field);

            auto escaped_chars = {delimiter_, quote_, '\r', '\n'};
            if(std::find_first_of(field_str.begin(), field_str.end(), escaped_chars.begin(), escaped_chars.end()) == field_str.end())
                return field_str;

            auto ret = std::regex_replace(field_str, std::regex(std::string{quote_}), std::string{quote_} + std::string{quote_});
            ret = quote_ + ret + quote_;
            return ret;
        }

        friend Writer &end_row(Writer & w);

        std::unique_ptr<std::ostream> internal_output_stream_;
        std::ostream * output_stream_;
        bool start_of_row_ = true;

        char delimiter_ {','};
        char quote_ {'"'};
    };

    inline Writer & end_row(Writer & w)
    {
        w.end_row();
        return w;
    }

    template <typename Header, typename Default_value = std::string>
    class Map_writer_iter
    {
    private:
        std::unique_ptr<Writer> writer;
        std::vector<Header> headers;
        Default_value default_val;

    public:
        Map_writer_iter(std::ostream & output_stream, const std::vector<Header> & headers, const Default_value & default_val = {},
                const char delimiter = ',', const char quote = '"'):
            writer(std::make_unique<Writer>(output_stream, delimiter, quote)), headers(headers), default_val(default_val)
        {
            writer->write_row(headers);
        }
        Map_writer_iter(const std::string& filename, const std::vector<Header> & headers, const Default_value & default_val = {},
                const char delimiter = ',', const char quote = '"'):
            writer(std::make_unique<Writer>(filename, delimiter, quote)), headers(headers), default_val(default_val)
        {
            writer->write_row(headers);
        }

        using value_type        = void;
        using difference_type   = void;
        using pointer           = void;
        using reference         = void;
        using iterator_category = std::output_iterator_tag;

        Map_writer_iter & operator*() { return *this; }
        Map_writer_iter & operator++() { return *this; }
        Map_writer_iter & operator++(int) { return *this; }

        template <typename K, typename T, typename std::enable_if_t<std::is_convertible_v<Header, K>, int> = 0>
        Map_writer_iter & operator=(const std::map<K, T> & row)
        {
            for(auto & h: headers)
            {
                try
                {
                    (*writer)<<row.at(h);
                }
                catch(std::out_of_range&)
                {
                    (*writer)<<default_val;
                }
            }

            writer->end_row();
            return *this;
        }

    };
};

#endif // CSV_HPP
