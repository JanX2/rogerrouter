webjournal_sources = []
webjournal_sources += 'webjournal.c'

webjournal_dep = []
webjournal_dep += rm_dep
webjournal_dep += roger_dep

libwebjournal = shared_library('webjournal',
                        webjournal_sources,
                        include_directories : rm_inc,
                        dependencies : webjournal_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

custom_target('webjournal.plugin',
    output : 'webjournal.plugin',
    input : 'webjournal.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')
