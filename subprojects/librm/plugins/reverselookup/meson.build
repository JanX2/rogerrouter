reverselookup_sources = ['reverselookup.c']

reverselookup_dep = []
reverselookup_dep += rm_dep

libreverselookup = shared_library('reverselookup',
                        reverselookup_sources,
                        include_directories : rm_inc,
                        dependencies : reverselookup_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/reverselookup/')

custom_target('reverselookup.plugin',
    output : 'reverselookup.plugin',
    input : 'reverselookup.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/reverselookup/')

install_data('share/lookup.xml', install_dir : rm_plugins_path + '/reverselookup/')
