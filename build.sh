#!/bin/bash
SCRIPT_DIRECTORY=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

clear && \
# mkdir -p icecream && \
# cd icecream && \
# curl -o icecream.hpp https://raw.githubusercontent.com/renatoGarcia/icecream-cpp/master/icecream.hpp && \
# cd .. && \
mkdir -p nlohmann && \
cd nlohmann && \
curl -o json.hpp https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp && \
cd .. && \
# mkdir -p imHotKey && \
# cd imHotKey && \
# curl -o imHotKey.h https://raw.githubusercontent.com/CedricGuillemet/ImHotKey/master/imHotKey.h && \
# cd .. && \
# mkdir -p fmt && \
# cd fmt && \
# curl -o args.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/args.h && \
# curl -o base.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/base.h && \
# curl -o chrono.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/chrono.h && \
# curl -o color.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/color.h && \
# curl -o compile.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/compile.h && \
# curl -o core.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/core.h && \
# curl -o format-inl.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/format-inl.h && \
# curl -o format.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/format.h && \
# curl -o os.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/os.h && \
# curl -o ostream.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/ostream.h && \
# curl -o printf.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/printf.h && \
# curl -o ranges.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/ranges.h && \
# curl -o std.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/std.h && \
# curl -o xchar.h https://raw.githubusercontent.com/fmtlib/fmt/master/include/fmt/xchar.h && \
# curl -o fmt.cc https://raw.githubusercontent.com/fmtlib/fmt/master/src/fmt.cc && \
# curl -o format.cc https://raw.githubusercontent.com/fmtlib/fmt/master/src/format.cc && \
# curl -o os.cc https://raw.githubusercontent.com/fmtlib/fmt/master/src/os.cc && \
# cd .. && \
