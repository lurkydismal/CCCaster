SRCDIR                = .
SUBDIRS               =
DLLS                  = states.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11 \
			-march=native \
			-flto \
			-fopenmp-simd \
			-funroll-loops
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++17 \
			-march=native \
			-fopenmp \
			-funroll-loops \
			-fvisibility=hidden \
			-Wno-narrowing \
			-Wno-overflow \
			-Wno-shift-count-overflow
RCEXTRA               =
DEFINES               = -D_GLIBCXX_PARALLEL
INCLUDE_PATH          = -I include \
			-I ../stdfunc/include
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### states.dll sources and settings

states_dll_MODULE = states.dll
states_dll_C_SRCS = ../stdfunc/src/stdfunc.c
states_dll_CXX_SRCS= src/main.cpp
states_dll_RC_SRCS=
states_dll_LDFLAGS= -shared \
			$(states_dll_MODULE:.dll=.def) \
			-m32 \
			-s \
			-O1 \
			-static-libstdc++ \
			-static
states_dll_ARFLAGS=
states_dll_DLL_PATH=
states_dll_DLLS   =
states_dll_LIBRARY_PATH=
states_dll_LIBRARIES=

states_dll_OBJS   = $(states_dll_C_SRCS:.c=.o) \
			$(states_dll_CXX_SRCS:.cpp=.o) \
			$(states_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(states_dll_C_SRCS)
CXX_SRCS              = $(states_dll_CXX_SRCS)
RC_SRCS               = $(states_dll_RC_SRCS)


### Tools

CC = ccache i686-w64-mingw32-gcc 
CXX = ccache i686-w64-mingw32-g++ 
RC = wrc
AR = i686-w64-mingw32-ar


### Generic targets

all: $(SUBDIRS) $(DLLS:%=%.so) $(LIBS) $(EXES)

### Build rules

.PHONY: all clean dummy

$(SUBDIRS): dummy
	@cd $@ && $(MAKE)

# Implicit rules

.SUFFIXES: .cpp .cxx .rc .res
DEFINCL = $(INCLUDE_PATH) $(DEFINES) $(OPTIONS)

.c.o:
	$(CC) -c $(CFLAGS) $(CEXTRA) $(DEFINCL) -o $@ $<

.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CXXEXTRA) $(DEFINCL) -o $@ $<

.cxx.o:
	$(CXX) -c $(CXXFLAGS) $(CXXEXTRA) $(DEFINCL) -o $@ $<

.rc.res:
	$(RC) $(RCFLAGS) $(RCEXTRA) $(DEFINCL) -fo$@ $<

# Rules for cleaning

CLEAN_FILES     = y.tab.c y.tab.h lex.yy.c core *.orig *.rej \
                  \\\#*\\\# *~ *% .\\\#*

clean:: $(SUBDIRS:%=%/__clean__) $(EXTRASUBDIRS:%=%/__clean__)
	$(RM) $(CLEAN_FILES) $(RC_SRCS:.rc=.res) $(C_SRCS:.c=.o) $(CXX_SRCS:.cpp=.o)
	$(RM) $(DLLS:%=%.so) $(LIBS) $(EXES) $(EXES:%=%.so)

$(SUBDIRS:%=%/__clean__): dummy
	cd `dirname $@` && $(MAKE) clean

$(EXTRASUBDIRS:%=%/__clean__): dummy
	-cd `dirname $@` && $(RM) $(CLEAN_FILES)

### Target specific build rules
DEFLIB = $(LIBRARY_PATH) $(LIBRARIES) $(DLL_PATH) $(DLL_IMPORTS:%=-l%)

$(states_dll_MODULE).so: $(states_dll_OBJS) $(states_dll_MODULE:.dll=.def)
	$(CXX) $(states_dll_LDFLAGS) -o $@ $(states_dll_OBJS) $(states_dll_LIBRARY_PATH) $(states_dll_DLL_PATH) $(DEFLIB) $(states_dll_DLLS:%=-l%) $(states_dll_LIBRARIES:%=-l%)
