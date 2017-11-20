PKG             := gupnp
$(PKG)_WEBSITE  := https://download.gnome.org/
$(PKG)_DESCR    := GUPnP
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 1.0.2
$(PKG)_CHECKSUM := 5173fda779111c6b01cd4a5e41b594322be9d04f8c74d3361f0a0c2069c77610
$(PKG)_SUBDIR   := gupnp-$($(PKG)_VERSION)
$(PKG)_FILE     := gupnp-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://download.gnome.org/sources/gupnp/1.0//$($(PKG)_FILE)
$(PKG)_DEPS     := gcc gssdp

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
