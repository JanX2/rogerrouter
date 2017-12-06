webjournal_gresources = gnome.compile_resources(
  'webjournal-gresources', 'webjournal.gresource.xml',
  source_dir : '.',
  c_name : '')

webjournal_sources = []
webjournal_sources += 'webjournal.c'
webjournal_sources += webjournal_gresources

webjournal_dep = []
webjournal_dep += plugins_dep

webjournal_inc = [roger_inc]

libwebjournal = shared_module('webjournal',
                        webjournal_sources,
                        include_directories : webjournal_inc,
                        dependencies : webjournal_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

custom_target('webjournal.plugin',
    output : 'webjournal.plugin',
    input : 'webjournal.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/webjournal/')

install_data('org.tabos.roger.plugins.webjournal.gschema.xml', install_dir : 'share/glib-2.0/schemas')


