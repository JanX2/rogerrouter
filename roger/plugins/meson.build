if host_machine.system() == 'linux'
subdir('evolution')
endif

subdir('fritzfon')
subdir('gnotification')
#subdir('google')

if host_machine.system() == 'windows'
subdir('gtknotify')
endif

subdir('sip')

if get_option('enable-indicator')
	indicator = []
	indicator += dependency('appindicator3-0.1')
	subdir('indicator')
endif

if get_option('enable-kwallet')
	kwallet = []
	subdir('kwallet')
endif

subdir('webjournal')
