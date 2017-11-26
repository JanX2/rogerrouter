PKG             := gssdp
$(PKG)_WEBSITE  := https://download.gnome.org/
$(PKG)_DESCR    := A GObject-based API for handling resource discovery and announcement over SSDP
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 1.0.2
$(PKG)_CHECKSUM := a1e17c09c7e1a185b0bd84fd6ff3794045a3cd729b707c23e422ff66471535dc
$(PKG)_SUBDIR   := gssdp-$($(PKG)_VERSION)
$(PKG)_FILE     := gssdp-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://download.gnome.org/sources/gssdp/$(call SHORT_PKG_VERSION,$(PKG))/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc

define $(PKG)_UPDATE
    $(WGET) -q -O- 'https://git.gnome.org/browse/gssdp/refs/tags' | \
    $(SED) -n "s,.*gssdp-\([0-9]\+\.[0-9]*[02468]\.[^']*\)\.tar.*,\1,p" | \
    $(SORT) -Vr | \
    head -1
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
	$(subst docdir$(comma),,$(MXE_CONFIGURE_OPTS))
    $(MAKE) -C '$(1)'
    $(MAKE) -C '$(1)' install
endef
