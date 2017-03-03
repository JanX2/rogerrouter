portaudio = dependency('portaudio-2.0', required: false)

if portaudio.found()

portaudio_sources = ['portaudio.c']
portaudio_dep = []
portaudio_dep += portaudio
portaudio_dep += rm_dep

libportaudio = shared_library('portaudio',
                        portaudio_sources,
                        include_directories : rm_inc,
                        dependencies : portaudio_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/portaudio/')

custom_target('portaudio.plugin',
    output : 'portaudio.plugin',
    input : 'portaudio.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/portaudio/')

endif