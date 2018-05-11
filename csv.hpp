#ifndef CSV_HPP
#define CSV_HPP

#include <exception>
#include <fstream>
#include <map>
#include <regex>
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

                    if(_end_of_row)
                    {
                        row = nullptr;
                    }
                    else
                    {
                        obj = row->read_field<T>();

                        if(row->end_of_row())
                            _end_of_row = true;
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
                bool _end_of_row = false;
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
                _end_of_row = reader->end_of_row();
                return * this;
            }

            template<typename T = std::string>
            auto read_field()
            {
                assert(reader);
                if(end_of_row())
                    throw Out_of_range_error("Read past end of row");

                auto field = reader->read_field<T>();
                _end_of_row = reader->end_of_row();
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
                read_tuple_helper<0, Args...>(ret);
                return ret;
            }

            template <typename T, typename ... Others>
            void read_v(T & store, Others & ... others)
            {
                store = read_field<T>();

                if constexpr(sizeof...(others) > 0)
                {
                    read_v(others...);
                }
                else
                {
                    reader->_start_of_row = true;
                    reader->_end_of_row = false;
                }
            }

            bool end_of_row() const { return _end_of_row; }

            operator bool() { return reader && !end_of_row(); }

        private:
            friend Reader;

            template <std::size_t I, typename ... Args>
            void read_tuple_helper(std::tuple<Args...> & ret)
            {
                std::get<I>(ret) = read_field<typename std::decay<decltype(std::get<I>(ret))>::type>();

                if constexpr(I < sizeof...(Args) - 1)
                {
                    read_tuple_helper<I + 1, Args...>(ret);
                }
            }

            Row(): reader(nullptr) {}
            explicit Row(Reader & reader): reader(&reader) {}

            Reader * reader;
            bool _end_of_row = false;
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

        explicit Reader(std::istream & input_stream):
            _input_stream(input_stream)
        {}

        explicit Reader(const std::string & input_data):
            _input_string(input_data),
            _input_stream(_input_string)
        {}

        ~Reader() = default;

        Reader(const Reader &) = delete;
        Reader(Reader &&) = default;
        Reader & operator=(const Reader &) = delete;
        Reader & operator=(Reader &&) = default;

        bool end_of_row() const { return _end_of_row || _eof; }
        bool eof()
        {
            if(_eof)
                return true;

            // consume empty rows
            while(true)
            {
                auto c = _input_stream.peek();
                if(!_eof && c == std::istream::traits_type::eof())
                {
                    _eof = true;
                    return true;
                }
                else if(c == '\r' || c == '\n')
                {
                    parse();
                }
                else
                {
                    return _eof;
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
            if(_end_of_row)
            {
                _start_of_row = true;
                _end_of_row = false;
            }

            std::string field;
            if(_conversion_retry)
            {
                field = *_conversion_retry;
                _conversion_retry.reset();
            }
            else
            {
                field = parse();
            }

            if(_eof)
                throw Out_of_range_error("Read past end-of-file");

            // no conversion needed for strings
            if constexpr(std::is_same<std::string, T>::value)
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
                    _conversion_retry = field;
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
                else if(_eof)
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
            read_row_tuple_helper<0, Args...>(ret);

            return ret;
        }

        template <typename T, typename ... Others>
        void read_row_v(T & store, Others & ... others)
        {
            if(end_of_row())
                store = {};
            else
                store = read_field<T>();

            if constexpr(sizeof...(others) > 0)
            {
                read_row_v(others...);
            }
            else
            {
                _start_of_row = true;
                _end_of_row = false;
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
        template <std::size_t I, typename ... Args>
        void read_row_tuple_helper(std::tuple<Args...> & ret)
        {
            if(end_of_row())
                std::get<I>(ret) = {};
            else
                std::get<I>(ret) = read_field<typename std::decay<decltype(std::get<I>(ret))>::type>();

            if constexpr(I < sizeof...(Args) - 1)
            {
                read_row_tuple_helper<I + 1, Args...>(ret);
            }
            else
            {
                if(end_of_row())
                {
                    _end_of_row = false;
                    _start_of_row = true;
                }
            }
        }

        std::string parse()
        {
            if(_input_stream.eof())
            {
                _end_of_row = true;
                _eof = true;
                return {};
            }

            bool quoted = false;
            std::string field;

            while(true)
            {
                char c = '\0';
                _input_stream.get(c);
                if(c == '\n')
                {
                    ++_line_no;
                    _col_no = 0;
                }
                else
                    ++_col_no;

                if(!_input_stream && !_input_stream.eof())
                    throw Parse_error("Error reading from stream", _line_no, _col_no);

                // we need special handling for quotes
                if(c == '"')
                {
                    if(quoted)
                    {
                        _start_of_row = false;
                        _input_stream.get(c);
                        if(c == '\n')
                        {
                            ++_line_no;
                            _col_no = 0;
                        }
                        else
                            ++_col_no;

                        // end of the field?
                        if(c == ',' || c == '\n' || c == '\r' || _input_stream.eof())
                            quoted = false;
                        // if it's not an escaped quote, then it's an error
                        else if(c != '"')
                            throw Parse_error("Unecaped double-quote", _line_no, _col_no - 1);
                    }
                    else
                    {
                        if(field.empty() && c == '"')
                        {
                            _start_of_row = false;
                            quoted = true;
                            continue;
                        }
                        else
                        {
                            // quotes are not allowed inside of an unquoted field
                            throw Parse_error("Double-quote found in unquoted field", _line_no, _col_no);
                        }
                    }
                }

                if(_input_stream.eof() && quoted)
                {
                    throw Parse_error("Unterminated quoted field - reached end-of-file", _line_no, _col_no);
                }
                else if(!quoted && c == ',')
                {
                    break;
                }
                else if(!quoted && (c == '\n' || c == '\r' || _input_stream.eof()))
                {
                    _end_of_row = true;
                    // consume newlines
                    while(_input_stream)
                    {
                        _input_stream.get(c);
                        if(c != '\r' && c != '\n')
                        {
                            if(_input_stream)
                                _input_stream.unget();
                            break;
                        }
                        if(c == '\n')
                        {
                            ++_line_no;
                            _col_no = 0;
                        }
                        else
                            ++_col_no;
                    }
                    break;
                }

                _start_of_row = false;
                field += c;
            }

            return field;
        }

        std::istringstream _input_string;
        std::istream & _input_stream;

        bool _start_of_row = true;
        bool _end_of_row = false;
        bool _eof = false;

        int _line_no = 1;
        int _col_no = 0;

        std::optional<std::string> _conversion_retry;
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

    class Map_reader_iter
    {
    private:

        std::unique_ptr<Reader> reader;
        std::string default_val;
        std::vector<std::string> headers;
        std::map<std::string, std::string> obj;

        auto get_header_row(const std::vector<std::string> & headers)
        {
            if(!std::empty(headers))
                return headers;

            auto header_row = reader->read_row_vec();
            if(header_row)
                return *header_row;
            else
                throw std::runtime_error("can't get header row");
        }

    public:
        Map_reader_iter() {}
        Map_reader_iter(std::istream & input_stream, const std::string & default_val = {}, const std::vector<std::string> & headers = {}):
            reader(std::make_unique<Reader>(input_stream)),
            default_val(default_val),
            headers(get_header_row(headers))
        {
            ++(*this);
        }
        Map_reader_iter(const std::string & input_data, const std::string & default_val = {}, const std::vector<std::string> & headers = {}):
            reader(std::make_unique<Reader>(input_data)),
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

        Map_reader_iter & operator++()
        {
            auto row = reader->read_row_vec();
            if(!row)
                reader.reset();
            else
            {
                if(std::size(*row) > std::size(headers))
                    throw std::length_error("Too many columns");

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

        bool equals(const Map_reader_iter & rhs) const
        {
            return reader == rhs.reader;
        }

        auto get_headers() const
        {
            return headers;
        }
    };

    inline bool operator==(const Map_reader_iter & lhs, const Map_reader_iter & rhs)
    {
        return lhs.equals(rhs);
    }

    inline bool operator!=(const Map_reader_iter & lhs, const Map_reader_iter & rhs)
    {
        return !lhs.equals(rhs);
    }

    namespace detail
    {
        using std::to_string;
        template<typename T>
        class can_to_string
        {
            template<typename TT>
            static auto test(int) -> decltype(to_string(std::declval<TT>()), std::true_type());
            template<typename>
            static auto test(...) -> std::false_type;
        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };

        template<typename S, typename T>
        class can_ostream
        {
            template<typename SS, typename TT>
            static auto test(int) -> decltype(std::declval<SS&>() << std::declval<TT>(), std::true_type());
            template<typename, typename>
            static auto test(...) -> std::false_type;
        public:
            static constexpr bool value = decltype(test<S, T>(0))::value;
        };
    };

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

        explicit Writer(std::ostream & output_stream):
            _output_stream(output_stream)
        {}
        ~Writer()
        {
            if(!_start_of_row)
                end_row();
        }

        Writer(const Writer &) = delete;
        Writer(Writer &&) = delete;
        Writer & operator=(const Writer &) = default;
        Writer & operator=(Writer &&) = default;

        Iterator iterator()
        {
            return Iterator(*this);
        }

        template<typename Iter>
        void write_row(Iter first, Iter last)
        {
            if(!_start_of_row)
                end_row();

            auto no_comma = "";
            auto comma = ",";
            auto & sep = no_comma;

            for(; first != last; ++ first)
            {
                _output_stream<<sep<<quote(*first);
                sep = comma;
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

        template<typename T, typename ...Others>
        void write_row_v(const T & data, const Others & ...others)
        {
            write_field(data);
            if constexpr(sizeof...(others) > 0)
            {
                write_row_v(others...);
            }
            else
                end_row();
        }

        template<typename T>
        void write_field(const T & field)
        {
            if(!_start_of_row)
                _output_stream<<",";

            _output_stream<<quote(field);

            _start_of_row = false;
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
            _output_stream<<"\r\n";
            _start_of_row = true;
        }
    private:

        template<typename T>
        std::string quote(const T & field)
        {
            std::string field_str;
            using std::to_string;

            static_assert(std::disjunction_v<std::is_convertible<T, std::string>, detail::can_ostream<std::ostringstream, T>, detail::can_to_string<T>>, "Can't convert to string for output");

            if constexpr(std::is_convertible_v<T, std::string>)
            {
                field_str = field;
            }
            else if constexpr(detail::can_ostream<std::ostringstream, T>::value)
            {
                std::ostringstream oss;
                oss<<field;
                field_str = oss.str();
            }
            else if constexpr(detail::can_to_string<T>::value)
            {
                field_str = to_string(field);
            }

            auto escaped_chars = {'"', '\r', '\n', ','};
            if(std::find_first_of(field_str.begin(), field_str.end(), escaped_chars.begin(), escaped_chars.end()) == field_str.end())
                return field_str;

            auto ret = std::regex_replace(field_str, std::regex("\""), "\"\"");
            ret = "\"" + ret + "\"";
            return ret;
        }

        friend Writer &end_row(Writer & w);

        std::ostream & _output_stream;
        bool _start_of_row = true;
    };

    inline Writer & end_row(Writer & w)
    {
        w.end_row();
        return w;
    }

    class Map_writer_iter
    {
    private:
        std::unique_ptr<Writer> writer;
        std::vector<std::string> headers;
        std::string default_val;

    public:
        Map_writer_iter(std::ostream & output_stream, const std::vector<std::string> & headers, const std::string default_val = {}):
            writer(std::make_unique<Writer>(output_stream)), headers(headers), default_val(default_val)
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

        template <typename T>
        Map_writer_iter & operator=(const std::map<std::string, T> & row)
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
