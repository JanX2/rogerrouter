if WITH_CUPS
cupsbackend_PROGRAMS = roger-cups
roger_cups_SOURCES = \
	roger-backend.c \
	backend-runloop.c \
	backend-spooldir.c \
	roger-backend.h \
	TODO 

roger_cups_CFLAGS = -Wall $(EXTRA_CFLAGS) $(CUPS_CFLAGS)
roger_cups_LDFLAGS = $(EXTRA_LIBS) $(CUPS_LIBS)

#install-data-hook:
#	chmod 0500 $(DESTDIR)$(cupsbackenddir)/roger-cups 
#	chown root.root $(DESTDIR)$(cupsbackenddir)/roger-cups
endif
