plugins_dep=[]
plugins_dep += dependency('gtk+-3.0')
plugins_dep += dependency('libsoup-2.4')
plugins_dep += dependency('librm')

subdir('google')

if host_machine.system().contains('linux')
subdir('indicator')
subdir('evolution')
else
subdir('gtknotify')
subdir('statusicon')
endif

subdir('keychain')
subdir('osxab')


subdir('thunderbird')
subdir('vcard')
subdir('webjournal')
