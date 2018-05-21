class Librm < Formula
  desc "RouterManager Library"
  homepage "https://www.tabos.org"
  url "https://www.tabos.org/wp-content/uploads/2018/05/librm-master.tar.gz"
  version "2.0.0"
  sha256 "09f78f5dd6a8959186a90cb9f00bdb445146d434a7595feea3203b294c31af08"

  depends_on "meson-internal" => :build
  depends_on "ninja" => :build
  depends_on "pkg-config" => :build
  depends_on "capi20"
  depends_on "gdk-pixbuf"
  depends_on "gettext"
  depends_on "glib"
  depends_on "gst-plugins-base"
  depends_on "gst-plugins-good"
  depends_on "gstreamer"
  depends_on "gtk+3"
  depends_on "gtk-mac-integration"
  depends_on "gupnp"
  depends_on "json-glib"
  depends_on "libsndfile"
  depends_on "libsoup"
  depends_on "spandsp"
  depends_on "speex"

  def install
    args = %W[
      --prefix=#{prefix}
      -Denable-post-install=false
    ]

    mkdir "build" do
      system "meson", *args, ".."
      system "ninja"
      system "ninja", "install"
    end

    # meson-internal gives wrong install_names for dylibs due to their unusual installation location
    # create softlinks to fix
    ln_s Dir.glob("#{lib}/rm/*dylib"), lib
  end

  def post_install
    system "#{Formula["glib"].opt_bin}/glib-compile-schemas", "#{HOMEBREW_PREFIX}/share/glib-2.0/schemas"
    system "#{Formula["gtk+3"].opt_bin}/gtk3-update-icon-cache", "-f", "-t", "#{HOMEBREW_PREFIX}/share/icons/hicolor"
  end

  test do
    # `test do` will create, run in and delete a temporary directory.
    #
    # This test will fail and we won't accept that! For Homebrew/homebrew-core
    # this will need to be a test that verifies the functionality of the
    # software. Run the test with `brew test librm`. Options passed
    # to `brew install` such as `--HEAD` also need to be provided to `brew test`.
    #
    # The installed folder is not in the path, so use the entire path to any
    # executables being tested: `system "#{bin}/program", "do", "something"`.
    system "false"
  end
end
