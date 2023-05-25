#!/bin/bash

SRC=$(dirname "$(realpath "$0")")

# All C source and header files in the root
C_SRC="$(find "$SRC" | grep -E ".*(\.ino|\.c|\.h)$")"

# Run clang-format using config from `$SRC/.clang-format`
clang-format --verbose -i --style=file $C_SRC
