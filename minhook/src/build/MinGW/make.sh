ccache \
    i686-w64-mingw32-gcc-win32 \
    -Ofast \
    -std=c11 \
    -masm=intel \
    -Wall \
    -Werror \
    -I../../include \
    -I../../src \
    ../../src/*.c \
    ../../src/hde/*.c \
    -c && \
    i686-w64-mingw32-ar rcs -o libminhook.a *.o && \
    rm *.o
