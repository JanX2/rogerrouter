gstreamer_sources = []
gstreamer_sources += 'gstreamer.c'

gstreamer_dep = []
gstreamer_dep += routermanager_dep
gstreamer_dep += dependency('gstreamer-1.0')
gstreamer_dep += dependency('gstreamer-app-1.0')

libgstreamer = shared_library('gstreamer',
                        gstreamer_sources,
                        include_directories : routermanager_inc,
                        dependencies : gstreamer_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/gstreamer/')

custom_target('gstreamer.plugin',
    output : 'gstreamer.plugin',
    input : 'gstreamer.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/gstreamer/')
