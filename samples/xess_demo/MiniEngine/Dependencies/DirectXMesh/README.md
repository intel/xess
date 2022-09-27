![DirectX Logo](https://raw.githubusercontent.com/wiki/Microsoft/DirectXMesh/X_jpg.jpg)

# DirectXMesh geometry processing library

http://go.microsoft.com/fwlink/?LinkID=324981

Copyright (c) Microsoft Corporation.

**May 9, 2022**

This package contains DirectXMesh, a shared source library for performing various geometry content processing operations including generating normals and tangent frames, triangle adjacency computations, vertex cache optimization, and meshlet generation.

This code is designed to build with Visual Studio 2019 (16.7 or later), Visual Studio 2022, or clang for Windows v11 or later. Use of the Windows 10 May 2020 Update SDK ([19041](https://walbourn.github.io/windows-10-may-2020-update-sdk/)) or later is required.

These components are designed to work without requiring any content from the legacy DirectX SDK. For details, see [Where is the DirectX SDK?](https://aka.ms/dxsdk).

## Directory Layout

* ``DirectXMesh\``

   + This contains the DirectXMesh library.

> The majority of the header files here are intended for  implementation the library only (``DirectXMeshP.h``, ``scoped.h``, etc.). Only ``DirectXMesh.h`` and ``DirectXMesh.inl`` are meant as a 'public' headers for the library.

* ``Utilities\``

  + This contains helper code related to mesh processing that is not general enough to be part of the DirectXMesh library.
    * ``WaveFrontReader.h``: Contains a simple C++ class for reading mesh data from a WaveFront OBJ file.

* ``Meshconvert\``

  + This DirectXMesh sample is an implementation of the ``meshconvert`` command-line texture utility from the legacy DirectX SDK utilizing DirectXMesh rather than D3DX.

> This tool does not support legacy ``.X`` files, but can export ``CMO``, ``SDKMESH``, and ``VBO`` files.

## Documentation

Documentation is available on the [GitHub wiki](https://github.com/Microsoft/DirectXMesh/wiki).

## Notices

All content and source code for this package are subject to the terms of the [MIT License](https://github.com/microsoft/DirectXMesh/blob/main/LICENSE).

For the latest version of DirectXMesh, bug reports, etc. please visit the project site on [GitHub](https://github.com/microsoft/DirectXMesh).

## Release Notes

* Starting with the June 2020 release, this library makes use of typed enum bitmask flags per the recommendation of the _C++ Standard_ section *17.5.2.1.3 Bitmask types*. This is consistent with Direct3D 12's use of the ``DEFINE_ENUM_FLAG_OPERATORS`` macro. This may have *breaking change* impacts to client code:

  * You cannot pass the ``0`` literal as your flags value. Instead you must make use of the appropriate default enum value: ``CNORM_DEFAULT``, ``VALIDATE_DEFAULT``, or ``MESHLET_DEFAULT``.

  * Use the enum type instead of ``DWORD`` if building up flags values locally with bitmask operations. For example, ```CNORM_FLAGS flags = CNORM_DEFAULT; if (...) flags |= CNORM_WIND_CW;```

* The UWP projects and the Win10 classic desktop project include configurations for the ARM64 platform. Building these requires installing the ARM64 toolset.

* When using clang/LLVM for the ARM64 platform, the Windows 11 SDK ([22000](https://walbourn.github.io/windows-sdk-for-windows-11/)) is required.

## Support

For questions, consider using [Stack Overflow](https://stackoverflow.com/questions/tagged/directxtk) with the *directxtk* tag, or the [DirectX Discord Server](https://discord.gg/directx) in the *dx12-developers* or *dx9-dx11-developers* channel.

For bug reports and feature requests, please use GitHub [issues](https://github.com/microsoft/DirectXMesh/issues) for this project.

## Contributing

This project welcomes contributions and suggestions. Most contributions require you to agree to a Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us the rights to use your contribution. For details, visit https://cla.opensource.microsoft.com.

When you submit a pull request, a CLA bot will automatically determine whether you need to provide a CLA and decorate the PR appropriately (e.g., status check, comment). Simply follow the instructions provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## Trademarks

This project may contain trademarks or logos for projects, products, or services. Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general). Use of Microsoft trademarks or logos in modified versions of this project must not cause confusion or imply Microsoft sponsorship. Any use of third-party trademarks or logos are subject to those third-party's policies.

## Credits

The DirectXMesh library is the work of Chuck Walbourn, with contributions from Dr. Hugues Hoppe, Alex Nankervis, James Stanard, Craig Peeper, and the numerous other Microsoft engineers who developed the D3DX utility library over the years.

Thanks to Matt Hurliman for his contribution of the meshlet generation functions.

Thanks to Adrian Stone (Game Angst) for the public domain implementation of Tom Forsyth's linear-speed vertex cache optimization, and thanks to Tom Forsyth for his contribution.
