project('roger', 'c', version : '1.10-pre1', license : 'GPL2', default_options : ['b_lundef=false', 'prefix=/usr'])

gnome = import('gnome')

msgfmt = find_program('msgfmt')

add_global_arguments('-DUSE_PRINTER_SPOOLER', language : 'c')

name = 'roger'
version = '1.10-pre1'

cdata = configuration_data()
cdata.set('PACKAGE_NAME', '"Roger Router"')
cdata.set('PACKAGE_VERSION', '"@0@"'.format(version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')
cdata.set('GETTEXT_PACKAGE', '"roger"')
cdata.set('APP_DATA', '"' + get_option('prefix') + '/' + get_option('datadir') + '/roger/"')
cdata.set('APP_LOCALE', '"' + get_option('prefix') + '/' + get_option('localedir') + '"')
cdata.set('APP_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/roger/"')
cdata.set('RM_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/routermanager/"')

configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)

subdir('po')
subdir('docs')
subdir('libroutermanager')
subdir('roger')
