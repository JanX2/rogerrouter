sip_sources = []
sip_sources += 'sip.c'

sip_dep = []
sip_dep += rm_dep
sip_dep += roger_dep
sip_dep += dependency('libpjproject')

libsip = shared_library('sip',
                        sip_sources,
                        include_directories : rm_inc,
                        dependencies : sip_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/sip/')

custom_target('sip.plugin',
    output : 'sip.plugin',
    input : 'sip.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/sip/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.sip.gschema.xml',
install_dir : 'share/glib-2.0/schemas')
