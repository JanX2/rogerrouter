gnotification_sources = []
gnotification_sources += 'gnotification.c'

gnotification_dep = []
gnotification_dep += rm_dep
gnotification_dep += roger_dep

libgnotification = shared_library('gnotification',
                        gnotification_sources,
                        include_directories : rm_inc,
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
