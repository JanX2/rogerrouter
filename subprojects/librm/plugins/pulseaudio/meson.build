pulseaudio = dependency('libpulse-simple', required: false)

if pulseaudio.found()

pulseaudio_sources = ['pulseaudio.c']

pulseaudio_dep = []
pulseaudio_dep += rm_dep
pulseaudio_dep += pulseaudio

libpulseaudio = shared_library('pulseaudio',
                        pulseaudio_sources,
                        include_directories : rm_inc,
                        dependencies : pulseaudio_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/pulseaudio/')

custom_target('pulseaudio.plugin',
    output : 'pulseaudio.plugin',
    input : 'pulseaudio.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/pulseaudio/')

endif
