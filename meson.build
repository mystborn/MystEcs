project('myst_ecs', 'c', version: '0.0.2', license: 'MIT')

c_comp = meson.get_compiler('c')
check_location = get_option('check_location')

include_files = ['include']

inc = include_directories(include_files)

if check_location != ''
    test_inc = include_directories(include_files + [check_location + '/include'])
endif

subdir('src')

# Needed because visual studio fails to link with
# a static library that ends with '.a'
# so we need to specify the suffix.

if build_machine.system() == 'windows'
    myst_ecs = static_library('myst_ecs',
                              lib_sources,
                              include_directories: inc,
                              name_suffix: 'lib')

    shared_library('myst_ecs',
                   lib_sources,
                   include_directories: inc)
else
    myst_ecs = static_library('myst_ecs',
                              lib_sources,
                              include_directories: inc)

    shared_library('myst_ecs',
                   lib_sources,
                   include_directories: inc)
endif

if check_location != ''
    subdir('test')
endif