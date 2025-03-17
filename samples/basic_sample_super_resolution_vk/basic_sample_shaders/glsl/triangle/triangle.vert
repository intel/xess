/*
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 * Copyright (C) 2024 Intel Corporation
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;

layout (binding = 0) uniform UBO 
{
	vec4 offset;
	vec4 velocity;
	vec4 resolution;
} ubo;

layout (location = 0) out vec4 outColor;

out gl_PerVertex 
{
    vec4 gl_Position;   
};


void main() 
{
	outColor = inColor;
	gl_Position = vec4(inPos.xyz, 1.0);
	gl_Position.xy += ubo.offset.xy + 2. * (ubo.offset.zw / ubo.resolution.xy);
}
