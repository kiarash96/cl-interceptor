project('cl-interceptor', 'cpp',
  version : '0.0.0',
  default_options : ['cpp_std=c++20'])

boost_dep = dependency('boost', modules : ['program_options', 'filesystem'])

executable('cl', 'cl.cpp', dependencies : [boost_dep])
executable('intercept', 'intercept.cpp', dependencies : [boost_dep])
