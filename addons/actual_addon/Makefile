SRCDIR                = .
SUBDIRS               =
DLLS                  = actual_addon.dll
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
			-I patch_callbacks/include \
			-I patch_t/include \
			-I settings_parser/include \
			-I ../
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### actual_addon.dll sources and settings

actual_addon_dll_MODULE= actual_addon.dll
actual_addon_dll_C_SRCS= src/main.c \
			 addon_callbacks/src/addon_callbacks.c \
			 patch_t/src/patch_t.c \
			 patch_callbacks/src/patch_callbacks.c \
			 settings_parser/src/settings_parser.c \
			 ../../states_dll/_useCallback/src/_useCallback.c
actual_addon_dll_CXX_SRCS=
actual_addon_dll_RC_SRCS=
actual_addon_dll_LDFLAGS= -shared \
			$(actual_addon_dll_MODULE:.dll=.def) \
			-m32 \
			-s
actual_addon_dll_ARFLAGS=
actual_addon_dll_DLL_PATH=
actual_addon_dll_DLLS = odbc32 \
			ole32 \
			oleaut32 \
			winspool \
			odbccp32
actual_addon_dll_LIBRARY_PATH=
actual_addon_dll_LIBRARIES= uuid

actual_addon_dll_OBJS = $(actual_addon_dll_C_SRCS:.c=.o) \
			$(actual_addon_dll_CXX_SRCS:.cpp=.o) \
			$(actual_addon_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(actual_addon_dll_C_SRCS)
CXX_SRCS              = $(actual_addon_dll_CXX_SRCS)
RC_SRCS               = $(actual_addon_dll_RC_SRCS)


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

$(actual_addon_dll_MODULE).so: $(actual_addon_dll_OBJS) $(actual_addon_dll_MODULE:.dll=.def)
	$(CXX) $(actual_addon_dll_LDFLAGS) -o $@ $(actual_addon_dll_OBJS) $(actual_addon_dll_LIBRARY_PATH) $(actual_addon_dll_DLL_PATH) $(DEFLIB) $(actual_addon_dll_DLLS:%=-l%) $(actual_addon_dll_LIBRARIES:%=-l%)


