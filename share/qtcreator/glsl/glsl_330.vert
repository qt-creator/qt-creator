/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

// Vertex shader special variables.
vec4  gl_Position;
float gl_PointSize;
vec4  gl_ClipVertex;

int   gl_VertexID;
int   gl_InstanceID;

struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];

int gl_PrimitiveIDIn;
struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
};

int gl_PrimitiveID;
int gl_Layer;

// Compatibility profile.
struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    vec4  gl_ClipVertex;
};

struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    vec4  gl_ClipVertex;
} gl_in[];

vec4  gl_Color;
vec4  gl_SecondaryColor;
vec3  gl_Normal;
vec4  gl_Vertex;
vec4  gl_MultiTexCoord0;
vec4  gl_MultiTexCoord1;
vec4  gl_MultiTexCoord2;
vec4  gl_MultiTexCoord3;
vec4  gl_MultiTexCoord4;
vec4  gl_MultiTexCoord5;
vec4  gl_MultiTexCoord6;
vec4  gl_MultiTexCoord7;
float gl_FogCoord;
