// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec4 color;
layout (location = 1) in float d;

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor = color * float(d < (2.0 / 3.0) || d > (5.0 / 6.0));
}
