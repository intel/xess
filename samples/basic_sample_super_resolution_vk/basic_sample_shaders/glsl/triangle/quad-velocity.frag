/*
 * Copyright (C) 2016-2024 by Sascha Willems - www.saschawillems.de
 * Copyright (C) 2024 Intel Corporation
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#version 450

layout (location = 0) in vec2 texCoord;
layout (location = 0) out vec4 outFragColor;

layout (set = 0, binding = 0) uniform sampler2D t;

void main()
{
  vec2 value = texture(t, texCoord).rg;
  float maxVal = 0.02;   // assumed min/max velocity
  float minVal = -0.02;
  outFragColor = vec4((value - minVal) / (maxVal - minVal), 0.0, 1.0f);
}