project('dolos2x', 'c')

libpetraea_dep = dependency('libpetraea', fallback : ['petraea', 'libpetraea_dep'])
libsdl2_dep = dependency('sdl2')
glib_dep = dependency('glib-2.0')

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
  dependencies : [libpetraea_dep, libsdl2_dep, glib_dep],
  install : true)

