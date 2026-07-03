#!/usr/bin/env fish

fd --type f \
    -e c -e cc -e cpp -e cxx \
    -e h -e hh -e hpp -e hxx \
    -e inc -e inl \
    -e m -e mm |
    parallel --bar clang-format -i {}
