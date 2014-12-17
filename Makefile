VERSION = 2.1
SUFFIX = e
NAME = cccaster
TAG =

ifneq ($(TAG),)
DOT_TAG = .$(TAG)
endif

# Main programs
ARCHIVE = $(NAME).v$(VERSION)$(SUFFIX)$(DOT_TAG).zip
BINARY = $(NAME).v$(VERSION)$(DOT_TAG).exe
FOLDER = $(NAME)
DLL = hook$(DOT_TAG).dll
LAUNCHER = launcher.exe
DEBUGGER = debugger.exe
GENERATOR = generator.exe
MBAA_EXE = MBAA.exe

# Library sources
GTEST_CC_SRCS = 3rdparty/gtest/fused-src/gtest/gtest-all.cc
JLIB_CC_SRCS = $(wildcard 3rdparty/JLib/*.cc)
HOOK_CC_SRCS = $(wildcard 3rdparty/minhook/src/*.cc) $(wildcard 3rdparty/d3dhook/*.cc)
HOOK_C_SRCS = $(wildcard 3rdparty/minhook/src/hde32/*.c)
CONTRIB_CC_SRCS = $(GTEST_CC_SRCS) $(JLIB_CC_SRCS)
CONTRIB_C_SRCS = $(wildcard 3rdparty/*.c)

# Main program sources
LIB_CPP_SRCS = $(wildcard lib/*.cpp)
BASE_CPP_SRCS = $(wildcard *.cpp) $(LIB_CPP_SRCS)
MAIN_CPP_SRCS = $(wildcard targets/Main*.cpp) $(wildcard tests/*.cpp) $(BASE_CPP_SRCS)
DLL_CPP_SRCS = $(wildcard targets/Dll*.cpp) $(BASE_CPP_SRCS)

NON_GEN_SRCS = \
	$(wildcard *.cpp) $(wildcard tools/*.cpp) $(wildcard targets/*.cpp) $(wildcard lib/*.cpp) $(wildcard tests/*.cpp)
NON_GEN_HEADERS = \
	$(filter-out lib/Version.%.h lib/Protocol.%.h,$(wildcard *.h targets/*.h lib/*.h tests/*.h))
AUTOGEN_HEADERS = $(wildcard lib/Version.*.h) $(wildcard lib/Protocol.*.h)

# Main program objects
LIB_OBJECTS = $(LIB_CPP_SRCS:.cpp=.o) $(CONTRIB_C_SRCS:.c=.o)
MAIN_OBJECTS = $(MAIN_CPP_SRCS:.cpp=.o) $(CONTRIB_CC_SRCS:.cc=.o) $(CONTRIB_C_SRCS:.c=.o)
DLL_OBJECTS = $(DLL_CPP_SRCS:.cpp=.o) $(HOOK_CC_SRCS:.cc=.o) $(HOOK_C_SRCS:.c=.o) $(CONTRIB_C_SRCS:.c=.o)

# Tool chain
PREFIX = i686-w64-mingw32-
GCC = $(PREFIX)gcc
CXX = $(PREFIX)g++
WINDRES = $(PREFIX)windres
STRIP = $(PREFIX)strip
TOUCH = touch
ZIP = zip

# OS specific tools
ifeq ($(OS),Windows_NT)
	CHMOD_X = icacls $@ /grant Everyone:F
	GRANT = icacls $@ /grant Everyone:F
	ASTYLE = 3rdparty/astyle.exe
else
	CHMOD_X = chmod +x $@
	GRANT =
	ASTYLE = 3rdparty/astyle
	TOUCH = $(PREFIX)strip
endif


# Build flags
DEFINES = -DWIN32_LEAN_AND_MEAN -DWINVER=0x501 -D_WIN32_WINNT=0x501 -D_M_IX86
DEFINES += -DNAMED_PIPE='"\\\\.\\pipe\\cccaster_pipe"' -DMBAA_EXE='"$(MBAA_EXE)"' -DBINARY='"$(BINARY)"'
DEFINES += -DHOOK_DLL='"$(FOLDER)\\$(DLL)"' -DLAUNCHER='"$(FOLDER)\\$(LAUNCHER)"' -DFOLDER='"$(FOLDER)\\"'
INCLUDES = -I$(CURDIR) -I$(CURDIR)/lib -I$(CURDIR)/tests -I$(CURDIR)/3rdparty -I$(CURDIR)/3rdparty/cereal/include
INCLUDES += -I$(CURDIR)/3rdparty/gtest/include -I$(CURDIR)/3rdparty/minhook/include -I$(CURDIR)/3rdparty/d3dhook
CC_FLAGS = -m32 $(INCLUDES) $(DEFINES)

# Linker flags
LD_FLAGS = -m32 -static -lws2_32 -lpsapi -lwinpthread -lwinmm -lole32

# Build options
# DEFINES += -DDISABLE_LOGGING
# DEFINES += -DDISABLE_ASSERTS
# DEFINES += -DLOGGER_MUTEXED
# DEFINES += -DJLIB_MUTEXED

# Install after make, set to 0 to disable install after make
INSTALL = 1

# Build type flags
DEBUG_FLAGS   = -ggdb3 -O0 -fno-inline -D_GLIBCXX_DEBUG
LOGGING_FLAGS = -s -Os -O2 -DRELEASE
RELEASE_FLAGS = -s -Os -O2 -fno-rtti -DNDEBUG -DRELEASE -DDISABLE_LOGGING -DDISABLE_ASSERTS

# Build type
BUILD_TYPE = build_debug


all: debug

# target-profile: STRIP = $(TOUCH)
# target-profile: DEFINES += -DNDEBUG -DRELEASE -DDISABLE_LOGGING -DDISABLE_ASSERTS
# target-profile: CC_FLAGS += -O2 -fno-rtti -pg
# target-profile: LD_FLAGS += -pg -lgmon
# target-profile: $(ARCHIVE)

launcher: $(FOLDER)/$(LAUNCHER)
debugger: tools/$(DEBUGGER)
generator: tools/$(GENERATOR)


$(ARCHIVE): $(BINARY) $(FOLDER)/$(DLL) $(FOLDER)/$(LAUNCHER) $(FOLDER)/states.bin
	@echo
	rm -f $(filter-out %.log,$(filter-out $(ARCHIVE),$(wildcard $(NAME)*.zip)))
	$(ZIP) $(ARCHIVE) ChangeLog.txt $^
	$(GRANT)

$(BINARY): $(addprefix $(BUILD_TYPE)/,$(MAIN_OBJECTS))
	rm -f $(filter-out $(BINARY),$(wildcard $(NAME)*.exe))
	$(CXX) -o $@ $(CC_FLAGS) -Wall -std=c++11 $(addprefix $(BUILD_TYPE)/,$(MAIN_OBJECTS)) $(LD_FLAGS)
	@echo
	$(STRIP) $@
	$(CHMOD_X)
	@echo

$(FOLDER)/$(DLL): $(addprefix $(BUILD_TYPE)/,$(DLL_OBJECTS))
	@mkdir -p $(FOLDER)
	$(CXX) -o $@ $(CC_FLAGS) -Wall -std=c++11 $(addprefix $(BUILD_TYPE)/,$(DLL_OBJECTS)) -shared $(LD_FLAGS) -ld3dx9
	@echo
	$(STRIP) $@
	$(GRANT)
	@echo

$(FOLDER)/$(LAUNCHER): tools/Launcher.cpp
	@mkdir -p $(FOLDER)
	$(CXX) -o $@ tools/Launcher.cpp -m32 -s -Os -O2 -Wall -static -mwindows
	@echo
	$(PREFIX)strip $@
	$(CHMOD_X)
	@echo

$(FOLDER)/states.bin: tools/$(GENERATOR)
	tools/$(GENERATOR) $(FOLDER)/states.bin

# tools/$(DEBUGGER): DEFINES += -DRELEASE
# tools/$(DEBUGGER): CC_FLAGS += -s -Os -O2
# tools/$(DEBUGGER): $(LIB_OBJECTS)
# 	$(CXX) -o $@ $(CC_FLAGS) -Wall -std=c++11 tools/Debugger.cpp $(LIB_OBJECTS) $(LD_FLAGS) \
# -I$(CURDIR)/3rdparty/distorm3/include -L$(CURDIR)/3rdparty/distorm3 -ldistorm3
# 	@echo
# 	$(PREFIX)strip $@
# 	$(CHMOD_X)
# 	@echo

tools/$(GENERATOR): CC_FLAGS += $(RELEASE_FLAGS)
tools/$(GENERATOR): build_release tools/Generator.cpp $(addprefix build_release/,$(LIB_OBJECTS))
	$(CXX) -o $@ $(CC_FLAGS) -Wall -std=c++11 tools/Generator.cpp $(addprefix build_release/,$(LIB_OBJECTS)) $(LD_FLAGS)
	@echo
	$(PREFIX)strip $@
	$(CHMOD_X)
	@echo

# res/icon.res: res/icon.rc res/icon.ico
# 	$(WINDRES) -F pe-i386 res/icon.rc -O coff -o $@


define make_version
@scripts/make_version $(VERSION)$(SUFFIX) > lib/Version.local.h
endef

define make_protocol
@scripts/make_protocol $(NON_GEN_HEADERS)
endef

define make_depend
@scripts/make_depend "$(CXX)" "-m32 $(INCLUDES)"
endef


version:
	$(make_version)

proto:
	$(make_protocol)

reset-proto:
	rm -f lib/ProtocolEnums.h
	@$(MAKE) proto

depend: version proto
	$(make_depend)

.depend: $(NON_GEN_SRCS) $(NON_GEN_HEADERS)
	$(make_version)
	$(make_protocol)
	$(make_depend)


clean-proto:
	git co -- lib/ProtocolEnums.h
	rm -f $(AUTOGEN_HEADERS)

clean-common: clean-proto
	rm -f .depend .include
	rm -f *.res *.exe *.dll *.zip $(FOLDER)/*.exe $(FOLDER)/*.dll

clean: clean-common
	rm -rf build_debug

clean-logging: clean-common
	rm -rf build_logging

clean-release: clean-common
	rm -rf build_release

clean-all: clean clean-logging clean-release
	rm -rf $(FOLDER)


check:
	cppcheck --enable=all $(NON_GEN_SRCS) $(NON_GEN_HEADERS)

trim:
	sed --binary --in-place 's/\\r$$//' $(NON_GEN_SRCS) $(NON_GEN_HEADERS)
	sed --in-place 's/[[:space:]]\\+$$//' $(NON_GEN_SRCS) $(NON_GEN_HEADERS)

format:
	$(ASTYLE)                   	\
    --indent=spaces=4           	\
    --convert-tabs              	\
    --indent-preprocessor       	\
    --indent-switches           	\
    --style=allman              	\
    --max-code-length=120       	\
    --pad-paren                 	\
    --pad-oper                  	\
    --suffix=none               	\
    --formatted                 	\
    --keep-one-line-blocks      	\
    --align-pointer=name        	\
    --align-reference=type      	\
$(filter-out CharacterSelect.cpp lib/KeyboardMappings.h AsmHacks.h,$(NON_GEN_SRCS) $(NON_GEN_HEADERS))

count:
	@wc -l $(NON_GEN_SRCS) $(NON_GEN_HEADERS) | sort -nr | head -n 10 && echo '    ...'


ifeq (,$(findstring version,$(MAKECMDGOALS)))
ifeq (,$(findstring proto,$(MAKECMDGOALS)))
ifeq (,$(findstring depend,$(MAKECMDGOALS)))
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
ifeq (,$(findstring check,$(MAKECMDGOALS)))
ifeq (,$(findstring trim,$(MAKECMDGOALS)))
ifeq (,$(findstring format,$(MAKECMDGOALS)))
ifeq (,$(findstring count,$(MAKECMDGOALS)))
ifeq (,$(findstring install,$(MAKECMDGOALS)))
ifeq (,$(findstring sdl,$(MAKECMDGOALS)))
-include .depend
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif


pre-build:
	$(make_version)
	$(make_protocol)
	@echo
	@echo ========== Main-build ==========
	@echo

post-build: main-build
	@echo
	@echo ========== Post-build ==========
	@echo
	if [ $(INSTALL) = 1 ] && [ -s ./scripts/install ]; then ./scripts/install; fi;


debug: post-build
logging: post-build
release: post-build
profile: post-build

target-debug: $(ARCHIVE)
target-logging: $(ARCHIVE)
target-release: $(ARCHIVE)


ifneq (,$(findstring logging,$(MAKECMDGOALS)))
main-build: pre-build build_logging
	@$(MAKE) --no-print-directory target-logging BUILD_TYPE=build_logging
else
ifneq (,$(findstring release,$(MAKECMDGOALS)))
main-build: pre-build build_release
	@$(MAKE) --no-print-directory target-release BUILD_TYPE=build_release
else
ifneq (,$(findstring profile,$(MAKECMDGOALS)))
main-build: pre-build build_debug
	@$(MAKE) --no-print-directory target-profile BUILD_TYPE=build_debug STRIP=touch
else
main-build: pre-build build_debug
	@$(MAKE) --no-print-directory target-debug BUILD_TYPE=build_debug STRIP=touch
endif
endif
endif


build_debug:
	rsync -a -f"- .git/" -f"+ */" -f"- *" --exclude=".*" . $@

build_debug/%.o: %.cpp
	$(CXX) $(CC_FLAGS) $(DEBUG_FLAGS) -Wall -Wempty-body -std=c++11 -o $@ -c $<

build_debug/%.o: %.cc
	$(CXX) $(CC_FLAGS) $(DEBUG_FLAGS) -o $@ -c $<

build_debug/%.o: %.c
	$(GCC) $(filter-out -fno-rtti,$(CC_FLAGS) $(DEBUG_FLAGS)) -Wno-attributes -o $@ -c $<


build_logging:
	rsync -a -f"- .git/" -f"+ */" -f"- *" --exclude=".*" . $@

build_logging/%.o: %.cpp
	$(CXX) $(CC_FLAGS) $(LOGGING_FLAGS) -Wall -Wempty-body -std=c++11 -o $@ -c $<

build_logging/%.o: %.cc
	$(CXX) $(CC_FLAGS) $(LOGGING_FLAGS) -o $@ -c $<

build_logging/%.o: %.c
	$(GCC) $(filter-out -fno-rtti,$(CC_FLAGS) $(LOGGING_FLAGS)) -Wno-attributes -o $@ -c $<


build_release:
	rsync -a -f"- .git/" -f"+ */" -f"- *" --exclude=".*" . $@

build_release/%.o: %.cpp
	$(CXX) $(CC_FLAGS) $(RELEASE_FLAGS) -Wall -Wempty-body -std=c++11 -o $@ -c $<

build_release/%.o: %.cc
	$(CXX) $(CC_FLAGS) $(RELEASE_FLAGS) -o $@ -c $<

build_release/%.o: %.c
	$(GCC) $(filter-out -fno-rtti,$(CC_FLAGS) $(RELEASE_FLAGS)) -Wno-attributes -o $@ -c $<

