fritzbox_sources = []
fritzbox_sources += 'callmonitor.c'
fritzbox_sources += 'csv.c'
fritzbox_sources += 'firmware-04-00.c'
fritzbox_sources += 'firmware-04-74.c'
fritzbox_sources += 'firmware-05-50.c'
fritzbox_sources += 'firmware-06-35.c'
fritzbox_sources += 'firmware-common.c'
fritzbox_sources += 'firmware-query.c'
fritzbox_sources += 'fritzbox.c'

libfritzbox = shared_library('fritzbox',
                        fritzbox_sources,
                        include_directories : rm_inc,
                        dependencies : rm_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/fritzbox/')

custom_target('fritzbox.plugin',
    output : 'fritzbox.plugin',
    input : 'fritzbox.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/fritzbox/')

gnome.compile_schemas()
install_data('org.tabos.routermanager.plugins.fritzbox.gschema.xml',
install_dir : 'share/glib-2.0/schemas')
