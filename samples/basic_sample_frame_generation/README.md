# XeSS FG DX12 basic sample

Shows basic XeSS FG integration with DX12.

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
- `-maximized`. Force window to be maximized.
- `-width value`. Override default window width.
- `-height value`. Override default window height.
- `-top`. Force window to be topmost.
- `-tag_interpolated_frames value`. Tag interpolated frames by showing purple stripes. Default is true.
- `-fullscreen`. Start the application in exclusive fullscreen mode.

### Shortcuts
- `3`: Toggle frame interpolation ON/OFF.
- `space`: Pause animation.
- `F3`. Switch between 1080p and 1440p.
- `F4`. Switch between exclusive fullscreen and windowed mode.
- `F5`. Switch show only interpolated frames.
- `F6`. Switch tag interpolated frames.
