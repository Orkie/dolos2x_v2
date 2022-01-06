project('dolos2x', 'c')

libpetraea_dep = dependency('libpetraea', fallback : ['petraea', 'libpetraea_dep'])

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
  dependencies : [libpetraea_dep],
  install : true)

