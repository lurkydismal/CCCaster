SRCDIR                = .
SUBDIRS               =
DLLS                  = log.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11 \
			-march=native \
			-fopenmp
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++11 \
			-march=native \
			-fopenmp
RCEXTRA               =
DEFINES               =
INCLUDE_PATH          = -I log/include \
			-I addon_callbacks/include
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### log.dll sources and settings

keyboard_dll_MODULE= log.dll
keyboard_dll_C_SRCS= src/main.c \
		     log/src/log.c \
		     addon_callbacks/src/addon_callbacks.c
keyboard_dll_CXX_SRCS=
keyboard_dll_RC_SRCS=
keyboard_dll_LDFLAGS= -shared \
			$(keyboard_dll_MODULE:.dll=.def) \
			-m32 \
			-s
keyboard_dll_ARFLAGS=
keyboard_dll_DLL_PATH=
keyboard_dll_DLLS= odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
keyboard_dll_LIBRARY_PATH=
keyboard_dll_LIBRARIES= uuid

keyboard_dll_OBJS= $(keyboard_dll_C_SRCS:.c=.o) \
			$(keyboard_dll_CXX_SRCS:.cpp=.o) \
			$(keyboard_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(keyboard_dll_C_SRCS)
CXX_SRCS              = $(keyboard_dll_CXX_SRCS)
RC_SRCS               = $(keyboard_dll_RC_SRCS)


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

### Target specific build rules
DEFLIB = $(LIBRARY_PATH) $(LIBRARIES) $(DLL_PATH) $(DLL_IMPORTS:%=-l%)

$(keyboard_dll_MODULE).so: $(keyboard_dll_OBJS) $(keyboard_dll_MODULE:.dll=.def)
	$(CXX) $(keyboard_dll_LDFLAGS) -o $@ $(keyboard_dll_OBJS) $(keyboard_dll_LIBRARY_PATH) $(keyboard_dll_DLL_PATH) $(DEFLIB) $(keyboard_dll_DLLS:%=-l%) $(keyboard_dll_LIBRARIES:%=-l%)


