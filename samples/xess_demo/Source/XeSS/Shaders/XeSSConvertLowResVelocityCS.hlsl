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
#include "../../../MiniEngine/Core/Shaders/PixelPacking_Velocity.hlsli"

Texture2D<packed_velocity_t> InVelocityBuffer : register(t0);
RWTexture2D<float2> OutVelocityBuffer : register(u0);

cbuffer CB1 : register(b1)
{
    float2 InBufferDim;
    float2 RcpInBufferDim;
}

[RootSignature(Common_RootSig)]
[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 Gid : SV_GroupID)
{
	uint2 st = DTid.xy;

	packed_velocity_t velocityPacked = InVelocityBuffer[st];
	float2 velocity = UnpackVelocity(velocityPacked).xy;

#ifdef _XESS_NDC_VELOCITY_  // here we demo NDC space velocity
    velocity = float2(velocity.x * 2.0 * RcpInBufferDim.x, -velocity.y * 2.0 * RcpInBufferDim.y);
#endif

	OutVelocityBuffer[st] = velocity;
}
