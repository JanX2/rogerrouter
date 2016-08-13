simple_pwd_sources = []
simple_pwd_sources += 'simple_pwd.c'

simple_pwd_dep = []
simple_pwd_dep += routermanager_dep

libsimple_pwd = shared_library('simple_pwd',
                        simple_pwd_sources,
                        include_directories : routermanager_inc,
                        dependencies : simple_pwd_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/simple_pwd/')

custom_target('simple_pwd.plugin',
    output : 'simple_pwd.plugin',
    input : 'simple_pwd.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/simple_pwd/')
