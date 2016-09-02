indicator_sources = []
indicator_sources += 'indicator.c'

indicator_dep = []
indicator_dep += rm_dep
indicator_dep += roger_dep
indicator_dep += dependency('libebook-1.2')

libindicator = shared_library('indicator',
                        indicator_sources,
                        include_directories : rm_inc,
                        dependencies : indicator_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')

custom_target('indicator.plugin',
    output : 'indicator.plugin',
    input : 'indicator.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')
