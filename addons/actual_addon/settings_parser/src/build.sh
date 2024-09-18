#!/bin/bash
#-fopt-info-optall=out.txt && \
clear && \
    gcc \
    main.c \
    -Og -g \
    settings_parser.c \
    ../../../../stdfunc/src/stdfunc.c \
    -I ../include \
    -I ../../../../stdfunc/include \
    -march=native \
    -fopenmp-simd \
    -m32
#   -fopt-info-vec=out.txt && \
#   valgrind --leak-check=full -s ./a.out && \
#   cat out.txt
    #cat out.txt  | less
