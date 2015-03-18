/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

attribute vec4 vertexCoord;
attribute vec2 rectSize;
attribute float selectionId;
attribute vec4 vertexColor;

uniform vec2 scale;
uniform mat4 matrix;
uniform vec4 selectionColor;
uniform float selectedItem;

varying vec3 color;
varying vec3 edgeColor;
varying vec2 barycentric;

void main()
{
    gl_Position = matrix * vertexCoord;

    // Make very narrow events somewhat wider so that they don't collapse into 0 pixels
    float scaledWidth = scale.x * rectSize.x;
    float shift = sign(scaledWidth) * max(0.0, 3.0 - abs(scaledWidth)) * 0.0005;
    gl_Position.x += shift;

    // Ditto for events with very small height
    float scaledHeight = scale.y * rectSize.y;
    gl_Position.y += float(rectSize.y > 0.0) * max(0.0, 3.0 - scaledHeight) * 0.003;

    barycentric = vec2(rectSize.x > 0.0 ? 1.0 : 0.0, rectSize.y > 0.0 ? 1.0 : 0.0);
    color = vertexColor.rgb;
    float selected = min(1.0, abs(selectionId - selectedItem));
    edgeColor = mix(selectionColor.rgb, vertexColor.rgb, selected);
    gl_Position.z += mix(0.0, (shift + 0.0015) / 10.0, selected);
    gl_Position.w = 1.0;
}
