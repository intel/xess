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

TextureCube<float3> radianceIBLTexture      : register(t10);
TextureCube<float3> irradianceIBLTexture    : register(t11);
Texture2D<float> texSSAO			        : register(t12);
Texture2D<float> texSunShadow			    : register(t13);
Texture2D<float2> BRDFLUTTexture            : register(t14);

// Numeric constants
static const float PI = 3.14159265;
static const float3 kDielectricSpecular = float3(0.04, 0.04, 0.04);

struct SurfaceProperties
{
    float3 N;
    float3 V;
    float3 c_diff;
    float3 c_spec;
    float roughness;
    float alpha; // roughness squared
    float alphaSqr; // alpha squared
    float NdotV;
};

struct LightProperties
{
    float3 L;
    float NdotL;
    float LdotH;
    float NdotH;
};

//
// Shader Math
//

float Pow5(float x)
{
    float xSq = x * x;
    return xSq * xSq * x;
}

// Shlick's approximation of Fresnel
float3 Fresnel_Shlick(float3 F0, float3 F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

float Fresnel_Shlick(float F0, float F90, float cosine)
{
    return lerp(F0, F90, Pow5(1.0 - cosine));
}

// Lambertian diffuse
float3 Diffuse_Lambertian(SurfaceProperties Surface)
{
    return Surface.c_diff / PI;
}

// Burley's diffuse BRDF
float3 Diffuse_Burley(SurfaceProperties Surface, LightProperties Light)
{
    float fd90 = 0.5 + 2.0 * Surface.roughness * Light.LdotH * Light.LdotH;
    return Surface.c_diff * Fresnel_Shlick(1, fd90, Light.NdotL).x * Fresnel_Shlick(1, fd90, Surface.NdotV).x;
}

// GGX specular D (normal distribution)
float Specular_D_GGX(SurfaceProperties Surface, LightProperties Light)
{
    float lower = Light.NdotH * Light.NdotH * (Surface.alphaSqr - 1.0) + 1.0;
    return Surface.alphaSqr / max(1e-6, PI * lower * lower);
}

float SchlickGGX(float NdotV, float alphaSqr)
{
    return 2.0 * NdotV / (NdotV + sqrt(alphaSqr + (1.0 - alphaSqr) * (NdotV * NdotV)));
}
  
float G_Schlick_Smith(SurfaceProperties Surface, LightProperties Light)
{
    float GV = SchlickGGX(Surface.NdotV, Surface.alphaSqr);
    float GL = SchlickGGX(Light.NdotL, Surface.alphaSqr);
    
    return GV * GL;
}

// Schlick-Smith specular visibility with Hable's LdotH approximation
float G_Shlick_Smith_Hable(SurfaceProperties Surface, LightProperties Light)
{
    return 1.0 / lerp(Light.LdotH * Light.LdotH, 1, Surface.alphaSqr * 0.25);
}

// A microfacet based BRDF.
// alpha:    This is roughness squared as in the Disney PBR model by Burley et al.
// c_spec:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals.  This is the specular albedo.
// NdotV, NdotL, LdotH, NdotH:  vector dot products
//  N - surface normal
//  V - normalized view vector
//  L - normalized direction to light
//  H - normalized half vector (L+V)/2 -- halfway between L and V
float3 Specular_BRDF(SurfaceProperties Surface, LightProperties Light)
{
    // Normal Distribution term
    float ND = Specular_D_GGX(Surface, Light);

    // Geometric Visibility term
    //float GV = G_Shlick_Smith_Hable(Surface, Light);
    float GV = G_Schlick_Smith(Surface, Light);

    // Fresnel term
    float3 F = Fresnel_Shlick(Surface.c_spec, 1.0, Light.LdotH);

    return ND * GV * F;
}

float3 Diffuse_IBL(SurfaceProperties Surface, float3x3 envRotation)
{
#if 0
        // This is nicer but more expensive, and specular can often drown out the diffuse anyway
        float LdotH = saturate(dot(Surface.N, normalize(Surface.N + Surface.V)));
        float fd90 = 0.5 + 2.0 * Surface.roughness * LdotH * LdotH;
        float3 DiffuseBurley = Surface.c_diff * Fresnel_Shlick(1, fd90, Surface.NdotV);
        return DiffuseBurley * irradianceIBLTexture.Sample(cubeMapSampler, mul(envRotation, Surface.N));
#else
        float3 diffuseLight = irradianceIBLTexture.Sample(cubeMapSampler, mul(envRotation, Surface.N)).rgb;
        float3 diffuse = diffuseLight * Surface.c_diff;
        return diffuse;
#endif
}

float3 Specular_IBL(SurfaceProperties Surface, float IBLRange, float IBLBias, float3x3 envRotation)
{
    float lod = Surface.roughness * IBLRange + IBLBias;
    float2 brdf = BRDFLUTTexture.Sample(clampSampler, float2(Surface.NdotV, Surface.roughness)).rg;
    float3 specularLight = radianceIBLTexture.SampleLevel(cubeMapSampler, mul(envRotation, reflect(-Surface.V, Surface.N)), lod).xyz;
    float3 specular = specularLight * (Surface.c_spec * brdf.x + brdf.y);

    return specular;
}

float3 AmbientLight(
    float3	diffuse,	// Diffuse albedo
    float	ao,			// Pre-computed ambient-occlusion
    float3	lightColor	// Radiance of ambient light
    )
{
    return ao * diffuse * lightColor;
}