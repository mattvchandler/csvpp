# csvpp

[csvpp on Github](https://github.com/mattvchandler/csvpp)

[API Documentation](https://mattvchandler.github.io/csvpp/index.html)

csvpp is a collection of RFC 4180 compliant CSV readers and writers. All 3
modules are totally independent of each other, so feel free to use whichever
best suits you application

## csv.hpp - A c++ header-only library

### Reader
* Read in rows of data as std::vectors or std::tuples, a stream, field-by-field,
iterators, or with variadic argmuents
* Template type conversion available for all of the above (when reading a
vector, the whole row will be converted to the same type)

Some example usages:

```cpp
#include <csvpp/csv.hpp>

…

std::ifstream mycsv_file{"mycsv.csv"};
csv::Reader mycsv{mycsv_file}; // open a std::istream

for(auto && row: mycsv) // row is a csv::Row object
{
    for(auto && column: row) // column is a std::string. To get another type,
                             // use Row::Range. For example:
                             // for(auto && column: row.range<int>())
    {
        // process columns
    }
}

/******************************************************************************/

csv::Reader mycsv2{"mycsv2.csv"}; // open by filename

auto all_data = mycsv2.read_all<int>(); // returns a std::vector<std::vector<T>>

/******************************************************************************/

csv::Reader mycsv3{csv::Reader::input_string, "A,B,C\nD,E,F\nG,H,I"}; // read from a string

char a;
std::string b;
My_type c; // can auto-convert any type T that has an std::ostream & operator>>(std::ostream &, const T &)

auto row = mycsv3.get_row();
row >> a >> b >> c;

…

row = mycsv3.get_row();
row.read_v(a, b, c);

…

row = mycsv3.get_row();
auto row_tuple = row.read_tuple<char, std::string, My_type>();
```

### Writer

* Write data as vectors, tuples, a stream, field-by-field, iterators, or
variadicaly

Some example usages:

```cpp
std::ofstream mycsv_file{"mycsv.csv"};
csv::Writer mycsv{mycsv_file};

std::vector row = …;
mycsv.write_row(std::begin(row), std::end(row));
mycsv.write_row(row); // ranges supported too

/******************************************************************************/

int field1 = …;
std::string field2 = …;
My_type field3 = …";

mycsv << field1 << field2 << field3 << csv::end_row;

mycsv.write_row_v(field1, field2, field3);
```

### Map_reader_iter / Map_writer_iter

* Read / Write rows as std::maps with column headers as keys
Some example usages:

```cpp
for(auto row = csv::Map_reader_iter{csv::Reader::input_string, "Col1,Col2,Col3\nA,B,C"}; row != csv::Map_reader_iter{}; ++row)
{
    assert((*row)["Col1"] == "A");
    for(auto && [k,v]: *row)
    {
        std::cout<<k<<": "<<v<<"\n";
    }
}

csv::Map_writer_iter<std::string, int> mycsv{"mycsv4.csv", {"Col1", "Col2", "Col3"}};
std::map<std::string, int> row;
row["Col1"] = 1;
row["Col2"] = 2;
row["Col3"] = 3;
*mycsv++ = row;
```

## csv.h - A C CSV library

### CSV_reader

Some example usages:

```C

#include <csvpp/csv.h>

…

CSV_reader * mycsv = CSV_reader_init_from_filename("mycsv.csv");
CSV_row * row = NULL;
while((row = CSV_reader_read_row(mycsv)))
{
    size_t num_fields = CSV_row_size(row);

    if(num_fields >= 1)
    {
        char * field = CSV_strdup(CSV_row_get(row, 0));

        // Process field;
        free(field);
    }

    // get read-only array of strings for the row:
    const char * const * arr = CSV_row_arr(row);
    my_process_row(row);

    CSV_row_free(row);
}

CSV_reader_free(mycsv);

/******************************************************************************/

CSV_reader * mycsv2 = CSV_reader_init_from_str("A,B,C\nD,E,F\n");

while(1)
{
    char * field = CSV_reader_read_field(mycsv2);
    if(!field)
    {
        if(CSV_reader_get_error(mycsv2) != CSV_EOF)
            fprintf(stderr, "Error: %s\n", CSV_reader_get_error_msg(mycsv2));

        break;
    }

    // Process field

    free(field);

    if(CSV_reader_end_of_row(mycsv2))
        // Process end of row
}

CSV_reader_free(mycsv2);
```
### CSV_writer

Some example usages:

```C

#include <csvpp/csv.h>

…

CSV_writer * mycsv = CSV_writer_init_from_filename("mycsv.csv");

CSV_row * row = CSV_row_init();

CSV_row_append(row, CSV_strdup("field1"));
CSV_row_append(row, CSV_strdup("field2"));
CSV_row_append(row, CSV_strdup("field3"));

CSV_writer_write_row(mycsv, row);

CSV_row_free(row);

/******************************************************************************/

CSV_writer_write_row_v(mycsv, "A", "B", "C", NULL);

/******************************************************************************/

CSV_writer_write_field(mycsv, "D");
CSV_writer_write_field(mycsv, "E");
CSV_writer_write_field(mycsv, "F");
CSV_writer_end_row(mycsv);

/******************************************************************************/

char * data[10];
…

CSV_writer_write_row_ptr(mycsv, data, 10);

CSV_writer_free(mycsv);
```

## embcsv.h - A stripped-down CSV reader ideal for embedded environments

* Allows reading input character-by-character, so unbuffered input may be read
* Compile with EMBCSV_NO_MALLOC to prevent heap memory allocation

Some example usages:

```C
#include <csvpp/embcsv.h>

…

EMBCSV_reader * mycsv = EMBCSV_reader_init();

while(!my_done_reading_serial())
{
    char c = my_wait_for_serial_byte();
    const char * field_out = NULL;
    EMBCSV_result r = EMBCSV_reader_parse_char(mycsv, c, &field_out);

    if(r == EMBCSV_PARSE_ERROR)
        my_handle_error();
    else if(r == EMBCSV_FIELD || r == EMBCSV_END_OF_ROW)
    {
        my_process_field(field);
        if(r == EMBCSV_END_OF_ROW)
            my_process_end_of_row();
    }
}

EMBCSV_reader_free(mycsv);
```

## Compiling & Installation

Requirements:
* CMake
* A C++ compiler with C++17 support
* A C compiler with C11 support
* No external libraries required!

Installation is not strictly required. You can directly include the relevant
files into your own project:

* C++ library
    * include/csvpp/csv.hpp
* C library
    * include/csvpp/csv.h
    * src/csv.c
* Embedded C library
    * include/csvpp/embcsv.h
    * src/embcsv.c

To build and install:

```
mkdir build
cd build
cmake .. <build option arguments>
make

# installation
make install
# OR
cpack
```

Build options

* `-DCSVPP_ENABLE_CPP=1` - Enable C++ library. Enabled by default
* `-DCSVPP_ENABLE_C=1` - Enable C library.
* `-DCSVPP_ENABLE_EMBEDDED=1` - Enable embedded C library.
* `-DCSVPP_ENABLE_ALL=1` - Enable all libraries
* `-DCSVPP_EMBEDDED_NO_MALLOC=1` - Disable heap allocation for embedded library
* `-DCSVPP_ENABLE_EXAMPLES=1` - Enable example utility programs
* `-DCSVPP_INTERNAL_DOCS=1` - Include private method documentation for doc
   target
* `-DBUILD_TESTING=1` - Enable tests

### Including libraries in your project

If installed, pkg-config and CMake files will be installed.

To include libraries with pkg-config:

* C++ library
    `pkg-config --cflags csvpp`
* C library
    `pkg-config --cflags --libs csvpp-c`
* Embedded C library
    `pkg-config --cflags --libs csvpp-embcsv`

To include libraries with CMake:

```CMake
find_package(csvpp)

add_executable(myproj …)
target_link_libraries(myproj … csvpp::csvpp) # for C++ library
target_link_libraries(myproj … csvpp::csv) # for C library
target_link_libraries(myproj … csvpp::embcsv) # for embedded C library
```

Or, if you have included the csvpp as a submodule of your project:

```CMake
add_subdirectory(csvpp)
add_executable(myproj …)
target_link_libraries(myproj … csvpp::csvpp) # for C++ library
target_link_libraries(myproj … csvpp::csv) # for C library
target_link_libraries(myproj … csvpp::embcsv) # for embedded C library
```

### Testing

To run test suite, generate with `-DBUILD_TESTING=1` and `make test`

### Documentation

Documentation can be built with doxygen (if installed). `make doc`
