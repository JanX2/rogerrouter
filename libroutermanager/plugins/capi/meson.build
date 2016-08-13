capi_sources = []
capi_sources += 'capi.c'
capi_sources += 'fax.c'
capi_sources += 'isdn-convert.c'
capi_sources += 'phone.c'
capi_sources += 'ringtone.c'
capi_sources += 'sff.c'

capi_dep = []
capi_dep += routermanager_dep
capi_dep += dependency('sndfile')
capi_dep += dependency('capi20')

libcapi = shared_library('capi',
                        capi_sources,
                        include_directories : routermanager_inc,
                        dependencies : capi_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/capi/')

custom_target('capi.plugin',
    output : 'capi.plugin',
    input : 'capi.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/capi/')
