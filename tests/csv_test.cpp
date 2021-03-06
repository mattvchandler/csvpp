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

#ifdef CSVPP_ENABLE_CPP
#include "cpp_test.hpp"
#endif

#ifdef CSVPP_ENABLE_C
#include "c_test.hpp"
#endif

#ifdef CSVPP_ENABLE_EMBEDDED
#include "embcsv_test.hpp"
#endif

#include "csv_test_suite.hpp"

int main(int, char *[])
{
    CSV_test_suite tests;

    #ifdef CSVPP_ENABLE_EMBEDDED
    tests.register_tests(Embcsv_test{});
    #endif

    #ifdef CSVPP_ENABLE_C
    tests.register_tests(C_test{});
    #endif

    #ifdef CSVPP_ENABLE_CPP
    tests.register_tests(Cpp_test{});
    #endif

    return tests.run_tests() ? EXIT_SUCCESS : EXIT_FAILURE;
}
