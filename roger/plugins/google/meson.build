google_sources = []
google_sources += 'google.c'

google_dep = []
google_dep += routermanager_dep
google_dep += roger_dep
google_dep += dependency('libgdata')

libgoogle = shared_library('google',
                        google_sources,
                        include_directories : routermanager_inc,
                        dependencies : google_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/google/')

custom_target('google.plugin',
    output : 'google.plugin',
    input : 'google.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/google/')
