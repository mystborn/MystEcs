check_lib = check_location + '/lib'

compat = c_comp.find_library('compat',
                             required: true,
                             dirs: check_lib)

check = c_comp.find_library('check',
                            required: true,
                            dirs: check_lib)

deps = [compat, check]

dispenser_test = executable('dispenser_test',
                            'ecs_dispenser_test.c',
                            link_with: myst_ecs,
                            link_args: link_args,
                            include_directories: test_inc,
                            dependencies: deps)

world_test = executable('world_test',
                        'ecs_world_test.c',
                        link_with: myst_ecs,
                        link_args: link_args,
                        include_directories: test_inc,
                        dependencies: deps)

component_test = executable('component_test',
                            'ecs_component_test.c',
                            link_with: myst_ecs,
                            link_args: link_args,
                            include_directories: test_inc,
                            dependencies: deps)

entity_builder_test = executable('entity_builder_test',
                                 'ecs_entity_set_test.c',
                                 link_with: myst_ecs,
                                 link_args: link_args,
                                 include_directories: test_inc,
                                 dependencies: deps)

system_test = executable('system_test',
                         'ecs_system_test.c',
                         link_with: myst_ecs,
                         link_args: link_args,
                         include_directories: test_inc,
                         dependencies: deps)

impl_test = executable('entity_system_impl',
                       'ecs_entity_system_impl.c',
                       link_with: myst_ecs,
                       link_args: link_args,
                       include_directories: inc)

test('Dispenser Test', dispenser_test)
test('World Test', world_test)
test('Component Test', component_test)
test('Entity Set Test', entity_builder_test)
test('System Test', system_test)