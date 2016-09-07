rmconf_data = configuration_data()

rm_dep = []
rm_dep += dependency('glib-2.0')
rm_dep += dependency('libpeas-1.0')
rm_dep += dependency('libsoup-2.4')
rm_dep += dependency('speex')
rm_dep += dependency('libpeas-1.0')
rm_dep += dependency('libxml-2.0')
rm_dep += dependency('spandsp')
rm_dep += dependency('json-glib-1.0')
rm_dep += dependency('dconf')

# Check for UPnP support
upnp_dep = dependency('gupnp-1.0', required : false)
rm_dep += upnp_dep

# Check for SSDP support
ssdp_dep = dependency('gssdp-1.0', required : false)
rm_dep += ssdp_dep

rmconf_data.set10('HAVE_SSDP', ssdp_dep.found())
configure_file(input : 'rmconfig.h.meson', output : 'rmconfig.h', configuration : rmconf_data)

rm_sources = []
rm_sources += 'rmaction.c'
rm_sources += 'rmaddressbook.c'
rm_sources += 'rmaudio.c'
rm_sources += 'rmcall.c'
rm_sources += 'rmcontact.c'
rm_sources += 'rmconnection.c'
rm_sources += 'rmcsv.c'
rm_sources += 'rmdevice.c'
rm_sources += 'rmdevicefax.c'
rm_sources += 'rmfaxspooler.c'
rm_sources += 'rmfaxserver.c'
rm_sources += 'rmfile.c'
rm_sources += 'rmfilter.c'
rm_sources += 'rmftp.c'
rm_sources += 'rmjournal.c'
rm_sources += 'rmstring.c'
rm_sources += 'rmlog.c'
rm_sources += 'rmlookup.c'
rm_sources += 'rmnetmonitor.c'
rm_sources += 'rmnetwork.c'
rm_sources += 'rmobject.c'
rm_sources += 'rmobjectemit.c'
rm_sources += 'rmprofile.c'
rm_sources += 'rmmain.c'
rm_sources += 'rmnotification.c'
rm_sources += 'rmnumber.c'
rm_sources += 'rmosdep.c'
rm_sources += 'rmpassword.c'
rm_sources += 'rmplugins.c'
rm_sources += 'rmprofile.c'
rm_sources += 'rmphone.c'
rm_sources += 'rmrouter.c'
rm_sources += 'rmsettings.c'
if ssdp_dep.found()
	rm_sources += 'rmssdp.c'
endif
rm_sources += 'rmvox.c'
rm_sources += 'xml.c'

rm_inc = include_directories('..')

libroutermanager = shared_library('routermanager',
                        rm_sources,
                        include_directories : rm_inc,
                        dependencies : rm_dep,
                        install : true,
                        link_args : '-lcrypt')

headers = [
	'rmaction.h',
	'rmaddressbook.h',
	'rmaudio.h',
	'rmcall.h',
	'rmconnection.h',
	'rmcontact.h',
	'rmcsv.h',
	'rmdevicefax.h',
	'rmdevice.h',
	'rmfaxprinter.h',
	'rmfile.h',
	'rmfilter.h',
	'rmftp.h',
	'rmjournal.h',
	'rmlog.h',
	'rmlookup.h',
	'rmmain.h',
	'rmnetmonitor.h',
	'rmnetwork.h',
	'rmobjectemit.h',
	'rmobject.h',
	'rmosdep.h',
	'rmpassword.h',
	'rmphone.h',
	'rmplugins.h',
	'rmprofile.h',
	'rmsettings.h',
	'rmssdp.h',
	'rmstring.h',
	'rmvox.h',
	'rmrouter.h',
	'xml.h'
]
install_headers(headers, subdir : 'libroutermanager')

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : libroutermanager,
                 version : '1.2',
                 name : 'libroutermanager',
                 filebase : 'libroutermanager',
                 description : 'A library to handle AVM FRITZ!Box routers.')

subdir('plugins')
subdir('data')
