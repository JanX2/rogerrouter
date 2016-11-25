project('roger', 'c', version : '1.10-pre1', license : 'GPL2', default_options : ['prefix=/usr'])

gnome = import('gnome')

msgfmt = find_program('msgfmt')

add_global_arguments('-DUSE_PRINTER_SPOOLER', language : 'c')

name = 'roger'
version = '1.10-pre1'

cc = meson.get_compiler('c')

cdata = configuration_data()
cdata.set('PACKAGE_NAME', '"Roger Router"')
cdata.set('PACKAGE_VERSION', '"@0@"'.format(version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')
cdata.set('GETTEXT_PACKAGE', '"roger"')
cdata.set('APP_DATA', '"' + get_option('prefix') + '/' + get_option('datadir') + '/roger/"')
cdata.set('APP_LOCALE', '"' + get_option('prefix') + '/' + get_option('localedir') + '"')
cdata.set('APP_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/roger/"')
cdata.set('RM_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/routermanager/"')
cdata.set('RM_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/routermanager/"')

check_headers = [['dlfcn.h','HAVE_DLFCN_H'],
  ['sys/utsname.h', 'HAVE_SYS_UTSNAME_H'],
]

foreach h : check_headers
  if cc.has_header(h.get(0))
    cdata.set(h.get(1), 1)
  endif
endforeach

configure_file(input : 'config.h.meson',
  output : 'config.h',
  configuration : cdata)

subdir('po')
subdir('libroutermanager')
subdir('roger')

#if get_option('enable-gtk-doc')
#	subdir('docs')
#endif

if get_option('do-post-install')
	meson.add_install_script('meson_post_install.sh')
endif
