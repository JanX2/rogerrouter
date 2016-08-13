routermanager_sources = []
routermanager_sources += 'rmaction.c'
routermanager_sources += 'address-book.c'
routermanager_sources += 'appobject-emit.c'
routermanager_sources += 'appobject.c'
routermanager_sources += 'audio.c'
routermanager_sources += 'call.c'
routermanager_sources += 'device.c'
routermanager_sources += 'device_fax.c'
routermanager_sources += 'contact.c'
routermanager_sources += 'rmconnection.c'
routermanager_sources += 'csv.c'
routermanager_sources += 'fax_spooler.c'
routermanager_sources += 'fax_server.c'
routermanager_sources += 'file.c'
routermanager_sources += 'filter.c'
routermanager_sources += 'ftp.c'
routermanager_sources += 'gstring.c'
routermanager_sources += 'logging.c'
routermanager_sources += 'lookup.c'
routermanager_sources += 'net_monitor.c'
routermanager_sources += 'network.c'
routermanager_sources += 'rmprofile.c'
routermanager_sources += 'rmmain.c'
routermanager_sources += 'rmosdep.c'
routermanager_sources += 'password.c'
routermanager_sources += 'plugins.c'
routermanager_sources += 'rmprofile.c'
routermanager_sources += 'rmphone.c'
routermanager_sources += 'router.c'
routermanager_sources += 'settings.c'
routermanager_sources += 'ssdp.c'
routermanager_sources += 'voxplay.c'
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
                        install : true)

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libroutermanager,
                 version : '1.2',
                 name : 'libroutermanager',
                 filebase : 'libroutermanager',
                 description : 'A library to handle AVM FRITZ!Box routers.')

subdir('plugins')