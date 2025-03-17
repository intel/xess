# XeLL DX12 basic sample

Shows basic XeLL integration with DX12.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value`. Allow to select gpu id.
- `-async`. Use asynchronous flip.
- `-fps value`. Force sample to run at a given fps.
- `-fullscreen`. Start the application in exclusive fullscreen mode.

### Shortcuts
- `l/L`. Toggle latency reduction ON/OFF.
- `space`: Pause animation.
