indicator = dependency('appindicator3-0.1', required: false)

if indicator.found()
indicator_sources = []
indicator_sources += 'indicator.c'

indicator_dep = []
indicator_dep += plugins_dep
indicator_dep += indicator

libindicator = shared_module('indicator',
                        indicator_sources,
                        include_directories : roger_inc,
                        dependencies : indicator_dep,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')

custom_target('indicator.plugin',
    output : 'indicator.plugin',
    input : 'indicator.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/roger/indicator/')

install_data('org.tabos.roger.plugins.indicator.gschema.xml', install_dir : 'share/glib-2.0/schemas')

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
images += 'images/scalable/roger-mono-dark.svg'
images += 'images/scalable/roger-mono-lite.svg'
images += 'images/scalable/roger-notify.svg'

install_data(images, install_dir : get_option('datadir') + '/icons/hicolor/scalable/status')

endif
