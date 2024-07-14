SRCDIR                = .
SUBDIRS               =
DLLS                  = netplay.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11 \
			-fpermissive
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++11 \
			-fpermissive
RCEXTRA               =
DEFINES               =
INCLUDE_PATH          = -I ../../states_dll/_useCallback/include \
			-I ../overlay_addon/native/include \
			-I addon_callbacks/include \
			-I ../ \
			-I ../../
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### netplay.dll sources and settings

netplay_dll_MODULE= netplay.dll
netplay_dll_C_SRCS=
netplay_dll_CXX_SRCS= addon_callbacks/src/addon_callbacks.cpp \
			    ../../states_dll/_useCallback/src/_useCallback.cpp
netplay_dll_RC_SRCS=
netplay_dll_LDFLAGS= -shared \
			$(netplay_dll_MODULE:.dll=.def) \
			-m32 \
			-s \
			-static
netplay_dll_ARFLAGS=
netplay_dll_DLL_PATH=
netplay_dll_DLLS= odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
netplay_dll_LIBRARY_PATH=
netplay_dll_LIBRARIES= uuid \
			     ws2_32 \
			     mswsock

netplay_dll_OBJS= $(netplay_dll_C_SRCS:.c=.o) \
			$(netplay_dll_CXX_SRCS:.cpp=.o) \
			$(netplay_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(netplay_dll_C_SRCS)
CXX_SRCS              = $(netplay_dll_CXX_SRCS)
RC_SRCS               = $(netplay_dll_RC_SRCS)


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

$(netplay_dll_MODULE).so: $(netplay_dll_OBJS) $(netplay_dll_MODULE:.dll=.def)
	$(CXX) $(netplay_dll_LDFLAGS) -o $@ $(netplay_dll_OBJS) $(netplay_dll_LIBRARY_PATH) $(netplay_dll_DLL_PATH) $(DEFLIB) $(netplay_dll_DLLS:%=-l%) $(netplay_dll_LIBRARIES:%=-l%)


