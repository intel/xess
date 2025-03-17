/*
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 * Copyright (C) 2024 Intel Corporation
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#version 450

layout (location = 0) out vec2 texCoord;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
    texCoord  = vec2(gl_VertexIndex & 2, (gl_VertexIndex << 1) & 2);
	gl_Position = vec4(texCoord * 2.0 - 1.0, 0.0, 1.0);
}
