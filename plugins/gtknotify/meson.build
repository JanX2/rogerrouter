gtknotify_gresources = gnome.compile_resources(
  'gtknotify-gresources', 'gtknotify.gresource.xml',
  source_dir : '.',
  c_name : '')


gtknotify_sources = []
gtknotify_sources += 'gtknotify.c'
gtknotify_sources += gtknotify_gresources

gtknotify_dep = []
gtknotify_dep += plugins_dep

gtknotify_inc = [roger_inc]

libgtknotify = shared_library('gtknotify',
                        gtknotify_sources,
                        include_directories : gtknotify_inc,
                        dependencies : gtknotify_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gtknotify/')

custom_target('gtknotify.plugin',
    output : 'gtknotify.plugin',
    input : 'gtknotify.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/gtknotify/')

install_data('org.tabos.roger.plugins.gtknotify.gschema.xml', install_dir : 'share/glib-2.0/schemas')
