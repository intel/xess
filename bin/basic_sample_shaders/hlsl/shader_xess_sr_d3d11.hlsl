//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// Copyright (C) 2024 Intel Corporation
//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

cbuffer SceneConstantBuffer : register(b0)
{
    float4 offset;
    float4 velocity;
    float4 resolution;
    float4 padding[13];
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct PSOutputColor
{
    float4 Color: SV_Target0;
};

PSInput VSMainColor(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = float4(position.xyz, 1.);
    result.position.xy += offset.xy + 2. * (offset.zw / resolution.xy);
    result.color = color;

    return result;
}

PSOutputColor PSMainColor(PSInput input) : SV_TARGET
{
    PSOutputColor output;
    output.Color = input.color;

    return output;
}

PSInput VSMainVelocity(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.position.xy += offset.xy + 2. * (offset.zw / resolution.xy );
    result.color = color;

    return result;
}

struct PSOutputVelocity
{
    float2 Velocity: SV_Target0;
};

PSOutputVelocity PSMainVelocity(PSInput input) : SV_TARGET
{
    PSOutputVelocity output;
    output.Velocity = velocity.xy;

    return output;
}

struct PSInputFSQ
{
    float4 position: SV_POSITION;
    float2 texcoord: TEXCOORD;
};

Texture2D input_color: register(t0);
SamplerState samp: register(s0);

PSInputFSQ VSMainFSQ(uint vertex_id: SV_VERTEXID)
{
    PSInputFSQ output;
    output.texcoord = float2(vertex_id & 2, (vertex_id << 1) & 2);
    output.position = float4(output.texcoord * 2.0 - 1.0, 0.0, 1.0);
    output.texcoord.y = 1.0f - output.texcoord.y;

    return output;
}

float4 PSMainFSQ(PSInputFSQ input) : SV_TARGET
{
    return input_color.Sample(samp, input.texcoord);
}

