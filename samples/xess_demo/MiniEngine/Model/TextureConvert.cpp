//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author(s):  James Stanard
//             Chuck Walbourn (ATG)
//
// This code depends on DirectXTex
//

#include "TextureConvert.h"
#include "../Core/Utility.h"
#include "DirectXTex.h"

using namespace DirectX;

#define GetFlag(f) ((Flags & f) != 0)

void CompileTextureOnDemand(const std::wstring& originalFile, uint32_t flags)
{
    std::wstring ddsFile = Utility::RemoveExtension(originalFile) + L".dds";

    struct _stat64 ddsFileStat, srcFileStat;

    bool srcFileMissing = _wstat64(originalFile.c_str(), &srcFileStat) == -1;
    bool ddsFileMissing = _wstat64(ddsFile.c_str(), &ddsFileStat) == -1;

    if (srcFileMissing && ddsFileMissing)
    {
        LOG_ERRORF("Texture %s is missing.", Utility::WideStringToUTF8(Utility::RemoveBasePath(originalFile)).c_str());
        return;
    }

    // If we can find the source texture and the DDS file is older, reconvert.
    if (ddsFileMissing || !srcFileMissing && ddsFileStat.st_mtime < srcFileStat.st_mtime)
    {
        LOG_INFOF("DDS texture %s missing or older than source.  Rebuilding.", Utility::WideStringToUTF8(Utility::RemoveBasePath(originalFile)).c_str());
        ConvertToDDS(originalFile, flags);
    }
}

bool ConvertToDDS( const std::wstring& filePath, uint32_t Flags )
{
    bool bInterpretAsSRGB =	GetFlag(kSRGB);
    bool bPreserveAlpha =	GetFlag(kPreserveAlpha);
    bool bContainsNormals = GetFlag(kNormalMap);
    bool bBumpMap =			GetFlag(kBumpToNormal);
    bool bBlockCompress =	GetFlag(kDefaultBC);
    bool bUseBestBC =		GetFlag(kQualityBC);
    bool bFlipImage =       GetFlag(kFlipVertical);

    // Can't be both
    ASSERT(!bInterpretAsSRGB || !bContainsNormals);
    ASSERT(!bPreserveAlpha || !bContainsNormals);

    LOG_INFOF( "Converting file \"%s\" to DDS.", Utility::WideStringToUTF8(filePath).c_str() );

    // Get extension as utf8 (ascii)
    std::wstring ext = Utility::ToLower(Utility::GetFileExtension(filePath));

    // Load texture image
    TexMetadata info;
    std::unique_ptr<ScratchImage> image(new ScratchImage);

    bool isDDS = false;
    bool isHDR = false;
    if ( ext == L"dds" )
    {
        // TODO:  It might be desired to compress or recompress existing DDS files
        //Utility::Printf("Ignoring existing DDS \"%ws\".\n", filePath.c_str());
        //return false;

        isDDS = true;
        HRESULT hr = LoadFromDDSFile( filePath.c_str(), DDS_FLAGS_NONE, &info, *image );
        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not load texture \"%s\" (DDS: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
            return false;
        }
    }
    else if ( ext == L"tga" )
    {
        HRESULT hr = LoadFromTGAFile( filePath.c_str(), &info, *image );
        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not load texture \"%s\" (TGA: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
            return false;
        }
    }
    else if ( ext == L"hdr" )
    {
        isHDR = true;
        HRESULT hr = LoadFromHDRFile( filePath.c_str(), &info, *image );
        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not load texture \"%s\" (HDR: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
            return false;
        }
    }
    else if ( ext == L"exr" )
    {
#ifdef USE_OPENEXR
        isHDR = true;
        HRESULT hr = LoadFromEXRFile(filePath.c_str(), &info, *image);
        if (FAILED(hr))
        {
            LOG_ERRORF("Could not load texture \"%s\" (EXR: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr);
            return false;
        }
#else
        LOG_ERROR("OpenEXR not supported for this build of the content exporter.");
        return false;
#endif
    }
    else
    {
        //WIC_FLAGS wicFlags = WIC_FLAGS_NONE;
        //if (g_pScene->Settings().bIgnoreSRGB)
        //    wicFlags |= WIC_FLAGS_IGNORE_SRGB;

        WIC_FLAGS wicFlags = WIC_FLAGS_IGNORE_SRGB;

        HRESULT hr = LoadFromWICFile( filePath.c_str(), wicFlags, &info, *image );
        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not load texture \"%s\" (WIC: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
            return false;
        }
    }

    if ( info.width > 16384 || info.height > 16384 )
    {
        LOG_ERRORF("Texture size (%Iu,%Iu) too large for feature level 11.0 or later (16384) \"%s\".",
            info.width, info.height, Utility::WideStringToUTF8(filePath).c_str());
        return false;
    }

    if (bFlipImage)
    {
        std::unique_ptr<ScratchImage> timage(new ScratchImage);

        HRESULT hr = FlipRotate( image->GetImages()[0], TEX_FR_FLIP_VERTICAL, *timage );

        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not flip image \"%s\" (%08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
        }
        else
        {
            image.swap(timage);
        }
       
    }

    DXGI_FORMAT tformat;
    DXGI_FORMAT cformat;

    if (isHDR)
    {
        tformat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
        cformat = bBlockCompress ? DXGI_FORMAT_BC6H_UF16 : DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    }
    else if (bBlockCompress)
    {
        tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
        if (bUseBestBC)
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
        else if (bPreserveAlpha)
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
        else
            cformat = bInterpretAsSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
    }
    else
    {
        cformat = tformat = bInterpretAsSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    if (bBumpMap)
    {
        std::unique_ptr<ScratchImage> timage(new ScratchImage);

        HRESULT hr = ComputeNormalMap(image->GetImages(), image->GetImageCount(), image->GetMetadata(),
            CNMAP_CHANNEL_LUMINANCE, 10.0f, tformat, *timage);

        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not compute normal map for \"%s\" (%08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
        }
        else
        {
            image.swap(timage);
            info.format = tformat;
        }
    }
    else if ( info.format != tformat )
    {
        std::unique_ptr<ScratchImage> timage(new ScratchImage);

        HRESULT hr = Convert( image->GetImages(), image->GetImageCount(), image->GetMetadata(),
            tformat, TEX_FILTER_DEFAULT, 0.5f, *timage );

        if (FAILED(hr))
        {
            LOG_ERRORF( "Could not convert \"%s\" (%08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
        }
        else
        {
            image.swap(timage);
            info.format = tformat;
        }
    }

    // Handle mipmaps
    if (info.mipLevels == 1)
    {
        std::unique_ptr<ScratchImage> timage(new ScratchImage);

        HRESULT hr = GenerateMipMaps( image->GetImages(), image->GetImageCount(), image->GetMetadata(), TEX_FILTER_DEFAULT, 0, *timage );

        if (FAILED(hr))
        {
            LOG_ERRORF( "Failing generating mimaps for \"%s\" (WIC: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
        }
        else
        {
            image.swap(timage);
        }
    }

    // Handle compression
    if (bBlockCompress)
    {
        if (info.width % 4 || info.height % 4)
        {
            LOG_ERRORF( "Texture size (%Iux%Iu) not a multiple of 4 \"%s\", so skipping compress.", info.width, info.height, Utility::WideStringToUTF8(filePath).c_str() );
        }
        else
        {
            std::unique_ptr<ScratchImage> timage(new ScratchImage);

            HRESULT hr = Compress( image->GetImages(), image->GetImageCount(), image->GetMetadata(), cformat, TEX_COMPRESS_DEFAULT, 0.5f, *timage );
            if (FAILED(hr))
            {
                LOG_ERRORF( "Failing compressing \"%s\" (WIC: %08X).", Utility::WideStringToUTF8(filePath).c_str(), hr );
            }
            else
            {
                image.swap(timage);
            }
        }
    }

    // Rename file extension to DDS
    const std::wstring wDest = Utility::RemoveExtension(filePath) + L".dds";
    
    // Save DDS
    HRESULT hr = SaveToDDSFile( image->GetImages(), image->GetImageCount(), image->GetMetadata(), DDS_FLAGS_NONE, wDest.c_str() );
    if( FAILED( hr ) )
    {
        LOG_ERRORF( "Could not write texture to file \"%s\" (%08X).", Utility::WideStringToUTF8(wDest).c_str(), hr );
        return false;
    }

    return true;
}
