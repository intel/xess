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
// Author:  James Stanard 
//          Julia Careaga
//

#include "ParticleUpdateCommon.hlsli"
#include "ParticleUtility.hlsli"

Texture2DArray<float4> ColorTex : register(t1);
Texture2D<float> LinearDepthTex : register(t2);

#ifdef ENABLE_PARTICLE_RESPONSIVE_MASK
struct MRT
{
    float4 Color : SV_Target0;
    float ResponsiveMask : SV_Target1;
};
#endif

[RootSignature(Particle_RootSig)]
#ifdef ENABLE_PARTICLE_RESPONSIVE_MASK
MRT main(ParticleVertexOutput input)
#else
float4 main(ParticleVertexOutput input ) : SV_Target0
#endif
{
    float3 uv = float3(input.TexCoord.xy, input.TexID);
    float4 TextureColor = ColorTex.Sample( gSampLinearBorder, uv );
    TextureColor.a *= saturate(1000.0 * (LinearDepthTex[(uint2)input.Pos.xy] - input.LinearZ));
    TextureColor.rgb *= TextureColor.a;
#ifdef ENABLE_PARTICLE_RESPONSIVE_MASK
    MRT mrt;
    mrt.Color = TextureColor * input.Color;
    float3 responsiveColor = ceil(mrt.Color.rgb);
    mrt.ResponsiveMask = max(responsiveColor.r, max(responsiveColor.g, responsiveColor.b));
    return mrt;
#else
    return TextureColor * input.Color;
#endif
}
