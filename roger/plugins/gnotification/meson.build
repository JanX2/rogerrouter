gnotification_sources = []
gnotification_sources += 'gnotification.c'

gnotification_dep = []
gnotification_dep += rm_dep
gnotification_dep += roger_dep

gnotification_inc = [rm_inc, roger_inc]

libgnotification = shared_library('gnotification',
                        gnotification_sources,
                        include_directories : gnotification_inc,
                        dependencies : gnotification_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gnotification/')

custom_target('gnotification.plugin',
    output : 'gnotification.plugin',
    input : 'gnotification.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gnotification/')
