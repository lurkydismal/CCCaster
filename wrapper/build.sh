#!/bin/bash

set -e

print_help() {
    cat <<EOF
Usage: $0 -l [-r | -d] [-h]

Options:
  -l    Build for Linux (native target)
  -r    Build in release mode
  -d    Build in debug mode (default if neither specified)
  -h    Show this help message

Notes:
  -l is required.
  -r and -d are mutually exclusive.
EOF
}

l_release=0
l_debug=0
l_linux=0

while getopts ":rdlh" opt; do
    case "$opt" in
    r) l_release=1 ;;
    d) l_debug=1 ;;
    l) l_linux=1 ;;
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

SCRIPT_DIRECTORY=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

# Validate platform flags
if ((!l_linux)); then
    echo "Error: one of -l must be specified"
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
if ((l_linux)); then
    l_compiler="wineg++"
fi

l_build_dir="out"
mkdir -p "$l_build_dir"

shopt -s nullglob
mapfile -t l_sources < <(find "$SCRIPT_DIRECTORY/src" -type f -name "*.cpp" | sort)
shopt -u nullglob

if ((${#l_sources[@]} == 0)); then
    echo "Error: no source files found in src/"
    exit 1
fi

l_total=${#l_sources[@]}
l_index=1

l_objects=()
for l_source in "${l_sources[@]}"; do
    l_base="$(basename "${l_source%.cpp}")"
    l_object="$l_build_dir/${l_base}.o"
    l_objects+=("$l_object")

    echo "[$l_index/$l_total] Compiling: $l_source"
    echo "[$l_index/$l_total] -> $l_object"

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

    ((l_index++))
done

echo "Linking $((${#l_objects[@]})) objects into wrapper.dll"

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
