#!/usr/bin/env python3

# Copyright 2020 Matthew Chandler
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import argparse, csv, sys

p = argparse.ArgumentParser(description = "Pretty-Print CSV files")

p.add_argument("--delimiter", "-d", metavar = "DELIMITER", default = ',',
    help = "delimiter character")
p.add_argument("--quote", "-q", metavar = "QUOTE", default = '"',
    help = "quote character")
p.add_argument("--lenient", "-l", action="store_true",
    help = "ignore parsing errors")
p.add_argument("filename",
    help = "CSV input file to display")

args = p.parse_args()

try:
    if not args.filename or args.filename == "-":
        input_file = sys.stdin
    else:
        input_file = open(args.filename, "r", newline="")

    input_csv = csv.reader(input_file, delimiter=args.delimiter, quotechar=args.quote, strict=(not args.lenient))

    data = []
    col_size = []

    for row in input_csv:
        data.append([])

        for i, field in enumerate(row):

            if i == len(col_size):
                col_size.append(len(field))
            else:
                col_size[i] = max(col_size[i], len(field))

            data[-1].append(field)

    for row in data:
        print(" | ".join(f"{field:{col_size[i]}}" for i, field in enumerate(row)))

except csv.Error as e:
    print(e, file=sys.stderr)
    sys.exit(1)

except OSError as e:
    print("I/O error:", e, file=sys.stderr)
    sys.exit(1)
