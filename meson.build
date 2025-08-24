project('stdexec-gtk-example', ['c', 'cpp'],
        license : 'GPLv3+',
        version : '0.0.1',
        default_options : ['c_std=c17',
                           'cpp_std=c++20',
                           'buildtype=release'])

cpp_compiler = meson.get_compiler('cpp')
if cpp_compiler.get_argument_syntax() == 'msvc'
  if cpp_compiler.has_argument('/std:c++23')
    cpp23_args = ['/std:c++23']
  else
    cpp23_args = ['/std:c++latest']
  endif
else
  if cpp_compiler.has_argument('-std=c++23')
    cpp23_args = ['-std=c++23']
  else
    cpp23_args = ['-std=c++2b']
  endif
endif
add_global_arguments(cpp23_args, language: 'cpp')

cpp_compiler = meson.get_compiler('cpp')

gtkmm_dep  = dependency('gtkmm-4.0')

curl_dep = dependency('libcurl')

stdexec_dep = dependency('stdexec', fallback: ['stdexec'])

subdir('glib')
