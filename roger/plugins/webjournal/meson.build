webjournal_sources = []
webjournal_sources += 'webjournal.c'

webjournal_dep = []
webjournal_dep += rm_dep
webjournal_dep += roger_dep

libwebjournal = shared_library('webjournal',
                        webjournal_sources,
                        include_directories : rm_inc,
                        dependencies : webjournal_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

custom_target('webjournal.plugin',
    output : 'webjournal.plugin',
    input : 'webjournal.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.webjournal.gschema.xml',
install_dir : 'share/glib-2.0/schemas')

libwebjournal_data = []
libwebjournal_data += 'share/dragtable.js'
libwebjournal_data += 'share/entry.html'
libwebjournal_data += 'share/footer.html'
libwebjournal_data += 'share/sortable.js'
libwebjournal_data += 'share/styling.css'

install_data(libwebjournal_data, install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

