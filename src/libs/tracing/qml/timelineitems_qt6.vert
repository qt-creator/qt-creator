// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec4 vertexCoord;
layout (location = 1) in vec2 rectSize;
layout (location = 2) in float selectionId;
layout (location = 3) in vec4 vertexColor;

layout (std140, binding = 0) uniform Block {
    mat4 matrix;
    vec2 scale;
    vec4 selectionColor;
    float selectedItem;
} block;

layout (location = 0) out vec3 color;
layout (location = 1) out vec3 edgeColor;
layout (location = 2) out vec2 barycentric;

void main()
{
    gl_Position = block.matrix * vertexCoord;

    // Make very narrow events somewhat wider so that they don't collapse into 0 pixels
    float scaledWidth = block.scale.x * rectSize.x;
    float shift = sign(rectSize.x) * max(0.0, 3.0 - abs(scaledWidth)) * 0.0005;
    gl_Position.x += shift;

    // Ditto for events with very small height
    float scaledHeight = block.scale.y * rectSize.y;
    gl_Position.y += float(rectSize.y > 0.0) * max(0.0, 3.0 - scaledHeight) * 0.003;

    barycentric = vec2(rectSize.x > 0.0 ? 1.0 : 0.0, rectSize.y > 0.0 ? 1.0 : 0.0);
    color = vertexColor.rgb;
    float selected = min(abs(selectionId - block.selectedItem), 1.0);
    edgeColor.rgb = mix(block.selectionColor.rgb, vertexColor.rgb, selected);
    gl_Position.z += mix(0.0, (shift + 0.0015) / 10.0, selected);
    gl_Position.w = 1.0;
}
