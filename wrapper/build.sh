#!/bin/bash

set -e

print_help() {
    cat <<EOF
Usage: $0 -w [-r | -d] [-h]

Options:
  -w    Build for Windows
  -r    Build in release mode
  -d    Build in debug mode (default if neither specified)
  -h    Show this help message

Notes:
  -w is required.
  -r and -d are mutually exclusive.
EOF
}

l_release=0
l_debug=0
l_windows=0

while getopts ":rdwh" opt; do
    case "$opt" in
    r) l_release=1 ;;
    d) l_debug=1 ;;
    w) l_windows=1 ;;
    h)
        print_help
        exit 0
        ;;
    :)
        echo "Error: option -$OPTARG requires an argument"
        exit 1
        ;;
    \?)
        echo "Error: invalid option -$OPTARG"
        print_help
        exit 1
        ;;
    esac
done

# Validate platform flags
if ((!l_windows)); then
    echo "Error: one of -w must be specified"
    print_help
    exit 1
fi

# Validate build type flags
if ((l_release && l_debug)); then
    echo "Error: -r and -d cannot be used together"
    exit 1
fi

# Build mode
l_build_flags=""
l_link_flags=""
if ((l_release)); then
    l_build_flags="-O3 -ffast-math -funroll-loops -s"
    l_link_flags="-Wl,-O1"

else
    l_build_flags="-Og -g"
    l_link_flags=""
fi

# Target selection
l_compiler=""
if ((l_windows)); then
    l_compiler="wineg++"
fi

l_build_dir="out"
mkdir -p "$l_build_dir"

shopt -s nullglob
l_sources=(./src/*.cpp)
shopt -u nullglob

if ((${#l_sources[@]} == 0)); then
    echo "Error: no source files found in ./src"
    exit 1
fi

l_objects=()
for l_source in "${l_sources[@]}"; do
    l_base="$(basename "${l_source%.cpp}")"
    l_object="$l_build_dir/${l_base}.o"
    l_objects+=("$l_object")

    ccache "$l_compiler" \
        -m32 -shared \
        -std=gnu++26 \
        -pipe -fuse-ld=mold -march=native \
        -ffunction-sections -fdata-sections \
        -fPIC -fopenmp-simd \
        -fno-ident -fno-short-enums \
        -Wall -Wextra \
        $l_build_flags \
        -fno-asynchronous-unwind-tables \
        -fno-rtti -fno-exceptions \
        -fno-threadsafe-statics \
        -fno-unwind-tables \
        -fPIC \
        -Wl,--gc-sections \
        -s -Wl,--no-eh-frame-hdr \
        $l_link_flags \
        -I include \
        -c "$l_source" \
        -o "$l_object"
done

"$l_compiler" \
    "${l_objects[@]}" \
    -m32 -shared \
    -std=gnu++26 \
    -pipe -fuse-ld=mold -march=native \
    -ffunction-sections -fdata-sections \
    -fPIC -fopenmp-simd \
    -fno-ident -fno-short-enums \
    -Wall -Wextra \
    $l_build_flags \
    -fno-asynchronous-unwind-tables \
    -fno-rtti -fno-exceptions \
    -fno-threadsafe-statics \
    -fno-unwind-tables \
    -fPIC \
    -Wl,--gc-sections \
    -s -Wl,--no-eh-frame-hdr \
    $l_link_flags \
    -o wrapper.dll
