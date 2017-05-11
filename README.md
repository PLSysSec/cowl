# Build instructions

- Copy COWL source code files into the corresponding directory in the Chromium source code

- Modify *WebKit/Source/core/core_idl_files.gni* to include COWL IDL files
```
     core_idl_files = get_path_info([
    ...
    "cowl/Label.idl",
    "cowl/Privilege.idl",
    ...
```

- Modify *WebKit/Source/core/BUILD.gn* to include the path to the COWL directory 
```
    component("core") {
    ...
    "//third_party/WebKit/Source/core/cowl",
    ...
```

- Follow [Chromium build instructions](https://www.chromium.org/developers/how-tos/get-the-code)

# Testing instructions

- Initial setup: `ninja -C out/Default blink_tests`

- Ubuntu 14.04: run from WebKit directory: `python Tools/Scripts/run-webkit-tests -t Default cowl`

- Ubuntu 16.04: run from src directory: `out/Default/content_shell --dump-render-tree ~/chromium/src/third_party/WebKit/LayoutTests/cowl/label.html`

- Issues with Ubuntu 16.04:
  + [Running layout tests on Linux](https://chromium.googlesource.com/chromium/src/+/master/docs/layout_tests_linux.md)
  + [Hacky solution](https://github.com/pwnall/pwnall-blog/blob/master/source/_posts/2013-11-15-running-the-blink-layout-tests-on-fedora-linux.markdown#fonts-for-the-layout-tests)

- [Chromium layout test instructions](https://chromium.googlesource.com/chromium/src/+/master/docs/testing/layout_tests.md)
