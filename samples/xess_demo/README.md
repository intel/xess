# XeSS-Demo
This is a demo of Intel® Xe Super Sampling (XeSS), using the MiniEngine from [DirectX-Graphics-Sample](https://github.com/microsoft/DirectX-Graphics-Samples)


## Folder contents
| Folder / file | Description |
| ----------- | ----------- |
| README.md | This readme file |
| Source | Source code folder of the sample application |
| MiniEngine | Folder of the modified version of Microsoft MiniEngine supporting XeSS |
| Assets | Art assets folder |
| CMakeLists.txt | Build configuration file for CMake |

## Sample code explanation
| Folder / file | Description |
| ----------- | ----------- |
| DemoApp.h/cpp | Application entry of the sample |
| DemoCameraController.h/cpp | Camera controller of the sample |
| DemoGui.h/cpp | ImGui based GUI code of the sample |
| XeSS/XeSSRuntime.h/cpp | XeSS DX12 runtime wrapper |
| XeSS/XeSSProcess.h/cpp | Management for XeSS processing |
| XeSS/XeSSJitter.h/cpp | Camera jittering management for XeSS |
| XeSS/XeSSDebug.h/cpp | Debugging for XeSS |
| Shaders/XeSSConvertLowResVelocityCS.hlsl | Shader code which converts velocity buffer from low-res to high-res |
| Shaders/XeSSGenerateHiResVelocityCS.hlsl | Shader code which generates high-res velocity buffer |
| Shaders/Debug/*.hlsl | Shaders for debugging purposes |

## Keyboard and mouse controls:
- Toggle GUI display: F1
- Forward/backward/strafe: WASD (FPS controls) / mouse wheel
- Up/down: E/Q key
- Yaw/pitch: Mouse move when any mouse button down
- Toggle fast movement:  Shift key
- Open debug menu: Backspace
- Navigate debug menu: Arrow keys
- Toggle debug menu item: Return key
- Adjust debug menu value:  Left/Right arrow keys

## Build the sample:
```
> cmake -S . -B build
> cmake --build build --config Debug --target ALL_BUILD
```
