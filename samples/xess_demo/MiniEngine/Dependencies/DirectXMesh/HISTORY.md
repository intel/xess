# DirectXMesh geometry processing library

http://go.microsoft.com/fwlink/?LinkID=324981

Releases available for download on [GitHub](https://github.com/microsoft/DirectXMesh/releases)

## Release History

### May 9, 2022
* Minor updates for VS 2022 (17.2)
* CMake project updates (now supports MSVC, clang/LLVM, and MinGW)
* Retired VS 2017 projects
* Reformat source using updated .editorconfig settings

### March 24, 2022
* Update build switches for SDL recommendations
* CMake project updates and UWP platform CMakePresets
* Code cleaup for meshconvert tool
* Optional C++17 usage in WaveFrontReader.h

### February 28, 2022
* Code and project review
* Added CMakePresets.json

### November 8, 2021
* VS 2022 support
* Fixed locale issue with WaveFront OBJ reading/writing
* Minor code and project review
* VS 2017 projects updated to require the Windows 10 SDK (19401)
* meshconvert: Fixed potential locale issue with ``-flist``

### September 28, 2021
* Fixed overflow case in meshlet generation with degenerate triangles
* Minor code review and project cleanup

### June 9, 2021
* meshconvert: improved ``-flist`` switch to support wildcards and file exclusions
* FlexibleVertexFormat.h header added to Utilities folder
* Code cleanup for meshconvert

### April 6, 2021
* New function **ConcatenateMesh**
* Minor code and project cleanup
* meshconvert: Updated with descriptions for HRESULT failure codes

### January 9, 2021
* Windows Subsystem for Linux support
* Code review for improved conformance
* CMake updated to support package install

### November 11, 2020
* Code review and project updates

### August 15, 2020
* Added GDK projects
* Code review and project updates
* meshconvert: Added ``-fn``, ``-fuc``, and ``-fc`` switches to control vertex format

### July 2, 2020
* Minor warning fixes for VS 2019 (16.7)

### June 1, 2020
* New functions **ComputeMeshlets** and **ComputeCullData** for use with Direct3D 12 Mesh Shaders
* Converted to typed enum bitmask flags (see release notes for details on this potential *breaking change*)
  + **ComputeNormals**, **Validate**
* Minor fix for degenerate cases in point-reps generation
* CMake project updates

### May 10, 2020
* Minor code review
* meshconvert: Updated with ``-l`` switch for case-sensitive file systems

### April 3, 2020
* Code review (``constexpr`` / ``noexcept`` usage)
* CMake updated for PCH usage with 3.16 or later

### February 14, 2020
* meshconvert: now supports exporting to WaveFront Object (OBJ) files
* Retired VS 2015 projects

### December 17, 2019
* Added ARM64 platform to VS 2019 Win32 desktop Win10 project
* CMake project
* Code cleaup

### August 21, 2019
* Added ``meshconvert`` to CMake project
* Code cleanup

### June 30, 2019
* Clang/LLVM warning cleanup
* Renamed ``DirectXMesh_Windows10.vcxproj`` to ``_Windows10_2017.vcxproj``
* Added VS 2019 UWP project

### May 30, 2019
* Added CMake project files
* Code cleanup

### April 26, 2019
* Added VS 2019 desktop projects
* Officially dropped Windows Vista support
* WaveFrontReader utility header updated for RMA texture in MTL

### February 7, 2019
* meshconvert: added ``-sdkmesh2`` switch for PBR materials
* WaveFrontReader utility header updated for some extended material textures

### November 16, 2018
* VS 2017 updated for Windows 10 October 2018 Update SDK (17763)
* ARM64 platform configurations added to UWP projects

### October 25, 2018
* Use UTF-8 instead of ANSI for narrow strings
* Minor project fix

### August 17, 2018
* Updated for VS 2017 15.8
* Code cleanup

### July 3, 2018
* Code and project cleanup
* meshconvert: added ``-ib32`` switch

### May 31, 2018
* VS 2017 updated for Windows 10 April 2018 Update SDK (17134)

### May 11, 2018
* Updated for VS 2017 15.7 update warnings
* Code and project cleanup
* Retired VS 2013 projects

### April 23, 2018
* Code and project cleanup

### February 7, 2018
* New functions: **WeldVertices**, **CompactVB**, **OptimizeFacesLRU**
* Added new optional parameter to **OptimizeVertices**
* Fixed bug in remap functions which were applying vertex remaps backwards
  + **FinalizeIB**, **FinalizeVB**, and **FinalizeVBAndPointReps**
* meshconvert: added ``-oplru`` switch
* WaveFrontReader utility header made more robust
* Minor code cleanup

### December 13, 2017
* Updated for VS 2017 15.5 update warnings
* Support building library with ``_XM_NO_XMVECTOR_OVERLOADS_``
* Added support for relative face indices to ``WaveFrontReader.h`` utility header

### November 1, 2017
* VS 2017 updated for Windows 10 Fall Creators Update SDK (16299)
* Code reformat and cleanup

### September 22, 2017
* Updated for VS 2017 15.3 update ``/permissive-`` changes
* meshconvert: added ``-flist`` option
* Minor code cleanup

### July 28, 2017
* Code cleanup

### April 24, 2017
* VS 2017 project updates

### April 7, 2017
* VS 2017 updated for Windows Creators Update SDK (15063)
* VBReader/VBWriter **GetElement** adapter for ``GetElement11``

### January 31, 2017
* DirectX 12 support for **VBReader**, **VBWriter**, **IsValid**, and **ComputeInputLayout**
* *breaking change*: VBReader/VBWriter method **GetElement** method now named ``GetElement11`` for DirectX 11

### October 27, 2016
* x2bias optional parameter for VB reader/writer
* Code cleanup

### September 14, 2016
* meshconvert: added wildcard support for input filename and optional ``-r`` switch for recursive search
* Code cleanup

### August 2, 2016
* Updated for VS 2015 Update 3 and Windows 10 SDK (14393)

### July 19, 2016
* meshconvert command-line tool updated with ``-flipu`` switch

### June 27, 2016
* Code cleanup

### April 26, 2016
* Retired VS 2012 projects and obsolete adapter code
* Minor code and project file cleanup

### November 30, 2015
* meshconvert command-line tool updated with ``-flipv`` and ``-flipz`` switches; removed ``-fliptc``
* Updated for VS 2015 Update 1 and Windows 10 SDK (10586)

### October 30, 2015
* Minor code cleanup

### August 18, 2015
* Xbox One platform updates

### July 29, 2015
* Updated for VS 2015 and Windows 10 SDK RTM
* Retired VS 2010 projects
* WaveFrontReader: updated utility to minimize debug output

### July 8, 2015
* Minor SAL fix and project cleanup

### March 27, 2015
* Added projects for Windows apps Technical Preview
* Fixed attributes usage for **OptimizeFacesEx**
* meshconvert: fix when importing from .vbo
* Minor code cleanup

### November 14, 2014
* meshconvert: sample improvements and fixes
* Added workarounds for potential compiler bug when using
* VB reader/writer

### October 28, 2014
* meshconvert command-line sample
* Added **VBReader/VBWriter::GetElement** method
* Added more **ComputeTangentFrame** overloads
* Explicit calling-convention annotation for public headers
* Windows phone 8.1 platform support
* Minor code and project cleanup

### June 27, 2014
* Original release
