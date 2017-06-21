rm_sources = [
	'rmaction.c',
	'rmaddressbook.c',
	'rmaudio.c',
	'rmcallentry.c',
	'rmcontact.c',
	'rmconnection.c',
	'rmcsv.c',
	'rmdevice.c',
	'rmfax.c',
	'rmfaxspooler.c',
	'rmfaxserver.c',
	'rmfile.c',
	'rmfilter.c',
	'rmftp.c',
	'rmjournal.c',
	'rmstring.c',
	'rmlog.c',
	'rmlookup.c',
	'rmnetmonitor.c',
	'rmnetwork.c',
	'rmobject.c',
	'rmobjectemit.c',
	'rmprofile.c',
	'rmmain.c',
	'rmnotification.c',
	'rmnumber.c',
	'rmosdep.c',
	'rmpassword.c',
	'rmplugins.c',
	'rmprofile.c',
	'rmphone.c',
	'rmrouter.c',
	'rmsettings.c',
	'rmssdp.c',
	'rmvox.c',
	'xml.c'
]

rm_headers = [
	'rmaction.h',
	'rmaddressbook.h',
	'rmaudio.h',
	'rmcallentry.h',
	'rmconnection.h',
	'rmcontact.h',
	'rmcsv.h',
	'rmfax.h',
	'rmdevice.h',
	'rmfaxserver.h',
	'rmfaxspooler.h',
	'rmfile.h',
	'rmfilter.h',
	'rmftp.h',
	'rmjournal.h',
	'rmlog.h',
	'rmlookup.h',
	'rmmain.h',
	'rmnetmonitor.h',
	'rmnetwork.h',
	'rmnotification.h',
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
	'rmrouterinfo.h',
	'xml.h'
]
install_headers(rm_headers, subdir : 'rm')

rm_inc = include_directories('.', '..')

rm_link = []

if host_machine.system() == 'linux'
rm_link += '-lcrypt'
endif

rm_shared = shared_library('rm',
                        rm_sources,
                        include_directories : rm_inc,
                        dependencies : rm_dep,
                        install : true,
                        link_args : rm_link)

rm_dep = declare_dependency(link_with : rm_shared,
    include_directories : rm_inc,
    dependencies : rm_dep)

pkg_mod = import('pkgconfig')
pkg_mod.generate(libraries : rm_shared,
                 version : '1.2',
                 name : 'librm',
                 filebase : 'librm',
                 description : 'A library to handle AVM FRITZ!Box routers.',
                 requires: 'libsoup-2.4 gio-2.0 glib-2.0 gmodule-2.0 libxml-2.0')
