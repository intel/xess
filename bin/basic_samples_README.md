# XeSS-SR DX12 basic sample

Shows basic XeSS-SR integration with DX12.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value` Allow to select gpu id.

### Shortcuts
- `1`: Show input color.
- `2`: Show intput velocity.
- `3`: Show output.
- `space`: Pause animation.

---
# XeSS-SR Vulkan basic sample

Shows basic XeSS-SR integration with Vulkan.

### Windows build steps

The sample requires a "glm" library source code to be unpacked into `basic_sample_vk/glm` directory.
Following commands can be used to download and unpack the glm library:
```
powershell -command "Invoke-WebRequest https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.zip -OutFile glm-1.0.1-light.zip"
powershell -command "Expand-Archive -Path glm-1.0.1-light.zip  -DestinationPath ."
```

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value` Allow to select gpu id.

---
# XeSS-SR DX11 basic sample

Shows basic XeSS-SR integration with DX11.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value` Allow to select gpu id.

---

# XeSS-FG DX12 basic sample

Shows basic XeSS-FG integration with DX12.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value` Allow to select gpu id.
- `-async` Use asynchronous flip.
- `-fps value` Force sample to run at a given fps.
- `-maximized` Force window to be maximized.
- `-width value` Override default window width.
- `-height value` Override default window height.
- `-top` Force window to be topmost.
- `-tag_interpolated_frames value` Tag interpolated frames by showing purple stripes. Default is true.
- `-fullscreen` Start the application in exclusive fullscreen mode.

### Shortcuts
- `3`: Toggle frame interpolation ON/OFF.
- `space`: Pause animation.
- `F3`: Switch between 1080p and 1440p.
- `F4`: Switch between exclusive fullscreen and windowed mode.
- `F5`: Switch show only interpolated frames.
- `F6`: Switch tag interpolated frames.

---

# XeLL DX12 basic sample

Shows basic XeLL integration with DX12.

### Windows build steps

Run a 'Developer Command Prompt for Visual Studio 2019", then type the following:
```
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command line options
- `-gpu_id value` Allow to select gpu id.
- `-async` Use asynchronous flip.
- `-fps value` Force sample to run at a given fps.
- `-fullscreen` Start the application in exclusive fullscreen mode.

### Shortcuts
- `l/L`: Toggle latency reduction ON/OFF.
