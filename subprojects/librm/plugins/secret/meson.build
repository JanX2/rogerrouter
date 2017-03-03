secret = dependency('libsecret-1', required: false)

if secret.found()

secret_sources = ['secret.c']

secret_dep = []
secret_dep += rm_dep
secret_dep += secret

libsecret = shared_library('secret',
                        secret_sources,
                        include_directories : rm_inc,
                        dependencies : secret_dep,
                        install : true,
                        install_dir : rm_plugins_path + '/secret/')

custom_target('secret.plugin',
    output : 'secret.plugin',
    input : 'secret.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', rm_podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : rm_plugins_path + '/secret/')

endif