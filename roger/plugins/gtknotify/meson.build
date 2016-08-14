gtknotify_sources = []
gtknotify_sources += 'gtknotify.c'

gtknotify_dep = []
gtknotify_dep += routermanager_dep
gtknotify_dep += roger_dep
gtknotify_dep += dependency('libebook-1.2')

libgtknotify = shared_library('gtknotify',
                        gtknotify_sources,
                        include_directories : routermanager_inc,
                        dependencies : gtknotify_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gtknotify/')

custom_target('gtknotify.plugin',
    output : 'gtknotify.plugin',
    input : 'gtknotify.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/gtknotify/')
