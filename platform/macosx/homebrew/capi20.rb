class Capi20 < Formula
  desc "Handle requests from CAPI-driven applications such as fax systems via active and passive ISDN cards or FRITZ!Box routers."
  homepage "https://www.tabos.org"
  url "https://www.tabos.org/wp-content/uploads/2017/03/capi20-v3.tar.xz"
  sha256 "d6612fc5472cd40f56de4463975eae602dcab38dc285c0c7d4059a5a35a39724"
  version="3.0.6"

  def install
    system "./configure", "--disable-dependency-tracking",
                          "--prefix=#{prefix}"

    system "make", "install"
  end

  test do
    # `test do` will create, run in and delete a temporary directory.
    #
    # This test will fail and we won't accept that! For Homebrew/homebrew-core
    # this will need to be a test that verifies the functionality of the
    # software. Run the test with `brew test capi20-v`. Options passed
    # to `brew install` such as `--HEAD` also need to be provided to `brew test`.
    #
    # The installed folder is not in the path, so use the entire path to any
    # executables being tested: `system "#{bin}/program", "do", "something"`.
    system "false"
  end
end
