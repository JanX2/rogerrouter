PKG             := adwaita-icon-theme
$(PKG)_WEBSITE  := https://www.gnome.org
$(PKG)_DESCR    := Adwaita Icon Theme
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := 3.26.0
$(PKG)_CHECKSUM := 9cad85de19313f5885497aceab0acbb3f08c60fcd5fa5610aeafff37a1d12212
$(PKG)_SUBDIR   := adwaita-icon-theme-$($(PKG)_VERSION)
$(PKG)_FILE     := adwaita-icon-theme-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://download.gnome.org/sources/$(PKG)/$(call SHORT_PKG_VERSION,$(PKG))/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc librsvg

define $(PKG)_UPDATE
   $(WGET) -q -O- 'https://git.gnome.org/browse/adwaita-icon-theme/refs/tags' | \
    grep '<a href=' | \
    $(SED) -n 's,.*<a[^>]*>\([0-9]*\.[0-9]*[02468]\.[^<]*\)<.*,\1,p' | \
    sort -Vr | \
    head -1
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
	$(subst docdir$(comma),,$(MXE_CONFIGURE_OPTS))
    $(MAKE) -C '$(1)'
    $(MAKE) -C '$(1)' install
endef
