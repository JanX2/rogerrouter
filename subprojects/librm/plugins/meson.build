subdir('areacodes_global')
subdir('capi')
subdir('fritzbox')

if get_option('enable-gstreamer')
	gstreamer = []
	gstreamer += dependency('gstreamer-1.0', version : '>= 1.4.0')
	gstreamer += dependency('gstreamer-app-1.0')
	subdir('gstreamer')
endif

if get_option('enable-portaudio')
	portaudio = []
	portaudio += dependency('portaudio-2.0', version : '>= 19')
	subdir('portaudio')
endif

if get_option('enable-pulseaudio')
	pulseaudio = []
	pulseaudio += dependency('libpulse-simple')
	subdir('pulseaudio')
endif

# subdir('reverselookup')

if get_option('enable-libsecret')
	subdir('secret')
endif

subdir('simple_pwd')

# Disabled - needs more love
#if host_machine.system() == 'windows'
#	subdir('wincred')
#endif
