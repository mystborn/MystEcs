project('myst_ecs', 'c', version: '0.0.2', license: 'MIT')

c_comp = meson.get_compiler('c')
x86 = get_option('x86')
check_location = get_option('check_location')

args = []
link_args = []

if x86
    if build_machine.system() == 'windows'
        args += '/arch:SSE2'
        link_args += '/MACHINE:X86'
    else
        args += '-march=i386'
    endif
else
    if build_machine.system() == 'windows'
        args += '/arch:AVX2'
        link_args += '/MACHINE:X64'
    else
        args += '-march=x86-64'
    endif
endif


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
                              link_args: link_args,
                              c_args: args,
                              name_suffix: 'lib')

    shared_library('myst_ecs',
                   lib_sources,
                   include_directories: inc,
                   link_args: link_args,
                   c_args: args)
else
    myst_ecs = both_libaries('myst_ecs',
                             lib_sources,
                             include_directories: inc,
                             link_args: link_args,
                             c_args: args)

endif

if check_location != ''
    subdir('test')
endif