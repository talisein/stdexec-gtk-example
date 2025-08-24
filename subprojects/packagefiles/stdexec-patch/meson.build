project('stdexec', ['cpp'],
        license : 'Apache-2.0',
        version : '23.03')

stdexec_inc = include_directories('include')
stdexec_dep = declare_dependency(
  link_args : [],
  compile_args : ['-fcoroutines', '-Wno-non-template-friend'],
  include_directories : stdexec_inc
)

meson.override_dependency('stdexec', stdexec_dep)

subdir('examples')
catch2_dep = dependency('catch2', fallback: ['catch2'])
subdir('test')
