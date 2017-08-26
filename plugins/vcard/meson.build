vcard_sources = []
vcard_sources += 'vcard.c'

vcard_dep = []
vcard_dep += plugins_dep

vcard_inc = [roger_inc]

libvcard = shared_library('vcard',
                        vcard_sources,
                        include_directories : vcard_inc,
                        dependencies : vcard_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/vcard/')

custom_target('vcard.plugin',
    output : 'vcard.plugin',
    input : 'vcard.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/vcard/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.vcard.gschema.xml',
install_dir : 'share/glib-2.0/schemas')
