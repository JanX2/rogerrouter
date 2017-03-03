dep = dependency('libebook-1.2', required: false)

if dep.found()

evolution_sources = []
evolution_sources += 'evolution.c'

evolution_dep = []
evolution_dep += rm_dep
evolution_dep += roger_dep
evolution_dep += dependency('libebook-1.2')

evolution_inc = []
evolution_inc += rm_inc
evolution_inc += include_directories('../../..')

libevolution = shared_library('evolution',
                        evolution_sources,
                        include_directories : evolution_inc,
                        dependencies : evolution_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/evolution/')

custom_target('evolution.plugin',
    output : 'evolution.plugin',
    input : 'evolution.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/evolution/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.evolution.gschema.xml',
install_dir : join_paths(get_option('datadir'), 'glib-2.0', 'schemas'))
endif