class Hckr < Formula
  desc "Browse Hacker News from the terminal"
  homepage "https://github.com/kzzzld/hckr"
  head "https://github.com/kzzzld/hckr.git", branch: "main"

  depends_on "pkgconf" => :build
  depends_on "curl"
  depends_on "libxml2"
  depends_on "ncurses"

  def install
    system "make", "CC=#{ENV.cc}"
    bin.install "build/hckr"
    pkgshare.install "config/config.ini"
  end

  test do
    ENV["XDG_CONFIG_HOME"] = (testpath/"config").to_s
    ENV.delete("HCKR_HN_URL")
    assert_match "an interactive terminal is required", shell_output("#{bin}/hckr 2>&1", 1)
    config = testpath/"config/hckr/config.ini"
    assert_path_exists config
    assert_match "[hackernews]", config.read
    assert_match "[ui]", config.read
  end
end
