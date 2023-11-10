project(
    'first_telegram_bot',
    'cpp',
    version: 'v1.0.0',
    default_options: [ 'cpp_std=c++14', 'warning_level=3' ]
)

cxx = meson.get_compiler( 'cpp' )

argument_list = []

cxxopts = dependency( 'cxxopts' )
nlohmann_json = dependency( 'nlohmann_json' )

# asio = dependency( 'asio' )
fmt = dependency( 'fmt' )
# openssl = dependency( 'openssl' )

# host_include = include_directories( 'host/include' )

add_project_arguments( argument_list, language: 'cpp' )

if cxx.get_id() == 'gcc' or cxx.get_id() == 'clang'
    test_warning_argument_list = [
        # It's useful not to have to initialize all struct fields when defining a
        # JSClass, since some of them are padding.
        '-Wno-missing-field-initializers',

        # We use argument names in some cases for explanatory purposes
        '-Wno-unused-parameter',
    ]
else
    test_warning_argument_list = []
endif

add_project_arguments(
    cxx.get_supported_arguments( test_warning_argument_list ),
    language: 'cpp'
)

# host = declare_dependency(
#     compile_args : argument_list + test_warning_argument_list,
#     dependencies : [ asio, fmt, openssl ],
#     include_directories : [ host_include ],
#     sources : [ 'host/src/host.cpp', 'host/src/session.cpp' ],
#     version : '1.0.0'
# )

executable(
    'main',
    'src/main.cpp',
    dependencies : [ cxxopts, fmt, nlohmann_json ],
    include_directories : []
)
