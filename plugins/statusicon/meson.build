statusicon_sources = []
statusicon_sources += 'statusicon.c'

libstatusicon = shared_library('statusicon',
                        statusicon_sources,
                        include_directories : roger_inc,
                        dependencies : plugins_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')

custom_target('statusicon.plugin',
    output : 'statusicon.plugin',
    input : 'statusicon.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.statusicon.gschema.xml',
install_dir : 'share/glib-2.0/schemas')

