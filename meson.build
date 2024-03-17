project('myst_ecs', 'c', version: '0.0.2', license: 'MIT')

c_comp = meson.get_compiler('c')
check_location = get_option('check_location')

include_files = ['include']

inc = include_directories(include_files)

if check_location != '' and check_location != 'true'
    test_inc = include_directories(include_files + [check_location + '/include'])
endif

args = ['/DECS_BUILD']

subdir('src')

# Needed because visual studio fails to link with
# a static library that ends with '.a'
# so we need to specify the suffix.

if build_machine.system() == 'windows'

    myst_ecs = static_library('myst_ecs',
        lib_sources,
        include_directories: inc,
        name_suffix: 'lib',
        name_prefix: '',
        c_args: args
    )

    myst_ecs_shared = shared_library('myst_ecs',
        lib_sources,
        include_directories: inc,
        c_args: args
    )
else
    myst_ecs = static_library('myst_ecs',
        lib_sources,
        include_directories: inc,
        c_args: args
    )

    myst_ecs_shared = shared_library('myst_ecs',
        lib_sources,
        include_directories: inc,
        c_args: args
    )
endif

myst_ecs_dep = declare_dependency(include_directories: inc,
    link_with: myst_ecs_shared
)

if check_location != ''
    subdir('test')
endif