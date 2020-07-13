# csvpp

### [csvpp on Github](https://github.com/mattvchandler/csvpp)

### [API Documentation](https://mattvchandler.github.io/csvpp/index.html)

csvpp is a collection of RFC 4180 compliant CSV readers and writers. All 3
modules are totally independent of each other, so feel free to use whichever
best suits you application

## csv.hpp - A c++ header-only library

#### Reader
* Read in rows of data as std::vectors or std::tuples, a stream, field-by-field,
iterators, or with variadic argmuents
* Template type conversion available for all of the above (when reading a
vector, the whole row will be converted to the same type)

Some example usages:

```cpp
#include <csvpp/csh.hpp>

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
My_type c; // can auto-convert any type that has an std::ostream & operator>>(std::ostream &)

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
My_type field 3 = …";

mycsv << field1 << field2 << field3 << csv::end_row;

mycsv_write_row_v(field1, field2, field3);
```

### Map_reader_iter / Map_writer_iter

* Read / Write rows as std::maps with column headers as keys
Some example usages:

```cpp
csv::Map_reader_iter mycsv{csv::Reader::input_string, "Col1,Col2,Col3\nA,B,C"};

for(auto && row: mycsv)
{
    assert(row["Col1"] == "A");
}

csv::Map_writer_iter<std::string, int> mycsv2{"mycsv.csv", {"Col1, "Col2", Col3"}};
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
while((row = CSV_reader_read_row(mycsv))
{
    size_t num_fields = CSV_row_size(row);

    if(num_fields >= 1)
    {
        char * field = NULL;
        CSV_strdup(field, CSV_row.get(row, 0));

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

CSV_reader * mycsv2 = CSV_reader_init_from_string("A,B,C\nD,E,F\n");
CSV_row * row = NULL;

while(1)
{
    char * field = CSV_reader_read_field(mycsv2);
    if(!field)
    {
        if(CSV_reader_get_error() != CSV_EOF)
            fprintf(stderr, "Error: %s\n", CSV_reader_get_error_msg())
        break;
    }
    if(CSV_end_of_row())
        my_process_end_of_row();
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

CSV_row_append(row, "field1");
CSV_row_append(row, "field2");
CSV_row_append(row, "field3");

CSV_writer_write_row(mycsv, row);

CSV_row_free(row);

/******************************************************************************/

CSV_writer_write_row_v("A", "B", "C", NULL);

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

while(1)
{
    char c = my_wait_for_serial_byte();
    char * field_out = NULL;
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
```
