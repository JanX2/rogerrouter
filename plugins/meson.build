plugins_dep=[]
plugins_dep+=dependency('gtk+-3.0')
plugins_dep+=dependency('libsoup-2.4')
plugins_dep+=dependency('librm')

subdir('evolution')
subdir('fritzfon')
subdir('gnotification')
subdir('google')

#if host_machine.system() == 'windows'
subdir('gtknotify')
#endif

subdir('indicator')
subdir('keychain')

# KWallet is disabled and needs some love for KDE5
#subdir('kwallet')

subdir('osxab')
subdir('sip')

# Statusicon needs more love
#subdir('statusicon')

subdir('thunderbird')
subdir('vcard')
subdir('webjournal')
