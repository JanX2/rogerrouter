evolution_sources = []
evolution_sources += 'evolution.c'

evolution_dep = []
evolution_dep += routermanager_dep
evolution_dep += roger_dep
evolution_dep += dependency('libebook-1.2')

libevolution = shared_library('evolution',
                        evolution_sources,
                        include_directories : routermanager_inc,
                        dependencies : evolution_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/evolution/')

custom_target('evolution.plugin',
    output : 'evolution.plugin',
    input : 'evolution.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/evolution/')
