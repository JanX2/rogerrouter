wincred_sources = ['wincred.c']
wincred_dep = [rm_dep]

libwincred = shared_library('wincred',
                        wincred_sources,
                        include_directories : rm_inc,
                        dependencies : wincred_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/wincred/')

custom_target('wincred.plugin',
    output : 'wincred.plugin',
    input : 'wincred.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/wincred/')
