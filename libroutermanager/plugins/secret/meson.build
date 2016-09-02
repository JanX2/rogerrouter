secret_sources = []
secret_sources += 'secret.c'

secret_dep = []
secret_dep += rm_dep
secret_dep += dependency('libsecret-1')

libsecret = shared_library('secret',
                        secret_sources,
                        include_directories : rm_inc,
                        dependencies : secret_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/secret/')

custom_target('secret.plugin',
    output : 'secret.plugin',
    input : 'secret.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/secret/')
