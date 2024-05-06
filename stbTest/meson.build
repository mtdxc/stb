project('stbTest', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++14'])

include_dirs = include_directories('..')
executable('stbTest',
           'stbTest.cpp',
           include_directories: include_dirs,
           install : true)
executable('VxTest',
           'VxText.cpp', 'VxText.h',
           include_directories: include_dirs,
           install : true)