gnotification_sources = []
gnotification_sources += 'gnotification.c'

gnotification_dep = []
gnotification_dep += routermanager_dep
gnotification_dep += roger_dep
gnotification_dep += dependency('libebook-1.2')

libgnotification = shared_library('gnotification',
                        gnotification_sources,
                        include_directories : routermanager_inc,
                        dependencies : gnotification_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gnotification/')

custom_target('gnotification.plugin',
    output : 'gnotification.plugin',
    input : 'gnotification.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gnotification/')
