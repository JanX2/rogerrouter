PKG             := capi
$(PKG)_WEBSITE  := https://www.tabos.org
$(PKG)_DESCR    := Common ISDN Application Programming Interface
$(PKG)_IGNORE   :=
$(PKG)_VERSION  := v3
$(PKG)_CHECKSUM := d6612fc5472cd40f56de4463975eae602dcab38dc285c0c7d4059a5a35a39724
$(PKG)_SUBDIR   := capi20-v3
$(PKG)_FILE     := capi20-$($(PKG)_VERSION).tar.xz
$(PKG)_URL      := https://www.tabos.org/wp-content/uploads/2017/03/$($(PKG)_FILE)
$(PKG)_DEPS     := gcc dlfcn-win32

define $(PKG)_UPDATE
    echo 'TODO: write update script for $(PKG).' >&2;
    echo $($(PKG)_VERSION)
endef

define $(PKG)_BUILD
    cd '$(1)' && ./configure \
	$(subst docdir$(comma),,$(MXE_CONFIGURE_OPTS))
    $(MAKE) -C '$(1)'
    $(MAKE) -C '$(1)' install
endef
