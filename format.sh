#!/usr/bin/env sh
FILE_LIST="$(find include src -type f -name '*.cpp' -o -name '*.hpp')"
clang-format-8 -i -style=file $FILE_LIST
