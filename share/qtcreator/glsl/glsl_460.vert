// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Vertex shader special variables.
vec4  gl_Position;
float gl_PointSize;
vec4  gl_ClipVertex;

struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    float gl_CullDistance[];
};
uniform gl_PerVertex[] gl_in;

int gl_DrawId;
int gl_BaseVertex;
int gl_BaseInstance;

int gl_PrimitiveIDIn;
int gl_PrimitiveID;

// Tesselation control shader.
in gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    float gl_CullDistance[];
} gl_in[gl_MaxPatchVertices];

in int   gl_PatchVerticesIn;
in int   gl_InvocationID;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    float gl_CullDistance[];
} gl_out[];

patch out float gl_TesselLevelOuter[4];
patch out float gl_TesselLevelInner[2];


// Tesselation evaluation shader.

in vec3 gl_TessCoord;

// Geometry shader.

out int gl_Layer;
out int gl_ViewPortIndex;

// Compatibility profile.
struct gl_PerVertex {
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
    float gl_CullDistance[];
    vec4  gl_ClipVertex;
    vec4 gl_FrontColor;
    vec4 gl_BackColor;
    vec4 gl_FrontSecondaryColor;
    vec4 gl_BackSecondaryColor;
    vec4 gl_TexCoord[];
    float gl_FogFragCoord;
};
uniform gl_PerVertex[] gl_in;

in vec4  gl_Color;
in vec4  gl_SecondaryColor;
in vec3  gl_Normal;
in vec4  gl_Vertex;
in vec4  gl_MultiTexCoord0;
in vec4  gl_MultiTexCoord1;
in vec4  gl_MultiTexCoord2;
in vec4  gl_MultiTexCoord3;
in vec4  gl_MultiTexCoord4;
in vec4  gl_MultiTexCoord5;
in vec4  gl_MultiTexCoord6;
in vec4  gl_MultiTexCoord7;
in float gl_FogCoord;

// Geometry shader functions.

void EmitStreamVertex(int stream);
void EndStreamPrimitive(int stream);
void EmitVertex();
void EndPrimitive();


//// Vulkan removed variables and functions


in int   gl_VertexID;
in int   gl_InstanceID;

