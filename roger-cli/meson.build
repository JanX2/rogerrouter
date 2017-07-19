sourcelist = []
sourcelist += 'main_cli.c'

roger_dep = []
roger_dep += dependency('libsoup-2.4')
roger_dep += dependency('libtiff-4')
#roger_dep += dependency('spandsp')
roger_dep += cc.find_library('m', required: true)
roger_dep += dependency('librm')

executable('roger-cli', sources : sourcelist, dependencies : roger_dep, include_directories : roger_inc, install : true)
