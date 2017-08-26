indicator = dependency('libappindicator', required: false)

if indicator.found()
indicator_sources = []
indicator_sources += 'indicator.c'

indicator_dep = []
indicator_dep += rm_dep
indicator_dep += plugins_dep
indicator_dep += indicator

libindicator = shared_library('indicator',
                        indicator_sources,
                        include_directories : rm_inc,
                        dependencies : indicator_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')

custom_target('indicator.plugin',
    output : 'indicator.plugin',
    input : 'indicator.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')

endif
