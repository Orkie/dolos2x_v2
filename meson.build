project('dolos2x', 'c')

libpetraea_dep = dependency('libpetraea', fallback : ['petraea', 'libpetraea_dep'])
libsdl2_dep = dependency('sdl2')

#################################################
# Sources
#################################################

sources = [
  'src/main.c'
]

#################################################
# Build
#################################################

executable('dolos2x',
  sources,
  dependencies : [libpetraea_dep, libsdl2_dep],
  install : true)

