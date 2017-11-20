PKG             := gssdp
$(PKG)_WEBSITE  := https://download.gnome.org/
$(PKG)_DESCR    := GSSDP
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 1.0.2
$(PKG)_CHECKSUM := a1e17c09c7e1a185b0bd84fd6ff3794045a3cd729b707c23e422ff66471535dc
$(PKG)_SUBDIR   := gssdp-$($(PKG)_VERSION)
$(PKG)_FILE     := gssdp-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://download.gnome.org/sources/gssdp/1.0//$($(PKG)_FILE)
$(PKG)_DEPS     := gcc

define $(PKG)_UPDATE
#    $(WGET) -q -O- 'https://git.xiph.org/?p=speex.git;a=tags' | \
#    grep '<a class="list name"' | \
#    $(SED) -n 's,.*<a[^>]*>Speex-\([0-9][^<]*\)<.*,\1,p' | \
#    head -1
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
	$(subst docdir$(comma),,$(MXE_CONFIGURE_OPTS))
    $(MAKE) -C '$(1)'
    $(MAKE) -C '$(1)' install
endef
