PKG             := spandsp
$(PKG)_WEBSITE  := https://www.soft-switch.org
$(PKG)_DESCR    := SpanDSP
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 0.0.6pre21
$(PKG)_CHECKSUM := bd152152bf0b204661ab9439c5a649098bcb8cefebcbfa959dd602442739aa50
$(PKG)_SUBDIR   := spandsp-0.0.6
$(PKG)_FILE     := spandsp-$($(PKG)_VERSION).tgz
$(PKG)_URL      := https://www.soft-switch.org/downloads/spandsp/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc tiff

define $(PKG)_UPDATE
#    $(WGET) -q -O- 'https://git.xiph.org/?p=speex.git;a=tags' | \
#    grep '<a class="list name"' | \
#    $(SED) -n 's,.*<a[^>]*>Speex-\([0-9][^<]*\)<.*,\1,p' | \
#    head -1
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
	$(subst docdir$(comma),,$(MXE_CONFIGURE_OPTS)) \
        LIBS="`'$(TARGET)-pkg-config' --libs libtiff-4`"
    $(MAKE) -C '$(1)'
    $(MAKE) -C '$(1)' install
endef
