# Intel® XeSS Sample Applications

This directory contains basic sample applications demonstrating integration of Intel® XeSS technologies.

Contents:

- [System Requirements](#system-requirements)
- [XeSS-SR DX12 Basic Sample](#xess-sr-dx12-basic-sample)
  - [Windows Build Steps](#windows-build-steps)
  - [Command Line Options](#command-line-options)
  - [Keyboard Shortcuts](#keyboard-shortcuts)
- [XeSS-SR Vulkan Basic Sample](#xess-sr-vulkan-basic-sample)
  - [Windows Build Steps](#windows-build-steps-1)
  - [Command Line Options](#command-line-options-1)
- [XeSS-SR DX11 Basic Sample](#xess-sr-dx11-basic-sample)
  - [Windows Build Steps](#windows-build-steps-2)
  - [Command Line Options](#command-line-options-2)
- [XeSS-FG DX12 Basic Sample](#xess-fg-dx12-basic-sample)
  - [Windows Build Steps](#windows-build-steps-3)
  - [Command Line Options](#command-line-options-3)
  - [Keyboard Shortcuts](#keyboard-shortcuts-1)
- [XeLL DX12 Basic Sample](#xell-dx12-basic-sample)
  - [Windows Build Steps](#windows-build-steps-4)
  - [Command Line Options](#command-line-options-4)
  - [Keyboard Shortcuts](#keyboard-shortcuts-2)

## System Requirements

- Windows 10/11
- Compatible GPU
- Visual Studio 2019 or newer
- CMake 3.20 or newer

## XeSS-SR DX12 Basic Sample

Demonstrates basic XeSS-SR integration with DirectX 12.

### Windows Build Steps

Run a 'Developer Command Prompt for Visual Studio 2019', then execute the following commands:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command Line Options

- `-gpu_id value`: Selects GPU by ID.

### Keyboard Shortcuts

- `1`: Display input color.
- `2`: Display input velocity.
- `3`: Display output.
- `space`: Pause animation.

---

## XeSS-SR Vulkan Basic Sample

Demonstrates basic XeSS-SR integration with Vulkan.

### Windows Build Steps

Install the Vulkan SDK from [https://www.lunarg.com/vulkan-sdk/](https://www.lunarg.com/vulkan-sdk/).

The sample requires the "glm" library source code to be unpacked into the `basic_sample_vk/glm` directory. Use the following commands to download and unpack the glm library:

```powershell
powershell -command "Invoke-WebRequest https://github.com/g-truc/glm/releases/download/1.0.1/glm-1.0.1-light.zip -OutFile glm-1.0.1-light.zip"
powershell -command "Expand-Archive -Path glm-1.0.1-light.zip -DestinationPath ."
```

Run a 'Developer Command Prompt for Visual Studio 2019', then execute the following commands:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command Line Options

- `-gpu_id value`: Selects GPU by ID.

---

## XeSS-SR DX11 Basic Sample

Demonstrates basic XeSS-SR integration with DirectX 11.

### Windows Build Steps

Run a 'Developer Command Prompt for Visual Studio 2019', then execute the following commands:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command Line Options

- `-gpu_id value`: Selects GPU by ID.

---

## XeSS-FG DX12 Basic Sample

Demonstrates basic XeSS-FG integration with DirectX 12.

### Windows Build Steps

Run a 'Developer Command Prompt for Visual Studio 2019', then execute the following commands:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command Line Options

- `-gpu_id value`: Selects GPU by ID.
- `-async`: Enables asynchronous flip.
- `-fps value`: Forces the sample to run at a specified FPS.
- `-maximized`: Maximizes the application window.
- `-width value`: Sets the window width.
- `-height value`: Sets the window height.
- `-top`: Makes the window topmost.
- `-tag_interpolated_frames value`: Tags interpolated frames with purple stripes (default: true).
- `-fullscreen`: Starts the application in exclusive fullscreen mode.

### Keyboard Shortcuts

- `3`: Toggle frame interpolation ON/OFF.
- `space`: Pause animation.
- `F3`: Switch between 1080p and 1440p.
- `F4`: Switch between exclusive fullscreen and windowed mode.
- `F5`: Display only interpolated frames.
- `F6`: Tag interpolated frames.

---

## XeLL DX12 Basic Sample

Demonstrates basic XeLL integration with DirectX 12.

### Windows Build Steps

Run a 'Developer Command Prompt for Visual Studio 2019', then execute the following commands:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

### Command Line Options

- `-gpu_id value`: Selects GPU by ID.
- `-async`: Enables asynchronous flip.
- `-fps value`: Forces the sample to run at a specified FPS.
- `-fullscreen`: Starts the application in exclusive fullscreen mode.

### Keyboard Shortcuts

- `l/L`: Toggle latency reduction ON/OFF.
