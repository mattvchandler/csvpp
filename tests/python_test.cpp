#include "python_test.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/stl.h>

// python allows fields w/ unescaped quotes. use this to pre-parse and filter those inputs out (this is essentially a stripped down csv::Reader::parse)
bool python_too_lenient(const std::string & csv, const char delimiter, const char quote)
{
    bool empty = true;
    bool quoted = false;
    for(auto i = std::begin(csv);; ++i)
    {
        if(i != std::end(csv) && *i == quote)
        {
            if(quoted)
            {
                ++i;
                if(i == std::end(csv) || *i == delimiter || *i == '\n' || *i == '\r')
                    quoted = false;
                else if(*i != quote)
                    return false;
            }
            else if(empty)
            {
                quoted = true;
                continue;
            }
            else
                return true;
        }
        if(i == std::end(csv) && quoted)
        {
            return false;
        }
        else if(!quoted && i != std::end(csv) && *i == delimiter)
        {
            empty = true;
            continue;
        }
        else if(!quoted && (i == std::end(csv) || *i == '\n' || *i == '\r'))
        {
            empty = true;
            for(;i != std::end(csv) && *i != '\r' && *i != '\n'; ++i);

            if(i == std::end(csv))
                return false;

            continue;
        }
        empty = false;
    }
}

auto convert_result(const pybind11::object & pyresult)
{
    auto result = pyresult.cast<int>();
    switch(result)
    {
    case 0: return test::fail();
    case 1: return test::pass();
    case 2: return test::skip();
    default: throw std::runtime_error("unknown python Result value: " + std::to_string(result));
    }
}

void handle_parse_error(const char * what) noexcept
{
    for(const char * c = what; *c && *c != '\n'; ++c)
        std::cout<<*c;
    std::cout<<'\n';
};

auto test_read_python(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    // Python's parser is very lenient even when Strict is enabled, and handles some quoting errors very strangely when not Strict. Rather than dealing with that, just skip them
    if(lenient || python_too_lenient(csv_text, delimiter, quote))
        return test::skip();

    const char * test_read_python_code = R"(
def test_read_python(csv_text, expected_data, delimiter, quote):
    infile = io.StringIO(csv_text, newline='')
    r = csv.reader(infile, delimiter = delimiter, quotechar = quote, strict=True)

    try:
        data = [row for row in r if row]
        return Result.PASS if data == expected_data else Result.FAIL
    except csv.Error as e:
        raise Parse_error(r.line_num, e)
    )";
    pybind11::exec(test_read_python_code);
    auto test_read_python_fun = pybind11::globals()["test_read_python"];

    try
    {
        return convert_result(test_read_python_fun(csv_text, expected_data, delimiter, quote));
    }
    catch(pybind11::error_already_set & e)
    {
        if(e.matches(pybind11::globals()["Parse_error"]))
        {
            handle_parse_error(e.what());
            return test::error();
        }
        else
            throw;
    }
};

auto test_read_python_map(const std::string & csv_text, const CSV_data & expected_data, const char delimiter, const char quote, const bool lenient)
{
    if(lenient || python_too_lenient(csv_text, delimiter, quote))
        return test::skip();

    const char * test_read_python_code = R"(
def test_read_python_map(csv_text, expected_data, delimiter, quote):
    infile = io.StringIO(csv_text, newline='')
    try:
        r = csv.DictReader(infile, delimiter = delimiter, quotechar = quote, strict=True)

        if not r.fieldnames and not expected_data:
            return Result.PASS

        headers = expected_data[0]
        if r.fieldnames != headers:
            return Result.FAIL

        i = 1
        for row in r:
            if i >= len(expected_data):
                return Result.FAIL

            if len(expected_data[i]) != len(headers):
                return Result.SKIP

            if row != collections.OrderedDict((headers[j], expected_data[i][j]) for j in range(len(headers))):
                return Result.FAIL
            i+=1

        if i != len(expected_data) or next(r, None) is not None:
            return Result.FAIL

    except csv.Error as e:
        raise Parse_error(r.line_num, e)

    return Result.PASS
    )";
    pybind11::exec(test_read_python_code);
    auto test_read_python_map_fun = pybind11::globals()["test_read_python_map"];

    try
    {
        return convert_result(test_read_python_map_fun(csv_text, expected_data, delimiter, quote));
    }
    catch(pybind11::error_already_set & e)
    {
        if(e.matches(pybind11::globals()["Parse_error"]))
        {
            handle_parse_error(e.what());
            return test::error();
        }
        else
            throw;
    }
};

auto test_write_python(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    const char * test_write_python_code = R"(
def test_write_python(expected_text, data, delimiter, quote):
    out = io.StringIO(newline='')
    w = csv.writer(out, delimiter=delimiter, quotechar=quote)
    for row in data:
        w.writerow(row)
    return Result.PASS if out.getvalue() == expected_text else Result.FAIL
    )";
    pybind11::exec(test_write_python_code);
    auto test_write_python_fun = pybind11::globals()["test_write_python"];
    return convert_result(test_write_python_fun(expected_text, data, delimiter, quote));
};

auto test_write_python_map(const std::string & expected_text, const CSV_data & data, const char delimiter, const char quote)
{
    const char * test_write_python_code = R"(
def test_write_python_map(expected_text, data, delimiter, quote):
    out = io.StringIO(newline='')
    if data:
        headers = data[0]

        w = csv.DictWriter(out, headers, delimiter=delimiter, quotechar=quote)
        w.writeheader()

        for row in data[1:]:
            if len(row) != len(headers):
                return Result.SKIP
            w.writerow({ headers[i]: row[i] for i in range(len(row)) })

    return Result.PASS if out.getvalue() == expected_text else Result.FAIL
    )";
    pybind11::exec(test_write_python_code);
    auto test_write_python_map_fun = pybind11::globals()["test_write_python_map"];
    return convert_result(test_write_python_map_fun(expected_text, data, delimiter, quote));
};

Python_test::Python_test()
{
    if(interpreter_ref_count++ == 0)
    {
        pybind11::initialize_interpreter();

        pybind11::exec(R"(
            import io, collections, csv, enum

            class Parse_error(Exception):
                def __init__(self, line_num, msg):
                    super().__init__("Python CSV error on line {}: {}".format(line_num, msg))

            class Result(enum.IntEnum):
                FAIL  = 0
                PASS  = 1
                SKIP  = 2
            )");
    }
}

Python_test::~Python_test()
{
    if(--interpreter_ref_count == 0)
        pybind11::finalize_interpreter();
}

Python_test::Python_test(const Python_test &)
{
    ++interpreter_ref_count;
}

Python_test & Python_test::operator=(const Python_test & other)
{
    if(&other != this)
        ++interpreter_ref_count;
    return *this;
}

Python_test::Python_test(Python_test &&)
{
    ++interpreter_ref_count;
}

Python_test & Python_test::operator=(Python_test && other)
{
    if(&other != this)
        ++interpreter_ref_count;
    return *this;
}

void Python_test::register_tests(CSV_test_suite & tests) const
{
    tests.register_read_test(test_read_python);
    tests.register_read_test(test_read_python_map);

    tests.register_write_test(test_write_python);
    tests.register_write_test(test_write_python_map);
}

std::size_t Python_test::interpreter_ref_count {0};
