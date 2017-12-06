statusicon_sources = []
statusicon_sources += 'statusicon.c'

libstatusicon = shared_module('statusicon',
                        statusicon_sources,
                        include_directories : roger_inc,
                        dependencies : plugins_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')

custom_target('statusicon.plugin',
    output : 'statusicon.plugin',
    input : 'statusicon.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/statusicon/')

install_data('org.tabos.roger.plugins.statusicon.gschema.xml', install_dir : 'share/glib-2.0/schemas')

icon_sizes = ['16x16', '22x22', '24x24', '32x32', '48x48']

# install bitmap icons
foreach size : icon_sizes
    install_data('images/' + size + '/roger-default.png',
                 install_dir: 'share/icons/hicolor/' + size + '/status/')
    install_data('images/' + size + '/roger-notify.png',
                 install_dir: 'share/icons/hicolor/' + size + '/status/')
    install_data('images/' + size + '/roger-mono-dark.png',
                 install_dir: 'share/icons/hicolor/' + size + '/status/')
    install_data('images/' + size + '/roger-mono-lite.png',
                 install_dir: 'share/icons/hicolor/' + size + '/status/')
endforeach

images = []
images += 'images/scalable/roger-default.svg'
images += 'images/scalable/roger-notify.svg'

install_data(images, install_dir : get_option('datadir') + '/icons/hicolor/scalable/status')
