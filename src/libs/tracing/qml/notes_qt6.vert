// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec4 vertexCoord;
layout (location = 1) in float distanceFromTop;

layout (std140, binding = 0) uniform Block {
    mat4 matrix;
    vec4 notesColor;
} block;

layout (location = 0) out vec4 color;
layout (location = 1) out float d;

void main()
{
    gl_Position = block.matrix * vertexCoord;
    gl_Position.z -= 0.1;
    gl_Position.w = 1.0;
    color = block.notesColor;
    d = distanceFromTop;
}
