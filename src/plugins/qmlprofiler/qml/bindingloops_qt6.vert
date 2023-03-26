// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec4 vertexCoord;
layout (location = 1) in vec2 postScaleOffset;

layout (std140, binding = 0) uniform Block {
    uniform mat4 matrix;
    uniform vec4 bindingLoopsColor;
} block;

layout (location = 0) out vec4 color;

void main()
{
    gl_Position = block.matrix * vertexCoord;
    gl_Position.x += postScaleOffset.x * 0.005;
    gl_Position.y += postScaleOffset.y * 0.01;
    gl_Position.z -= 0.1;
    gl_Position.w = 1.0;
    color = block.bindingLoopsColor;
}
