reverselookup_sources = []
reverselookup_sources += 'reverselookup.c'

reverselookup_dep = []
reverselookup_dep += routermanager_dep

libreverselookup = shared_library('reverselookup',
                        reverselookup_sources,
                        include_directories : routermanager_inc,
                        dependencies : reverselookup_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/reverselookup/')

custom_target('reverselookup.plugin',
    output : 'reverselookup.plugin',
    input : 'reverselookup.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/reverselookup/')

install_data('share/lookup.xml', install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/reverselookup/')