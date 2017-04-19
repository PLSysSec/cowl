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

- Follow chromium build instructions: https://www.chromium.org/developers/how-tos/get-the-code
