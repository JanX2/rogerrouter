routermanager_sources = []
routermanager_sources += 'rmaction.c'
routermanager_sources += 'rmaddressbook.c'
routermanager_sources += 'rmaudio.c'
routermanager_sources += 'rmcall.c'
routermanager_sources += 'rmcontact.c'
routermanager_sources += 'rmconnection.c'
routermanager_sources += 'rmcsv.c'
routermanager_sources += 'rmdevice.c'
routermanager_sources += 'rmdevicefax.c'
routermanager_sources += 'rmfaxspooler.c'
routermanager_sources += 'rmfaxserver.c'
routermanager_sources += 'rmfile.c'
routermanager_sources += 'rmfilter.c'
routermanager_sources += 'ftp.c'
routermanager_sources += 'rmstring.c'
routermanager_sources += 'rmlog.c'
routermanager_sources += 'rmlookup.c'
routermanager_sources += 'rmnetmonitor.c'
routermanager_sources += 'rmnetwork.c'
routermanager_sources += 'rmobject.c'
routermanager_sources += 'rmobjectemit.c'
routermanager_sources += 'rmprofile.c'
routermanager_sources += 'rmmain.c'
routermanager_sources += 'rmosdep.c'
routermanager_sources += 'rmpassword.c'
routermanager_sources += 'rmplugins.c'
routermanager_sources += 'rmprofile.c'
routermanager_sources += 'rmphone.c'
routermanager_sources += 'router.c'
routermanager_sources += 'rmsettings.c'
routermanager_sources += 'rmssdp.c'
routermanager_sources += 'rmvox.c'
routermanager_sources += 'xml.c'

routermanager_inc = include_directories('..')
#include_directories('libroutermanager')

routermanager_dep = []
routermanager_dep += dependency('glib-2.0')
routermanager_dep += dependency('libpeas-1.0')
routermanager_dep += dependency('libsoup-2.4')
routermanager_dep += dependency('gupnp-1.0')
routermanager_dep += dependency('gssdp-1.0')
routermanager_dep += dependency('speex')
routermanager_dep += dependency('libpeas-1.0')
routermanager_dep += dependency('libxml-2.0')
routermanager_dep += dependency('spandsp')
routermanager_dep += dependency('json-glib-1.0')

libroutermanager = shared_library('routermanager',
                        routermanager_sources,
                        include_directories : routermanager_inc,
                        dependencies : routermanager_dep,
                        install : true,
                        link_args : '-lcrypt')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libroutermanager,
                 version : '1.2',
                 name : 'libroutermanager',
                 filebase : 'libroutermanager',
                 description : 'A library to handle AVM FRITZ!Box routers.')

subdir('plugins')