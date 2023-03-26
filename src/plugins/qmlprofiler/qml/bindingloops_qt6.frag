// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#version 440

layout (location = 0) in vec4 color;

layout (location = 0) out vec4 fragColor;

void main()
{
    fragColor.rgb = color.rgb;
    fragColor.a = 1.0;
}
