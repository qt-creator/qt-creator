/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifdef GL_OES_standard_derivatives
#extension GL_OES_standard_derivatives : enable
// else we probably have fwidth() in core
#endif

varying lowp vec3 edgeColor;
varying lowp vec3 color;
varying lowp vec2 barycentric;

lowp vec4 zero = vec4(0.0);
void main()
{
    lowp vec2 d = fwidth(barycentric) * 5.0;
    lowp vec4 edge_closeness = smoothstep(zero, vec4(d.x, d.y, d.x, d.y),
            vec4(barycentric.x, barycentric.y, 1.0 - barycentric.x, 1.0 - barycentric.y));
    lowp float total = min(min(edge_closeness[0], edge_closeness[1]),
            min(edge_closeness[2], edge_closeness[3]));
    // square to make lines sharper
    total = total > 0.5 ? (1.0 - (1.0 - total) * (1.0 - total) * 2.0) : total * total * 2.0;
    gl_FragColor.rgb = mix(edgeColor, color, total);
    gl_FragColor.a = 1.0;
}
