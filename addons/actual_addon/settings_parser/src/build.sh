#!/bin/bash
#-fopt-info-optall=out.txt && \
clear && \
    gcc \
    main.c \
    -Ofast \
    -g \
    settings_parser.c \
    ../../../../stdfunc/src/stdfunc.c \
    -I ../include \
    -I ../../../../stdfunc/include \
    -march=native \
    -funroll-loops \
    -fopenmp-simd \
    -m32
    #-fprofile-use
    #-fprofile-generate
    #-Og -g -pg \
#   -fopt-info-vec=out.txt && \
#   valgrind --leak-check=full -s ./a.out && \
#   cat out.txt
    #cat out.txt  | less
