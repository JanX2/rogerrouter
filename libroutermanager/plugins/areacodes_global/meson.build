areacodes_global_sources = []
areacodes_global_sources += 'areacodes_global.c'
areacodes_global_sources += 'csv.c'

areacodes_global_dep = []
areacodes_global_dep += rm_dep

libareacodes_global = shared_library('areacodes_global',
                        areacodes_global_sources,
                        include_directories : rm_inc,
                        dependencies : areacodes_global_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/areacodes_global/')

custom_target('areacodes_global.plugin',
    output : 'areacodes_global.plugin',
    input : 'areacodes_global.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/areacodes_global/')

install_data('share/globalareacodes.csv', install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/areacodes_global/')
