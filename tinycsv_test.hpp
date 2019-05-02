#ifndef TINYCSV_TEST_HPP
#define TINYCSV_TEST_HPP

#include <string>
#include <utility>

#include <cstdio>

std::pair<std::string, int> tinycsv_parse(FILE * f);
std::pair<std::string, int> tinycsv_expanded_parse(FILE * f);

#endif // TINYCSV_TEST_HPP
