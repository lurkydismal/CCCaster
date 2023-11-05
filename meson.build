# meson setup --reconfigure --cross-file i686-w64-mingw32.txt meson-mingw/
# meson compile -C meson-mingw

# clear && meson setup --reconfigure --cross-file i686-w64-mingw32.txt meson-mingw/ && meson compile -C meson-mingw && i686-w64-mingw32-strip meson-mingw/*.exe meson-mingw//*.dll

project(
    'CCCaster',
    'cpp',
    'c',
    version: 'v1.0.0',
    default_options: [ 'c_std=c11', 'cpp_std=c++11', 'warning_level=3' ]
)

cxx = meson.get_compiler( 'cpp' )

# add_project_link_arguments(
#     [
#         '-lws2_32',
#         '-lpsapi',
#         '-lwinpthread',
#         '-lwinmm',
#         '-lole32',
#         '-ldinput',
#         '-lwininet'
#     ],
#     language : [
#         'c', 'cpp'
#     ]
# )

argument_list = [
    '-Wfatal-errors',
    '-DWIN32_LEAN_AND_MEAN',
    '-DWINVER=0x501',
    '-D_WIN32_WINNT=0x501',
    '-D_M_IX86',
    '-DNAMED_PIPE="//./pipe/cccaster_pipe"',
    '-DNAMED_PIPE2="//./pipe/cccaster2_pipe"',
    '-DPALETTES_FOLDER="palettes/"',
    '-DMBAA_EXE="MBAA.exe"',
    '-DBINARY="cccaster.v3.1.debug.exe"',
    '-DFOLDER="cccaster/"',
    '-DHOOK_DLL="cccaster/hook.debug.dll"',
    '-DLAUNCHER="cccaster/launcher.exe"',
    '-DRELAY_LIST="relay_list.txt"',
    '-DTAG="debug"'
]

cereal = dependency( 'cereal' )

netplay_include = include_directories( 'netplay' )
lib_include = include_directories( 'lib' )
thirdparty_include = include_directories( '3rdparty' )
minhook_include = include_directories( '3rdparty/minhook/include' )
d3dhook_include = include_directories( '3rdparty/d3dhook' )
JLib_include = include_directories( '3rdparty/JLib' )

# glfw_include = include_directories( '3rdparty/glfw' )
# AntTweakBar_include = include_directories( '3rdparty/AntTweakBar/include' )
# libpng_include = include_directories( '3rdparty/libpng' )
# libz_include = include_directories( '3rdparty/libz' )
# framedisplay_include = include_directories( '3rdparty/framedisplay' )

# add_project_arguments(
#     cxx.get_supported_arguments( argument_list ),
#     language: 'cpp'
# )

test_warning_argument_list = [
    '-Ofast',
    '-DNDEBUG',
    '-DRELEASE',
    '-DDISABLE_ASSERTS'
]

# add_project_arguments(
#     cxx.get_supported_arguments( test_warning_argument_list ),
#     language: 'cpp'
# )

thirdparty = declare_dependency(
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        '3rdparty/md5.c',
        '3rdparty/miniz.c'
    ],
    version : '1.0.0'
)
minhook = declare_dependency(
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        '3rdparty/minhook/src/buffer.cc',
        '3rdparty/minhook/src/export.cc',
        '3rdparty/minhook/src/hook.cc',
        '3rdparty/minhook/src/thread.cc',
        '3rdparty/minhook/src/trampoline.cc',
        '3rdparty/minhook/src/hde32/hde32.c'
    ],
    version : '1.0.0'
)
d3dhook = declare_dependency(
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        '3rdparty/d3dhook/CHookJump.cc',
        '3rdparty/d3dhook/D3DHook.cc'
    ],
    version : '1.0.0'
)
JLib = declare_dependency(
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        '3rdparty/JLib/CharacterBox.cc',
        '3rdparty/JLib/CodeFinder.cc',
        '3rdparty/JLib/ConsoleCore.cc',
        '3rdparty/JLib/ConsoleFormat.cc',
        '3rdparty/JLib/ConsoleMenu.cc',
        '3rdparty/JLib/ConsoleMenuItem.cc'
    ],
    version : '1.0.0'
)
netplay = declare_dependency(
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        'netplay/CharacterSelect.cpp',
        'netplay/PaletteManager.cpp',
        'netplay/ProcessManager.cpp',
        'netplay/ReplayCreator.cpp',
        'netplay/ReplayManager.cpp',
        'netplay/SpectatorManager.cpp'
    ],
    version : '1.0.0'
)
lib = declare_dependency(
    dependencies : [
        JLib
    ],
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    sources : [
        'lib/ChangeMonitor.cpp',
        'lib/Compression.cpp',
        'lib/ConsoleUi.cpp',
        'lib/Controller.cpp',
        'lib/ControllerManager.cpp',
        'lib/EventManager.cpp',
        'lib/Exceptions.cpp',
        'lib/ExternalIpAddress.cpp',
        'lib/GoBackN.cpp',
        'lib/Guid.cpp',
        'lib/HttpDownload.cpp',
        'lib/HttpGet.cpp',
        'lib/IpAddrPort.cpp',
        'lib/JoystickDetector.cpp',
        'lib/KeyboardManager.cpp',
        'lib/KeyboardState.cpp',
        'lib/KeyValueStore.cpp',
        'lib/Lobby.cpp',
        'lib/Logger.cpp',
        'lib/LoggerLogVersion.cpp',
        'lib/MatchmakingManager.cpp',
        'lib/MemDump.cpp',
        'lib/MingwSecureApiTrap.cpp',
        'lib/MouseManager.cpp',
        'lib/Pinger.cpp',
        'lib/Protocol.cpp',
        'lib/SmartSocket.cpp',
        'lib/Socket.cpp',
        'lib/SocketManager.cpp',
        'lib/StringUtils.cpp',
        'lib/TcpSocket.cpp',
        'lib/Thread.cpp',
        'lib/Timer.cpp',
        'lib/TimerManager.cpp',
        'lib/UdpSocket.cpp',
        'lib/Version.cpp'
    ],
    version : '1.0.0'
)

# dependencies : [
#     thirdparty,
#     minhook,
#     d3dhook,
#     netplay,
#     cereal
# ],
shared_library(
    'hook.debug',
    [
        'targets/DllMain.cpp',
        'targets/DllAsmHacks.cpp',
        'targets/DllControllerManager.cpp',
        'targets/DllFrameRate.cpp',
        'targets/DllHacks.cpp',
        'targets/DllNetplayManager.cpp',
        'targets/DllOverlayUi.cpp',
        'targets/DllOverlayUiText.cpp',
        'targets/DllPaletteManager.cpp',
        'targets/DllProcessManager.cpp',
        'targets/DllRollbackManager.cpp',
        'targets/DllSpectatorManager.cpp',
        'targets/DllTrialManager.cpp',
        'targets/CallDraw.s',
        'lib/ChangeMonitor.cpp',
        'lib/Compression.cpp',
        'lib/Controller.cpp',
        'lib/ControllerManager.cpp',
        'lib/EventManager.cpp',
        'lib/Exceptions.cpp',
        'lib/ExternalIpAddress.cpp',
        'lib/GoBackN.cpp',
        'lib/Guid.cpp',
        'lib/HttpDownload.cpp',
        'lib/HttpGet.cpp',
        'lib/IpAddrPort.cpp',
        'lib/JoystickDetector.cpp',
        'lib/KeyboardManager.cpp',
        'lib/KeyboardState.cpp',
        'lib/KeyValueStore.cpp',
        'lib/Lobby.cpp',
        'lib/Logger.cpp',
        'lib/LoggerLogVersion.cpp',
        'lib/MatchmakingManager.cpp',
        'lib/MemDump.cpp',
        'lib/MingwSecureApiTrap.cpp',
        'lib/MouseManager.cpp',
        'lib/Pinger.cpp',
        'lib/Protocol.cpp',
        'lib/SmartSocket.cpp',
        'lib/Socket.cpp',
        'lib/SocketManager.cpp',
        'lib/StringUtils.cpp',
        'lib/TcpSocket.cpp',
        'lib/Thread.cpp',
        'lib/Timer.cpp',
        'lib/TimerManager.cpp',
        'lib/UdpSocket.cpp',
        'lib/Version.cpp',
        '3rdparty/md5.c',
        '3rdparty/miniz.c',
        '3rdparty/minhook/src/buffer.cc',
        '3rdparty/minhook/src/export.cc',
        '3rdparty/minhook/src/hook.cc',
        '3rdparty/minhook/src/thread.cc',
        '3rdparty/minhook/src/trampoline.cc',
        '3rdparty/minhook/src/hde32/hde32.c',
        '3rdparty/d3dhook/CHookJump.cc',
        '3rdparty/d3dhook/D3DHook.cc',
        'netplay/CharacterSelect.cpp',
        'netplay/PaletteManager.cpp',
        'netplay/ProcessManager.cpp',
        'netplay/ReplayCreator.cpp',
        'netplay/ReplayManager.cpp',
        'netplay/SpectatorManager.cpp'
    ],
    objects : [
        'res/rollback.o'
    ],
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        minhook_include,
        d3dhook_include,
        JLib_include
    ],
    dependencies : [
        cereal
    ],
    cpp_args : [
        '-Wfatal-errors',
        '-DWIN32_LEAN_AND_MEAN',
        '-DWINVER=0x501',
        '-D_WIN32_WINNT=0x501',
        '-D_M_IX86',
        '-DNAMED_PIPE="//./pipe/cccaster_pipe"',
        '-DNAMED_PIPE2="//./pipe/cccaster2_pipe"',
        '-DPALETTES_FOLDER="palettes/"',
        '-DMBAA_EXE="MBAA.exe"',
        '-DBINARY="cccaster.v3.1.debug.exe"',
        '-DFOLDER="cccaster/"',
        '-DHOOK_DLL="cccaster/hook.debug.dll"',
        '-DLAUNCHER="cccaster/launcher.exe"',
        '-DRELAY_LIST="relay_list.txt"',
        '-DTAG="debug"',
        '-Ofast',
        '-DNDEBUG',
        '-DRELEASE',
        '-DDISABLE_ASSERTS'
    ],
    link_args : [
        '-lws2_32',
        '-lpsapi',
        '-lwinpthread',
        '-lwinmm',
        '-lole32',
        '-ldinput',
        '-lwininet',
        '-ld3dx9',
        '-s'
    ],
    name_prefix : ''
)

# dependencies : [
#     thirdparty,
#     JLib,
#     netplay,
#     lib,
#     cereal
# ],
executable(
    'cccaster.v3.1.debug',
    [
        'targets/Main.cpp',
        'targets/MainApp.cpp',
        'targets/MainUi.cpp',
        '3rdparty/md5.c',
        '3rdparty/miniz.c',
        '3rdparty/JLib/CharacterBox.cc',
        '3rdparty/JLib/CodeFinder.cc',
        '3rdparty/JLib/ConsoleCore.cc',
        '3rdparty/JLib/ConsoleFormat.cc',
        '3rdparty/JLib/ConsoleMenu.cc',
        '3rdparty/JLib/ConsoleMenuItem.cc',
        'netplay/CharacterSelect.cpp',
        'netplay/PaletteManager.cpp',
        'netplay/ProcessManager.cpp',
        'netplay/ReplayCreator.cpp',
        'netplay/ReplayManager.cpp',
        'netplay/SpectatorManager.cpp',
        'lib/ChangeMonitor.cpp',
        'lib/Compression.cpp',
        'lib/ConsoleUi.cpp',
        'lib/Controller.cpp',
        'lib/ControllerManager.cpp',
        'lib/EventManager.cpp',
        'lib/Exceptions.cpp',
        'lib/ExternalIpAddress.cpp',
        'lib/GoBackN.cpp',
        'lib/Guid.cpp',
        'lib/HttpDownload.cpp',
        'lib/HttpGet.cpp',
        'lib/IpAddrPort.cpp',
        'lib/JoystickDetector.cpp',
        'lib/KeyboardManager.cpp',
        'lib/KeyboardState.cpp',
        'lib/KeyValueStore.cpp',
        'lib/Lobby.cpp',
        'lib/Logger.cpp',
        'lib/LoggerLogVersion.cpp',
        'lib/MatchmakingManager.cpp',
        'lib/MemDump.cpp',
        'lib/MingwSecureApiTrap.cpp',
        'lib/MouseManager.cpp',
        'lib/Pinger.cpp',
        'lib/Protocol.cpp',
        'lib/SmartSocket.cpp',
        'lib/Socket.cpp',
        'lib/SocketManager.cpp',
        'lib/StringUtils.cpp',
        'lib/TcpSocket.cpp',
        'lib/Thread.cpp',
        'lib/Timer.cpp',
        'lib/TimerManager.cpp',
        'lib/UdpSocket.cpp',
        'lib/Version.cpp'
    ],
    objects : [
        'res/icon.res'
    ],
    include_directories : [
        netplay_include,
        lib_include,
        thirdparty_include,
        JLib_include
    ],
    dependencies : [
        cereal
    ],
    cpp_args : [
        '-Wfatal-errors',
        '-DWIN32_LEAN_AND_MEAN',
        '-DWINVER=0x501',
        '-D_WIN32_WINNT=0x501',
        '-D_M_IX86',
        '-DNAMED_PIPE="//./pipe/cccaster_pipe"',
        '-DNAMED_PIPE2="//./pipe/cccaster2_pipe"',
        '-DPALETTES_FOLDER="palettes/"',
        '-DMBAA_EXE="MBAA.exe"',
        '-DBINARY="cccaster.v3.1.debug.exe"',
        '-DFOLDER="cccaster/"',
        '-DHOOK_DLL="cccaster/hook.debug.dll"',
        '-DLAUNCHER="cccaster/launcher.exe"',
        '-DRELAY_LIST="relay_list.txt"',
        '-DTAG="debug"',
        '-Ofast',
        '-DNDEBUG',
        '-DRELEASE',
        '-DDISABLE_ASSERTS'
    ],
    link_args : [
        '-lws2_32',
        '-lpsapi',
        '-lwinpthread',
        '-lwinmm',
        '-lole32',
        '-ldinput',
        '-lwininet',
        '-s'
    ]
)

executable(
    'launcher',
    'tools/Launcher.cpp',
    cpp_args : [
        '-O2'
    ],
    link_args : [
        '-s'
    ]
)

configure_file(
    input : 'relay_list.txt',
    output : 'relay_list.txt',
    configuration : configuration_data()
)

configure_file(
    input : 'scripts/Add_Handler_Protocol.bat',
    output : 'Add_Handler_Protocol.bat',
    configuration : configuration_data()
)
