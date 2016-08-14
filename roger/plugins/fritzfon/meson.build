fritzfon_sources = []
fritzfon_sources += 'fritzfon.c'

fritzfon_dep = []
fritzfon_dep += routermanager_dep
fritzfon_dep += roger_dep
fritzfon_dep += dependency('libebook-1.2')

libfritzfon = shared_library('fritzfon',
                        fritzfon_sources,
                        include_directories : routermanager_inc,
                        dependencies : fritzfon_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/fritzfon/')

custom_target('fritzfon.plugin',
    output : 'fritzfon.plugin',
    input : 'fritzfon.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/fritzfon/')
