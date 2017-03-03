areacodes_global_sources = [
	'areacodes_global.c',
	'csv.c'
]

areacodes_global_dep = [rm_dep]

libareacodes_global = shared_library('areacodes_global',
                        areacodes_global_sources,
                        include_directories : rm_inc,
                        dependencies : areacodes_global_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/areacodes_global/')

custom_target('areacodes_global.plugin',
    output : 'areacodes_global.plugin',
    input : 'areacodes_global.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/areacodes_global/')

install_data('share/globalareacodes.csv', install_dir : rm_plugins_path + '/areacodes_global/')
