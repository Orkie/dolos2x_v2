project('dolos2x', 'c')

libpetraea_dep = dependency('libpetraea', fallback : ['petraea', 'libpetraea_dep'])
libsdl2_dep = dependency('sdl2')
glib_dep = dependency('glib-2.0')

inc = include_directories('include')

#################################################
# Sources
#################################################

sources = [
  'src/main.c',
  'src/nand.c',
  'src/clock.c',
  'src/timer.c',
  'src/uart.c',
  'src/video.c',
  'src/gpio.c'
]

#################################################
# Build
#################################################

executable('dolos2x',
  sources,
  include_directories : inc,
  dependencies : [libpetraea_dep, libsdl2_dep, glib_dep],
  install : true)

