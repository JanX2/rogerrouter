callmonitor_sources = []
callmonitor_sources += 'callmonitor.c'

callmonitor_dep = []
callmonitor_dep += rm_dep

libcallmonitor = shared_library('callmonitor',
                        callmonitor_sources,
                        include_directories : rm_inc,
                        dependencies : callmonitor_dep,
                        link_with : libroutermanager,
                        install : true,
                        install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/callmonitor/')

custom_target('callmonitor.plugin',
    output : 'callmonitor.plugin',
    input : 'callmonitor.plugin.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('prefix') + '/' + get_option('libdir') + '/routermanager/callmonitor/')
