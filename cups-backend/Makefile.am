cupsbackend_PROGRAMS = roger-fax
roger_fax_SOURCES = \
	roger-backend.c \
	backend-runloop.c \
	backend-spooldir.c \
	roger-backend.h \
	TODO 

roger_fax_CFLAGS = -Wall $(EXTRA_CFLAGS) 
roger_fax_LDFLAGS = $(EXTRA_LIBS)

install-data-hook:
	chmod 0500 $(DESTDIR)$(cupsbackenddir)/roger-fax 
	chown root.root $(DESTDIR)$(cupsbackenddir)/roger-fax
