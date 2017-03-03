project('roger', 'c', version : '1.90.0', license : 'GPL2')

gnome = import('gnome')

msgfmt = find_program('msgfmt')

add_global_arguments('-fdiagnostics-color=always', language : 'c')

app_version = meson.project_version()

version_arr = app_version.split('.')
app_version_major = version_arr[0]
app_version_minor = version_arr[1]
app_version_micro = version_arr[2]

cc = meson.get_compiler('c')

cdata = configuration_data()
cdata.set('PACKAGE_NAME', '"Roger Router"')
cdata.set('PACKAGE_VERSION', '"@0@"'.format(app_version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')
cdata.set('GETTEXT_PACKAGE', '"roger"')

if host_machine.system() == 'windows'
cdata.set('APP_PLUGINS', '"' + get_option('libdir') + '/roger/"')
cdata.set('APP_DATA', '"' + get_option('datadir') + '/roger/"')
cdata.set('APP_LOCALE', '"' + get_option('localedir') + '"')
else
cdata.set('APP_PLUGINS', '"' + get_option('prefix') + '/' + get_option('libdir') + '/roger/"')
cdata.set('APP_DATA', '"' + get_option('prefix') + '/' + get_option('datadir') + '/roger/"')
cdata.set('APP_LOCALE', '"' + get_option('prefix') + '/' + get_option('localedir') + '"')
endif

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

roger_inc = include_directories('.')

subdir('po')

rm_sp = subproject('librm')
rm_dep = rm_sp.get_variable('rm_dep')
rm_inc = rm_sp.get_variable('rm_inc')

subdir('roger')
subdir('roger-cli')
subdir('platform')

meson.add_install_script('meson_post_install.sh')
