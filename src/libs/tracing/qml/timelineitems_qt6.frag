// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec3 color;
layout (location = 1) in vec3 edgeColor;
layout (location = 2) in vec2 barycentric;

layout (location = 0) out vec4 fragColor;

void main()
{
    lowp vec2 d = fwidth(barycentric) * 4.0;
    lowp vec4 edge_closeness = step(vec4(d.x, d.y, d.x, d.y),
            vec4(barycentric.x, barycentric.y, 1.0 - barycentric.x, 1.0 - barycentric.y));
    lowp float total = min(min(edge_closeness[0], edge_closeness[1]),
            min(edge_closeness[2], edge_closeness[3]));
    fragColor.rgb = mix(edgeColor, color, total);
    fragColor.a = 1.0;
}
