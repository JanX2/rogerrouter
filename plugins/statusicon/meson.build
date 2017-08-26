statusicon_sources = []
statusicon_sources += 'statusicon.c'

statusicon_dep = []
statusicon_dep += rm_dep
statusicon_dep += plugins_dep

statusicon_inc = [rm_inc, roger_inc]

libstatusicon = shared_library('statusicon',
                        statusicon_sources,
                        include_directories : statusicon_inc,
                        dependencies : statusicon_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')

custom_target('statusicon.plugin',
    output : 'statusicon.plugin',
    input : 'statusicon.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')
