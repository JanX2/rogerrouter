roger_gresources = gnome.compile_resources(
  'roger-gresources', 'roger.gresource.xml',
  source_dir : '.',
  c_name : '')

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
sourcelist += 'print.c'
sourcelist += 'print.h'
sourcelist += 'settings.c'
sourcelist += 'settings.h'
sourcelist += 'shortcuts.c'
sourcelist += 'shortcuts.h'
sourcelist += 'uitools.c'
sourcelist += 'uitools.h'
sourcelist += roger_gresources

roger_dep = []
roger_dep += dependency('gtk+-3.0', version : '>=3.12.0')
roger_dep += dependency('libpeas-gtk-1.0')
roger_dep += dependency('libsoup-2.4')
roger_dep += dependency('libtiff-4')
roger_dep += dependency('spandsp')
roger_dep += cc.find_library('m', required: true)

inc = include_directories('..')

executable(name, sources : sourcelist, dependencies : roger_dep, link_with : libroutermanager, include_directories : inc, gui_app : true, install : true)

custom_target('org.tabos.roger.desktop',
    output : 'org.tabos.roger.desktop',
    input : 'org.tabos.roger.desktop.in',
    command : [msgfmt, '--desktop', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('datadir') + '/applications',)

custom_target('org.tabos.roger.appdata.xml',
    output : 'org.tabos.roger.appdata.xml',
    input : 'org.tabos.roger.appdata.xml.in',
    command : [msgfmt, '--xml', '--template', '@INPUT@', '-d', podir, '-o', '@OUTPUT@'],
    install : true,
    install_dir : get_option('datadir') + '/appdata',)

images = []
images += 'images/app.png'
images += 'images/roger-call-blocked.svg'
images += 'images/roger-call-blocked-symbolic.svg'
images += 'images/roger-call-in.svg'
images += 'images/roger-call-in-symbolic.svg'
images += 'images/roger-call-missed.svg'
images += 'images/roger-call-missed-symbolic.svg'
images += 'images/roger-call-out.svg'
images += 'images/roger-call-out-symbolic.svg'
images += 'images/roger-call-voice.svg'
images += 'images/roger-call-voice-symbolic.svg'
images += 'images/roger-fax.svg'
images += 'images/roger-fax-symbolic.svg'
images += 'images/roger-fax-report.svg'
images += 'images/roger-fax-report-symbolic.svg'
images += 'images/roger-record.svg'
images += 'images/roger-record-symbolic.svg'

install_data(images, install_dir : get_option('datadir') + '/roger/')

subdir('plugins')
subdir('data')
