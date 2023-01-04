// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
