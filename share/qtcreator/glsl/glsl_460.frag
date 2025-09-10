// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Fragment shader special variables.
in vec4  gl_FragCoord;
in bool  gl_FrontFacing;
in float gl_ClipDistance[];
in float gl_CullDistance[];
out float gl_FragDepth;
out int gl_SampleMask[];

// Varying variables.
varying vec4  gl_Color;
varying vec2  gl_PointCoord;
varying int   gl_PrimitiveID;

in int gl_SampleID;
in vec2 gl_SamplePosition;
in int gl_SampleMaskIn[];
in int gl_Layer;
in int gl_ViewportIndex;
in bool gl_HelperInvocation;


// Compute shader.
in uvec3 gl_NumWorkGroups;
const uvec3 gl_WorkGroupSize;

in uvec3 gl_WorkGroupID;
in uvec3 gl_LocalInvocationID;

// derived variables.
in uvec3 gl_GlobalInvocationID;
in uint gl_LocalInvocationIndex;


// Compatibility profile.
in gl_PerFrag {
    in float gl_FogFragCoord;
    in vec4 gl_TexCoord[];
    in vec4 gl_Color;
    in vec4 gl_SecondaryColor;
};

out vec4  gl_FragColor;
out vec4  gl_FragData[gl_MaxDrawBuffers];


// Fragment processing functions.
float dFdx(float p);
vec2 dFdx(vec2 p);
vec3 dFdx(vec3 p);
vec4 dFdx(vec4 p);
float dFdy(float p);
vec2 dFdy(vec2 p);
vec3 dFdy(vec3 p);
vec4 dFdy(vec4 p);
float dFdxFine(float p);
vec2 dFdxFine(vec2 p);
vec3 dFdxFine(vec3 p);
vec4 dFdxFine(vec4 p);
float dFdyFine(float p);
vec2 dFdyFine(vec2 p);
vec3 dFdyFine(vec3 p);
vec4 dFdyFine(vec4 p);
float dFdxCoarse(float p);
vec2 dFdxCoarse(vec2 p);
vec3 dFdxCoarse(vec3 p);
vec4 dFdxCoarse(vec4 p);
float dFdyCoarse(float p);
vec2 dFdyCoarse(vec2 p);
vec3 dFdyCoarse(vec3 p);
vec4 dFdyCoarse(vec4 p);
float fwidth(float p);
vec2 fwidth(vec2 p);
vec3 fwidth(vec3 p);
vec4 fwidth(vec4 p);
float fwidthFine(float p);
vec2 fwidthFine(vec2 p);
vec3 fwidthFine(vec3 p);
vec4 fwidthFine(vec4 p);
float fwidthCoarse(float p);
vec2 fwidthCoarse(vec2 p);
vec3 fwidthCoarse(vec3 p);
vec4 fwidthCoarse(vec4 p);

// Interpolations functions.
float interpolateAtCentroid(float interpolant);
vec2 interpolateAtCentroid(vec2 interpolant);
vec3 interpolateAtCentroid(vec3 interpolant);
vec4 interpolateAtCentroid(vec4 interpolant);
float interpolateAtSample(float interpolant, int _sample);
vec2 interpolateAtSample(vec2 interpolant, int _sample);
vec3 interpolateAtSample(vec3 interpolant, int _sample);
vec4 interpolateAtSample(vec4 interpolant, int _sample);
float interpolateAtOffset(float interpolant, vec2 offset);
vec2 interpolateAtOffset(vec2 interpolant, vec2 offset);
vec3 interpolateAtOffset(vec3 interpolant, vec2 offset);
vec4 interpolateAtOffset(vec4 interpolant, vec2 offset);
