project(
    'keyboard_addon',
    [ 'cpp' ],
    version: 'v1.0.0',
    default_options: [ 'cpp_std=c++11', 'warning_level=3' ],
    license: 'GPL-3.0'
)

cxx = meson.get_compiler( 'cpp' )

if host_machine.system() == 'windows'
    compile_argument_list = [ '-Ofast', '-fpermissive' ]

    link_argument_list = [
        'kernel32.lib', 'user32.lib', 'gdi32.lib',
        'winspool.lib', 'comdlg32.lib', 'advapi32.lib',
        'shell32.lib', 'ole32.lib', 'oleaut32.lib',
        'uuid.lib', 'odbc32.lib', 'odbccp32.lib',
        'ws2_32.lib', 'mswsock.lib'
    ]

else
    compile_argument_list = []

    link_argument_list = []

endif

fmt = dependency( 'fmt' )
nlohmann_json = dependency( 'nlohmann_json' )

addons_include = include_directories( '../../' )
addon_callbacks_include = include_directories( 'addon_callbacks/include' )

addon_callbacks = declare_dependency(
    compile_args : compile_argument_list,
    dependencies : [ fmt ],
    include_directories : [ addons_include, addon_callbacks_include ],
    sources : [ 'addon_callbacks/src/addon_callbacks.cpp' ],
    version : '1.0.0'
)

if host_machine.system() == 'windows'
    shared_library(
        'main',
        'src/main.cpp',
        link_args: link_argument_list,
        dependencies : [ fmt, nlohmann_json, addon_callbacks ],
        include_directories : [ addons_include, addon_callbacks_include ]
    )

endif
