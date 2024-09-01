SRCDIR                = .
SUBDIRS               =
DLLS                  = example.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11 \
			-march=native
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++11 \
			-march=native
RCEXTRA               =
DEFINES               =
INCLUDE_PATH          =
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### example.dll sources and settings

example_dll_MODULE    = example.dll
example_dll_C_SRCS    = example.c
example_dll_CXX_SRCS  =
example_dll_RC_SRCS   =
example_dll_LDFLAGS   = -shared \
			$(example_dll_MODULE:.dll=.def) \
			-m32 \
			-s
example_dll_ARFLAGS   =
example_dll_DLL_PATH  =
example_dll_DLLS      = odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
example_dll_LIBRARY_PATH=
example_dll_LIBRARIES = uuid

example_dll_OBJS      = $(example_dll_C_SRCS:.c=.o) \
			$(example_dll_CXX_SRCS:.cpp=.o) \
			$(example_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(example_dll_C_SRCS)
CXX_SRCS              = $(example_dll_CXX_SRCS)
RC_SRCS               = $(example_dll_RC_SRCS)


### Tools

CC = ccache i686-w64-mingw32-gcc
CXX = ccache i686-w64-mingw32-g++
RC = wrc
AR = ar


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

### Target.defific build rules
DEFLIB = $(LIBRARY_PATH) $(LIBRARIES) $(DLL_PATH) $(DLL_IMPORTS:%=-l%)

$(example_dll_MODULE).so: $(example_dll_OBJS) $(example_dll_MODULE:.dll=.def)
	$(CXX) $(example_dll_LDFLAGS) -o $@ $(example_dll_OBJS) $(example_dll_LIBRARY_PATH) $(example_dll_DLL_PATH) $(DEFLIB) $(example_dll_DLLS:%=-l%) $(example_dll_LIBRARIES:%=-l%)


