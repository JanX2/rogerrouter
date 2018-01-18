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

images = []
images += 'images/scalable/org.tabos.roger.default.svg'
images += 'images/scalable/org.tabos.roger.mono-dark.svg'
images += 'images/scalable/org.tabos.roger.mono-lite.svg'
images += 'images/scalable/org.tabos.roger.notify.svg'

install_data(images, install_dir : get_option('datadir') + '/icons/hicolor/scalable/status/')

endif
