SRCDIR                = .
SUBDIRS               =
DLLS                  = overlay.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++11
RCEXTRA               =
DEFINES               =
INCLUDE_PATH          = -I ../../states_dll/_useCallback/include \
			-I addon_callbacks/include \
			-I native/include \
			-I ../../
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          = -L native/src
LIBRARIES             = -lCallDraw


### overlay.dll sources and settings

overlay_dll_MODULE= overlay.dll
overlay_dll_C_SRCS= addon_callbacks/src/addon_callbacks.c \
		    native/src/native.c \
		    ../../states_dll/_useCallback/src/_useCallback.c
overlay_dll_CXX_SRCS=
overlay_dll_RC_SRCS=
overlay_dll_LDFLAGS= -shared \
			$(overlay_dll_MODULE:.dll=.def) \
			-m32 \
			-s
overlay_dll_ARFLAGS=
overlay_dll_DLL_PATH=
overlay_dll_DLLS= odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
overlay_dll_LIBRARY_PATH=
overlay_dll_LIBRARIES= uuid \
        gdi32 \
		        dwmapi


overlay_dll_OBJS= $(overlay_dll_C_SRCS:.c=.o) \
			$(overlay_dll_CXX_SRCS:.cpp=.o) \
			$(overlay_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(overlay_dll_C_SRCS)
CXX_SRCS              = $(overlay_dll_CXX_SRCS)
RC_SRCS               = $(overlay_dll_RC_SRCS)


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

$(overlay_dll_MODULE).so: $(overlay_dll_OBJS) $(overlay_dll_MODULE:.dll=.def)
	$(CXX) $(overlay_dll_LDFLAGS) -o $@ $(overlay_dll_OBJS) $(overlay_dll_LIBRARY_PATH) $(overlay_dll_DLL_PATH) $(DEFLIB) $(overlay_dll_DLLS:%=-l%) $(overlay_dll_LIBRARIES:%=-l%)


