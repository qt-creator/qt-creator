/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
