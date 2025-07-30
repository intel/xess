# XeSS-SR DX11 basic sample

Shows basic XeSS-SR integration with DX11.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value`. Allow to select gpu id.
