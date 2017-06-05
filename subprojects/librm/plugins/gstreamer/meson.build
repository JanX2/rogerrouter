gstreamer = dependency('gstreamer-1.0', required: false)

if gstreamer.found()

gstreamer_sources = ['gstreamer.c']

gstreamer_dep = []
gstreamer_dep += rm_dep
gstreamer_dep += gstreamer
gstreamer_dep += dependency('gstreamer-app-1.0')

libgstreamer = shared_library('gstreamer',
                        gstreamer_sources,
                        include_directories : rm_inc,
                        dependencies : gstreamer_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/gstreamer/')

custom_target('gstreamer.plugin',
    output : 'gstreamer.plugin',
    input : 'gstreamer.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/gstreamer/')

endif