subdir('evolution')

subdir('fritzfon')
subdir('gnotification')
#subdir('google')
subdir('gtknotify')
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
