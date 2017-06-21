project('rm', 'c', version : '2.0.0', license : 'GPL2', default_options : ['prefix=/usr'])

rm_name = meson.project_name()
rm_version = meson.project_version()

version_arr = rm_version.split('.')
rm_version_major = version_arr[0]
rm_version_minor = version_arr[1]
rm_version_micro = version_arr[2]

api_version = '2.0'
soversion = 0

libversion = '@0@.@1@.0'.format(soversion, rm_version_minor.to_int() * 100 + rm_version_micro.to_int())

gnome = import('gnome')

cc = meson.get_compiler('c')
msgfmt = find_program('msgfmt')

podir = '@0@'.format(meson.current_source_dir())

rm_dep = []
rm_dep += cc.find_library('m', required: true)
rm_dep += dependency('glib-2.0')
rm_dep += dependency('libsoup-2.4')
rm_dep += dependency('speex')
rm_dep += dependency('libxml-2.0')
rm_dep += dependency('libtiff-4')
rm_dep += dependency('spandsp')
rm_dep += dependency('json-glib-1.0')
rm_dep += dependency('gmodule-2.0')
rm_dep += dependency('sndfile')

if host_machine.system() == 'darwin'
rm_dep += dependency('gtk+-3.0')
rm_dep += dependency('gtk-mac-integration')
endif

# Check for UPnP support
upnp_dep = dependency('gupnp-1.0')
rm_dep += upnp_dep

# Check for SSDP support
ssdp_dep = dependency('gssdp-1.0')
rm_dep += ssdp_dep

# Check for dconf (allowing us to delete gsettings entries on Linux)
dconf_dep = dependency('dconf', required : false)
rm_dep += dconf_dep

# Check for uuid
uuid_dep = dependency('uuid', required : false)
rm_dep += uuid_dep

rm_inc = include_directories('.')

if host_machine.system() == 'windows'
rm_plugins_path = 'lib/' + rm_name
else
rm_plugins_path = get_option('prefix') + '/' + get_option('libdir') + '/' + rm_name
endif

cdata = configuration_data()
cdata.set('RM_NAME', '"rm"')
cdata.set('RM_VERSION', '"@0@"'.format(rm_version))
cdata.set('RM_PLUGINS', '"' + rm_plugins_path + '"')
cdata.set('RM_GETTEXT_PACKAGE', '"rm"')

if ssdp_dep.found()
cdata.set10('HAVE_SSDP', 1)
endif

cdata.set10('HAVE_DCONF', dconf_dep.found())
cdata.set10('HAVE_UUID', dconf_dep.found())

if host_machine.system() == 'windows'
	cdata.set('APP_LOCALE', '"' + get_option('localedir') + '"')
else
	cdata.set('APP_LOCALE', '"' + get_option('prefix') + '/' + get_option('localedir') + '"')
endif

configure_file(
  output : 'rmconfig.h',
  configuration : cdata)

gnome.compile_schemas()
install_data('org.tabos.rm.gschema.xml',
install_dir : 'share/glib-2.0/schemas')

subdir('po')
subdir('rm')
subdir('plugins')
#subdir('docs')

meson.add_install_script('meson_post_install.sh')
