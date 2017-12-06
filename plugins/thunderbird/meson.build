thunderbird_sources = []
thunderbird_sources += 'thunderbird.c'

thunderbird_dep = []
thunderbird_dep += plugins_dep

thunderbird_inc = [roger_inc]

libthunderbird = shared_module('thunderbird',
                        thunderbird_sources,
                        include_directories : thunderbird_inc,
                        dependencies : thunderbird_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/thunderbird/')

custom_target('thunderbird.plugin',
    output : 'thunderbird.plugin',
    input : 'thunderbird.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/thunderbird/')

install_data('org.tabos.roger.plugins.thunderbird.gschema.xml', install_dir : 'share/glib-2.0/schemas')
