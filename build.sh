#!/bin/sh
SCRIPT_DIRECTORY=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

clear && \
mkdir -p nlohmann && \
cd nlohmann && \
curl -O https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp && \
cd ..
