include $(top_srcdir)/m4/glib-tap.mk

AM_CFLAGS = -g $(EXTRA_CFLAGS) $(MAINTAINER_CFLAGS)
LDADD = $(top_builddir)/libroutermanager//libroutermanager.la $(EXTRA_LIBS)
AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"RouterManager\" \
	-I$(top_srcdir)	\
	-I$(top_builddir) \
	-I$(top_srcdir)/libroutermanager \
	-I$(top_builddir)/libroutermanager \
	$(NULL)

dist_test_data = \
	$(NULL)

test_programs = \
	action\
	call
	$(NULL)

#include $(top_srcdir)/m4/Makefile.am.gitignore
