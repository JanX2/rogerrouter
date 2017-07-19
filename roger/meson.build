roger_gresources = gnome.compile_resources(
  'roger-gresources', 'roger.gresource.xml',
  source_dir : '.',
  c_name : '')

cdata = configuration_data()
cdata.set('PACKAGE_NAME', 'Roger Router')
cdata.set('PACKAGE_VERSION', '@0@'.format(app_version))
cdata.set('PACKAGE_BUGREPORT', '"https://www.tabos.org/forum"')
cdata.set('ROGER_VERSION_MAJOR', app_version_major)
cdata.set('ROGER_VERSION_MINOR', app_version_minor)
cdata.set('ROGER_VERSION_MICRO', app_version_micro)
cdata.set('VERSION', app_version)

resfile = configure_file(input : 'roger_winres.rc.in',
  output : 'roger_winres.rc',
  configuration : cdata)

sourcelist = []
sourcelist += 'about.c'
sourcelist += 'about.h'
sourcelist += 'main_ui.c'
sourcelist += 'answeringmachine.c'
sourcelist += 'answeringmachine.h'
sourcelist += 'application.c'
sourcelist += 'application.h'
sourcelist += 'assistant.c'
sourcelist += 'assistant.h'
sourcelist += 'contacts.c'
sourcelist += 'contacts.h'
sourcelist += 'contactsearch.c'
sourcelist += 'contactsearch.h'
sourcelist += 'crash.c'
sourcelist += 'crash.h'
sourcelist += 'debug.c'
sourcelist += 'debug.h'
sourcelist += 'fax.c'
sourcelist += 'fax.h'
sourcelist += 'icons.c'
sourcelist += 'icons.h'
sourcelist += 'journal.c'
sourcelist += 'journal.h'
sourcelist += 'main.h'
sourcelist += 'main_ui.c'
sourcelist += 'phone.c'
sourcelist += 'phone.h'
sourcelist += 'plugins.c'
sourcelist += 'plugins.h'
sourcelist += 'print.c'
sourcelist += 'print.h'
sourcelist += 'settings.c'
sourcelist += 'settings.h'
sourcelist += 'shortcuts.c'
sourcelist += 'shortcuts.h'
sourcelist += 'uitools.h'
sourcelist += roger_gresources

if host_machine.system() == 'windows'
win = import('windows')
sourcelist += win.compile_resources(resfile)
endif

roger_dep = []
roger_dep += dependency('gtk+-3.0', version : '>=3.12.0')
roger_dep += dependency('libsoup-2.4')
roger_dep += dependency('libtiff-4')
roger_dep += dependency('spandsp')
roger_dep += dependency('librm')
roger_dep += cc.find_library('m', required: true)
roger_dep += cc.find_library('gs', required: true)

executable('roger', sources : sourcelist, dependencies : roger_dep, include_directories : roger_inc, gui_app : true, install : true)

i18n.merge_file(
    input: 'org.tabos.roger.desktop.in',
    output: 'org.tabos.roger.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications')
)

i18n.merge_file(
    input: 'org.tabos.roger.appdata.xml.in',
    output: 'org.tabos.roger.appdata.xml',
    type: 'xml',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'appdata')
)

images = []
images += 'images/app.png'
install_data(images, install_dir : get_option('datadir') + '/roger/')

images = []
images += 'images/roger.svg'
install_data(images, install_dir : get_option('datadir') + '/icons/hicolor/scalable/apps/')

subdir('plugins')
subdir('data')
