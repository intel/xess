/*******************************************************************************
 * Copyright 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "../../../MiniEngine/Core/Shaders/CommonRS.hlsli"
#include "../../../MiniEngine/Core/Shaders/ShaderUtility.hlsli"

#define WITH_DILATE

Texture2D<float> InDepthBuffer : register(t1);

SamplerState PointSampler : register(s3);

RWTexture2D<float2> OutVelocityBuffer : register(u0);

cbuffer CB1 : register(b1)
{
	float2 InBufferDim;	
	float2 OutBufferDim;	
	float2 RcpInBufferDim;
	float2 RcpOutBufferDim;	
	float2 ViewportJitter;
	matrix CurToPrevXForm;
}

int2 GetClosestDepth(float2 uv, out float ClosestDepth)
{
#ifdef WITH_DILATE
	float DepthO = InDepthBuffer.SampleLevel(PointSampler, uv, 0).x;
	float DepthA = InDepthBuffer.SampleLevel(PointSampler, uv, 0, int2(-1, -1)).x;
	float DepthB = InDepthBuffer.SampleLevel(PointSampler, uv, 0, int2(1, -1)).x;
	float DepthC = InDepthBuffer.SampleLevel(PointSampler, uv, 0, int2(-1, 1)).x;
	float DepthD = InDepthBuffer.SampleLevel(PointSampler, uv, 0, int2(1, 1)).x;

	ClosestDepth = min(DepthO, min(min(DepthA, DepthB), min(DepthC, DepthD)));

	if (DepthA == ClosestDepth)
		return int2(-1, -1);
	else if (DepthB == ClosestDepth)
		return int2(1, -1);
	else if (DepthC == ClosestDepth)
		return int2(-1, 1);
	else if (DepthD == ClosestDepth)
		return int2(1, 1);

	return int2(0, 0);
#else
	ClosestDepth = InDepthBuffer.SampleLevel(PointSampler, uv, 0).x;
	return int2(0, 0);
#endif
}


[RootSignature(Common_RootSig)]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	uint2 st = DTid.xy;

	float2 uvOut = (float2(st) + 0.5) * RcpOutBufferDim;
	float2 posIn = uvOut * InBufferDim + ViewportJitter;

	float2 pixelPosIn = floor(posIn) + 0.5;
	float2 nearestUVIn = pixelPosIn * RcpInBufferDim;

	float closestDepth = 0.0;
	int2 depthOffset = GetClosestDepth(nearestUVIn, closestDepth);

	float2 curPixel = st + 0.5;
	float4 hPos = float4(curPixel * closestDepth, 1.0, closestDepth);
	float4 prevHPos = mul(CurToPrevXForm, hPos);
	prevHPos.xy /= prevHPos.w;

	float2 velocity = prevHPos.xy - curPixel;

#ifdef _XESS_NDC_VELOCITY_ // here we demo NDC space velocity.
    velocity = float2(velocity.x * 2.0 * RcpOutBufferDim.x, -velocity.y * 2.0 * RcpOutBufferDim.y);
#endif

	OutVelocityBuffer[st] = velocity;
}
