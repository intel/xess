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
// Author(s):	James Stanard
//              Justin Saunders (ATG)

#include "Common.hlsli"
#include "LightingPBR.hlsli"
#include "Core/Shaders/ShaderUtility.hlsli"

//#define DEBUG_CHANNEL 1

Texture2D<float4> baseColorTexture          : register(t0);
Texture2D<float3> metallicRoughnessTexture  : register(t1);
Texture2D<float1> occlusionTexture          : register(t2);
Texture2D<float3> emissiveTexture           : register(t3);
Texture2D<float3> normalTexture             : register(t4);

SamplerState baseColorSampler               : register(s0);
SamplerState metallicRoughnessSampler       : register(s1);
SamplerState occlusionSampler               : register(s2);
SamplerState emissiveSampler                : register(s3);
SamplerState normalSampler                  : register(s4);

cbuffer MaterialConstants : register(b0)
{
    float4 baseColorFactor;
    float3 emissiveFactor;
    float normalTextureScale;
    float2 metallicRoughnessFactor;
    uint flags;
}

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
#ifndef NO_TANGENT_FRAME
    float4 tangent : TANGENT;
#endif
    float2 uv0 : TEXCOORD0;
#ifndef NO_SECOND_UV
    float2 uv1 : TEXCOORD1;
#endif
    float3 worldPos : TEXCOORD2;
    float3 sunShadowCoord : TEXCOORD3;
};

// Flag helpers
static const uint BASECOLOR = 0;
static const uint METALLICROUGHNESS = 1;
static const uint OCCLUSION = 2;
static const uint EMISSIVE = 3;
static const uint NORMAL = 4;
#ifdef NO_SECOND_UV
#define UVSET( offset ) vsOutput.uv0
#else
#define UVSET( offset ) lerp(vsOutput.uv0, vsOutput.uv1, (flags >> offset) & 1)
#endif

float3 sRGBToLinear(float3 srgbColor)
{
    return RemoveDisplayProfile(srgbColor, DISPLAY_PLANE_FORMAT);
}

float4 sRGBToLinear(float4 srgbColor)
{
    return float4(sRGBToLinear(srgbColor.xyz), srgbColor.w);
}

float3 ComputeNormal(VSOutput vsOutput, bool isFrontFace)
{
    float3 normal = normalize(vsOutput.normal);

#ifdef NO_TANGENT_FRAME
    normal = normalize(lerp(-normal, normal, float(isFrontFace)));
    return normal;
#else
    // Construct tangent frame
    float3 tangent = normalize(vsOutput.tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * vsOutput.tangent.w;

    float3x3 tangentFrame;
    if (isFrontFace)
    {
        tangentFrame = float3x3(tangent, bitangent, normal);
    }
    else
    {
        tangentFrame = float3x3(-tangent, -bitangent, -normal);
    }

    // Read normal map and convert to SNORM (TODO:  convert all normal maps to R8G8B8A8_SNORM?)
    normal = normalTexture.SampleBias(normalSampler, UVSET(NORMAL), ViewMipBias) * 2.0 - 1.0;

    // glTF spec says to normalize N before and after scaling, but that's excessive
    normal = normalize(normal * float3(normalTextureScale, normalTextureScale, 1));

    // Multiply by transpose (reverse order)
    return mul(normal, tangentFrame);
#endif
}


[RootSignature(Renderer_RootSig)] 
float4 main(VSOutput vsOutput, bool isFrontFace : SV_IsFrontFace)
    : SV_Target0
{
    // Load and modulate textures
    float4 baseColor = baseColorFactor * sRGBToLinear(baseColorTexture.SampleBias(baseColorSampler, UVSET(BASECOLOR), ViewMipBias));
    float2 metallicRoughness = metallicRoughnessFactor * 
        metallicRoughnessTexture.SampleBias(metallicRoughnessSampler, UVSET(METALLICROUGHNESS), ViewMipBias).bg; // b: metallic, g: roughness
    float occlusion = occlusionTexture.SampleBias(occlusionSampler, UVSET(OCCLUSION), ViewMipBias);
    float3 emissive = emissiveFactor * sRGBToLinear(emissiveTexture.SampleBias(emissiveSampler, UVSET(EMISSIVE), ViewMipBias));
    float3 normal = ComputeNormal(vsOutput, isFrontFace);

    SurfaceProperties Surface;
    Surface.N = normal;
    Surface.V = normalize(ViewerPos - vsOutput.worldPos);
    Surface.NdotV = clamp(dot(Surface.N, Surface.V), 1e-6, 1.0f);
    Surface.c_diff = baseColor.rgb * (1 - kDielectricSpecular) * (1 - metallicRoughness.x) * occlusion;
    Surface.c_spec = lerp(kDielectricSpecular, baseColor.rgb, metallicRoughness.x) * occlusion;
    Surface.roughness = metallicRoughness.y;
    Surface.alpha = metallicRoughness.y * metallicRoughness.y;
    Surface.alphaSqr = Surface.alpha * Surface.alpha;

    // Begin accumulating light starting with emissive
    float3 colorAccum = emissive;

    // Add IBL
    colorAccum += Diffuse_IBL(Surface, EnvRotation);
    colorAccum += Specular_IBL(Surface, IBLRange, IBLBias, EnvRotation);

    uint2 pixelPos = uint2(vsOutput.position.xy);
    float ssao = texSSAO[pixelPos];
    
    // Apply AO
    Surface.c_diff *= ssao;
    Surface.c_spec *= ssao;

    // Apply sun lighting
    colorAccum += ApplyDirectionalLight(Surface, SunDirection, SunIntensity, vsOutput.sunShadowCoord, texSunShadow );
    
    // Apply other scene lighting
    ShadeLights(colorAccum, pixelPos, Surface, vsOutput.worldPos, flags);

#ifndef DEBUG_CHANNEL
    return float4(colorAccum, baseColor.a);
#else
    int debug = (int)DebugFlag;
    if (debug == 1) // albedo
    {
        return baseColor;
    }
    else if (debug == 2) // normal
    {
        return float4(normal, baseColor.a);
    }
    else if (debug == 3) // metal
    {
        return float4(metallicRoughness.x, metallicRoughness.x, metallicRoughness.x, baseColor.a);
    }
    else if (debug == 4) // roughness
    {
        return float4(metallicRoughness.y, metallicRoughness.y, metallicRoughness.y, baseColor.a);
    }
    else if (debug == 5) // occlusion
    {
        return float4(occlusion, occlusion, occlusion, baseColor.a);
    }
    else if (debug == 6) // emissive
    {
        return float4(emissive, baseColor.a);
    }
    else
        return float4(colorAccum, baseColor.a);
#endif
}
