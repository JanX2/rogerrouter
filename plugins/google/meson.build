gdata = dependency('libgdata', required: false)

if gdata.found()
google_sources = []
google_sources += 'google.c'

google_dep = []
google_dep += plugins_dep
google_dep += gdata

libgoogle = shared_library('google',
                        google_sources,
                        include_directories : roger_inc,
                        dependencies : google_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/google/')

custom_target('google.plugin',
    output : 'google.plugin',
    input : 'google.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/google/')

gnome.compile_schemas()
install_data('org.tabos.roger.plugins.google.gschema.xml',
install_dir : 'share/glib-2.0/schemas')
endif
