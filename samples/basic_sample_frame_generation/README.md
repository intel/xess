# XeSS-FG DX12 Basic Sample

This sample demonstrates basic XeSS-FG integration with DirectX 12.

## Windows Build Steps

Open a "Developer Command Prompt for Visual Studio 2019" and run:

```console
cmake -S . -B build
cmake --build build --config Debug --target ALL_BUILD
```

## Command Line Options

- `-gpu_id <value>`: Select a specific GPU.
- `-fps <value>`: Lock the application to a specific framerate.
- `-maximized`: Launch with maximized window.
- `-width <value>`: Set custom window width.
- `-height <value>`: Set custom window height.
- `-top`: Make the window stay on top.
- `-tag_interpolated_frames <value>`: Mark interpolated frames with purple stripes (default: true).
- `-fullscreen`: Launch in exclusive fullscreen mode.
- `-init_from_swap_chain`: Initialize XeSS-FG proxy swap chain from an existing DXGI swap chain instance
  using `xefgSwapChainD3D12InitFromSwapChain` (default: uses `xefgSwapChainD3D12InitFromSwapChainDesc`).
- `-external_descriptor_heap`: Use external descriptor heap during frame generation.

## Keyboard Shortcuts

- `3`: Toggle frame interpolation ON/OFF.
- `4`: Cycle through number of generated frames (1 to maximum supported).
- `5`: Toggle UI composition ON/OFF.
- `v`: Toggle vertical synchronization ON/OFF.
- `Space`: Pause animation.
- `F3`: Switch between 1080p and 1440p resolution.
- `F4`: Toggle between exclusive fullscreen and windowed modes.
- `F5`: Toggle display of interpolated frames only.
- `F6`: Toggle interpolated frame markers.
