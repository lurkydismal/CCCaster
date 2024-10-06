SRCDIR                = .
SUBDIRS               =
DLLS                  = d3d9.dll
LIBS                  =
EXES                  =



### Common settings

CEXTRA                = -Ofast \
			-m32 \
			-std=c11 \
			-march=native \
			-flto \
			-fopenmp \
			-funroll-loops
CXXEXTRA              = -Ofast \
			-m32 \
			-std=c++11 \
			-march=native \
			-fopenmp \
			-funroll-loops \
			-fvisibility=hidden
RCEXTRA               =
DEFINES               = -D_GLIBCXX_PARALLEL
INCLUDE_PATH          = -I ../states_dll/_useCallback/include \
			-I _useCallback/include \
			-I D3DPERF_BeginEvent/include \
			-I D3DPERF_EndEvent/include \
			-I D3DPERF_GetStatus/include \
			-I D3DPERF_QueryRepeatFrame/include \
			-I D3DPERF_SetMarker/include \
			-I D3DPERF_SetOptions/include \
			-I D3DPERF_SetRegion/include \
			-I DebugSetLevel/include \
			-I DebugSetMute/include \
			-I Direct3D9EnableMaximizedWindowedModeShim/include \
			-I Direct3DCreate9/include \
			-I Direct3DCreate9Ex/include \
			-I Direct3DShaderValidatorCreate9/include \
			-I PSGPError/include \
			-I PSGPSampleTexture/include
DLL_PATH              =
DLL_IMPORTS           =
LIBRARY_PATH          =
LIBRARIES             =


### d3d9.dll sources and settings

d3d9_dll_MODULE= d3d9.dll
d3d9_dll_C_SRCS= D3DPERF_BeginEvent/src/D3DPERF_BeginEvent.c \
		 D3DPERF_EndEvent/src/D3DPERF_EndEvent.c \
		 D3DPERF_GetStatus/src/D3DPERF_GetStatus.c \
		 D3DPERF_QueryRepeatFrame/src/D3DPERF_QueryRepeatFrame.c \
		 D3DPERF_SetMarker/src/D3DPERF_SetMarker.c \
		 D3DPERF_SetOptions/src/D3DPERF_SetOptions.c \
		 D3DPERF_SetRegion/src/D3DPERF_SetRegion.c \
		 DebugSetLevel/src/DebugSetLevel.c \
		 DebugSetMute/src/DebugSetMute.c \
		 Direct3D9EnableMaximizedWindowedModeShim/src/Direct3D9EnableMaximizedWindowedModeShim.c \
		 Direct3DShaderValidatorCreate9/src/Direct3DShaderValidatorCreate9.c \
		 PSGPError/src/PSGPError.c \
		 PSGPSampleTexture/src/PSGPSampleTexture.c \
		 src/main.c \
		 ../states_dll/_useCallback/src/_useCallback.c
d3d9_dll_CXX_SRCS= Direct3DCreate9/src/Direct3DCreate9.cpp \
		   Direct3DCreate9Ex/src/Direct3DCreate9Ex.cpp \
		   src/IDirect3D9Ex.cpp \
		   src/IDirect3DCubeTexture9.cpp \
		   src/IDirect3DDevice9Ex.cpp \
		   src/IDirect3DIndexBuffer9.cpp \
		   src/IDirect3DPixelShader9.cpp \
		   src/IDirect3DQuery9.cpp \
		   src/IDirect3DStateBlock9.cpp \
		   src/IDirect3DSurface9.cpp \
		   src/IDirect3DSwapChain9Ex.cpp \
		   src/IDirect3DTexture9.cpp \
		   src/IDirect3DVertexBuffer9.cpp \
		   src/IDirect3DVertexDeclaration9.cpp \
		   src/IDirect3DVertexShader9.cpp \
		   src/IDirect3DVolume9.cpp \
		   src/IDirect3DVolumeTexture9.cpp \
		   src/InterfaceQuery.cpp
d3d9_dll_RC_SRCS=
d3d9_dll_LDFLAGS= -shared \
		  $(d3d9_dll_MODULE:.dll=.def) \
		  -m32 \
		  -s \
		  -O1 \
		  -static-libstdc++ \
		  -static
d3d9_dll_ARFLAGS=
d3d9_dll_DLL_PATH=
d3d9_dll_DLLS = odbc32 \
		ole32 \
		oleaut32 \
		winspool \
		odbccp32
d3d9_dll_LIBRARY_PATH=
d3d9_dll_LIBRARIES= uuid \
		    d3dx9 \
		    user32

d3d9_dll_OBJS = $(d3d9_dll_C_SRCS:.c=.o) \
			$(d3d9_dll_CXX_SRCS:.cpp=.o) \
			$(d3d9_dll_RC_SRCS:.rc=.res)



### Global source lists

C_SRCS                = $(d3d9_dll_C_SRCS)
CXX_SRCS              = $(d3d9_dll_CXX_SRCS)
RC_SRCS               = $(d3d9_dll_RC_SRCS)


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

$(d3d9_dll_MODULE).so: $(d3d9_dll_OBJS) $(d3d9_dll_MODULE:.dll=.def)
	$(CXX) $(d3d9_dll_LDFLAGS) -o $@ $(d3d9_dll_OBJS) $(d3d9_dll_LIBRARY_PATH) $(d3d9_dll_DLL_PATH) $(DEFLIB) $(d3d9_dll_DLLS:%=-l%) $(d3d9_dll_LIBRARIES:%=-l%)


