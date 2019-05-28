# MystEcs
MystEcs is an [entity-component-system](https://en.wikipedia.org/wiki/Entity_component_system) written in C. The library is usable, but the api is not stable quite yet.

# Building
This project uses [Meson](https://mesonbuild.com/) as it's build system, so apologies to the CMake users. (Also, I've never used any compiler but MSVC, so the compiler flags may be wrong for other compilers). That being said, building MystEcs is very easy. Just set up your build directory using `meson path/to/ecs/dir` and then `ninja` will build the project. It supports the following [build options](https://mesonbuild.com/Build-options.html):

| Option | Type | Description | Default |
| --- | --- | --- | --- |
| check_location | string | Path to [check](https://libcheck.github.io/check/web/install.html) unit test library. Leave blank to exclude tests | '' |