project('roger', 'c', version : '1.9.2', license : 'GPL2')

msgfmt = find_program('msgfmt')

add_global_arguments('-DUSE_PRINTER_SPOOLER', language : 'c')
add_global_link_arguments('-lcrypt', language : 'c')
add_global_link_arguments('-lm', language : 'c')

name = 'roger'
version = '1.9.2'

cdata = configuration_data()
cdata.set('PACKAGE_NAME', '"Roger Router"')
cdata.set('PACKAGE_VERSION', '"@0@"'.format(version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')
cdata.set('GETTEXT_PACKAGE', '"roger"')
cdata.set('APP_DATA', '"' + get_option('prefix') + '/' + get_option('datadir') + '"')
cdata.set('APP_LOCALE', '"' + get_option('prefix') + '/' + get_option('localedir') + '"')
cdata.set('APP_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/roger/"')
cdata.set('RM_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/routermanager/"')

configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)

subdir('po')

routermanager_sources = []
routermanager_sources += 'libroutermanager/rmaction.c'
routermanager_sources += 'libroutermanager/address-book.c'
routermanager_sources += 'libroutermanager/appobject-emit.c'
routermanager_sources += 'libroutermanager/appobject.c'
routermanager_sources += 'libroutermanager/audio.c'
routermanager_sources += 'libroutermanager/call.c'
routermanager_sources += 'libroutermanager/device.c'
routermanager_sources += 'libroutermanager/contact.c'
routermanager_sources += 'libroutermanager/rmconnection.c'
routermanager_sources += 'libroutermanager/csv.c'
routermanager_sources += 'libroutermanager/fax_spooler.c'
routermanager_sources += 'libroutermanager/fax_server.c'
routermanager_sources += 'libroutermanager/file.c'
routermanager_sources += 'libroutermanager/filter.c'
routermanager_sources += 'libroutermanager/ftp.c'
routermanager_sources += 'libroutermanager/gstring.c'
routermanager_sources += 'libroutermanager/logging.c'
routermanager_sources += 'libroutermanager/lookup.c'
routermanager_sources += 'libroutermanager/net_monitor.c'
routermanager_sources += 'libroutermanager/network.c'
routermanager_sources += 'libroutermanager/rmprofile.c'
routermanager_sources += 'libroutermanager/rmmain.c'
routermanager_sources += 'libroutermanager/rmosdep.c'
routermanager_sources += 'libroutermanager/password.c'
routermanager_sources += 'libroutermanager/plugins.c'
routermanager_sources += 'libroutermanager/rmprofile.c'
routermanager_sources += 'libroutermanager/rmphone.c'
routermanager_sources += 'libroutermanager/router.c'
routermanager_sources += 'libroutermanager/settings.c'
routermanager_sources += 'libroutermanager/ssdp.c'
routermanager_sources += 'libroutermanager/voxplay.c'
routermanager_sources += 'libroutermanager/xml.c'

routermanager_inc = include_directories('libroutermanager')

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


fritzbox_sources = []
fritzbox_sources += 'libroutermanager/plugins/fritzbox/csv.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-04-00.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-04-74.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-05-50.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-06-35.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-common.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/firmware-query.c'
fritzbox_sources += 'libroutermanager/plugins/fritzbox/fritzbox.c'

libfritzbox = shared_library('fritzbox',
                        fritzbox_sources,
                        include_directories : routermanager_inc,
                        dependencies : routermanager_dep,
                        link_with : libroutermanager,
                        install : true)

sourcelist = []
sourcelist += 'roger/about.c'
sourcelist += 'roger/about.h'
sourcelist += 'roger/main_ui.c'
sourcelist += 'roger/answeringmachine.c'
sourcelist += 'roger/answeringmachine.h'
sourcelist += 'roger/application.c'
sourcelist += 'roger/application.h'
sourcelist += 'roger/assistant.c'
sourcelist += 'roger/assistant.h'
sourcelist += 'roger/contacts.c'
sourcelist += 'roger/contacts.h'
sourcelist += 'roger/crash.c'
sourcelist += 'roger/crash.h'
sourcelist += 'roger/fax.c'
sourcelist += 'roger/fax.h'
sourcelist += 'roger/icons.c'
sourcelist += 'roger/icons.h'
sourcelist += 'roger/journal.c'
sourcelist += 'roger/journal.h'
sourcelist += 'roger/main.h'
sourcelist += 'roger/main_ui.c'
sourcelist += 'roger/phone.c'
sourcelist += 'roger/phone.h'
sourcelist += 'roger/print.c'
sourcelist += 'roger/print.h'
sourcelist += 'roger/settings.c'
sourcelist += 'roger/settings.h'
sourcelist += 'roger/shortcuts.c'
sourcelist += 'roger/shortcuts.h'
sourcelist += 'roger/uitools.c'
sourcelist += 'roger/uitools.h'

#    roger_gresource.c \
#    roger_gresource.h


dependencies = []
dependencies += dependency('gtk+-3.0')
dependencies += dependency('libpeas-gtk-1.0')
dependencies += dependency('libsoup-2.4')
dependencies += dependency('libtiff-4')
dependencies += dependency('spandsp')

inc = include_directories('libroutermanager')

executable(name, sources : sourcelist, dependencies : dependencies, link_with : libroutermanager, include_directories : inc, gui_app : true)

custom_target('org.tabos.roger.desktop',
    output : 'org.tabos.roger.desktop',
    input : 'org.tabos.roger.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('datadir') + '/applications')

custom_target('org.tabos.roger.appdata.xml',
    output : 'org.tabos.roger.appdata.xml',
    input : 'org.tabos.roger.appdata.xml.in',
    command : [msgfmt, '--xml', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('datadir') + '/appdata')
