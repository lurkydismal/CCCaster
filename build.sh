#!/bin/sh
SCRIPT_DIRECTORY=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

clear && \
mkdir -p icecream && \
cd icecream && \
curl -O https://raw.githubusercontent.com/renatoGarcia/icecream-cpp/master/icecream.hpp && \
cd .. && \
mkdir -p nlohmann && \
cd nlohmann && \
curl -O https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp && \
cd .. && \
mkdir -p fmt && \
cd fmt && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/args.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/base.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/chrono.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/color.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/compile.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/core.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/format-inl.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/format.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/os.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/ostream.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/printf.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/ranges.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/std.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/xchar.h && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/src/fmt.cc && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/src/format.cc && \
curl -O https://raw.githubusercontent.com/fmtlib/fmt/master/src/os.cc && \
cd ..
