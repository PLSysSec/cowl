# Build instructions

- [Chromium build instructions](https://www.chromium.org/developers/how-tos/get-the-code)

# Testing instructions

- [Chromium layout test instructions](https://chromium.googlesource.com/chromium/src/+/master/docs/testing/layout_tests.md)

- Ubuntu 14.04: `/src$ python third_party/WebKit/Tools/Scripts/run-webkit-tests -t Default cowl`

- Ubuntu 16.04: `/src$ out/Default/content_shell --dump-render-tree [third_party/WebKit/LayoutTests/cowl/label.html]`

- Issues with Ubuntu 16.04:
  + [Running layout tests on Linux](https://chromium.googlesource.com/chromium/src/+/master/docs/layout_tests_linux.md)
  + [Fonts for the Layout Tests](https://github.com/pwnall/pwnall-blog/blob/master/source/_posts/2013-11-15-running-the-blink-layout-tests-on-fedora-linux.markdown#fonts-for-the-layout-tests)

