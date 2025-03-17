/*
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 * Copyright (C) 2024 Intel Corporation
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#version 450

layout (location = 0) out vec2 outVelocity;

layout (binding = 0) uniform UBO 
{
	vec4 offset;
	vec4 velocity;
	vec4 resolution;
} ubo;


void main() 
{
  outVelocity = ubo.velocity.xy;
}