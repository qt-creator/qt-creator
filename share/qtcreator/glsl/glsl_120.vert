// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Vertex shader special variables.
vec4  gl_Position;
float gl_PointSize;
vec4  gl_ClipVertex;

// Vertex shader built-in attributes.
attribute vec4  gl_Color;
attribute vec4  gl_SecondaryColor;
attribute vec3  gl_Normal;
attribute vec4  gl_Vertex;
attribute vec4  gl_MultiTexCoord0;
attribute vec4  gl_MultiTexCoord1;
attribute vec4  gl_MultiTexCoord2;
attribute vec4  gl_MultiTexCoord3;
attribute vec4  gl_MultiTexCoord4;
attribute vec4  gl_MultiTexCoord5;
attribute vec4  gl_MultiTexCoord6;
attribute vec4  gl_MultiTexCoord7;
attribute float gl_FogCoord;

// Varying variables.
varying vec4  gl_FrontColor;
varying vec4  gl_BackColor;
varying vec4  gl_FrontSecondaryColor;
varying vec4  gl_BackSecondaryColor;
varying vec4  gl_TexCoord[];
varying float gl_FogFragCoord;

// Common functions.
vec4 ftransform();

// Texture level-of-detail functions.
vec4 texture1DLod(sampler1D sampler, float coord, float lod);
vec4 texture1DProjLod(sampler1D sampler, vec2 coord, float lod);
vec4 texture1DProjLod(sampler1D sampler, vec4 coord, float lod);
vec4 texture2DLod(sampler2D sampler, vec2 coord, float lod);
vec4 texture2DProjLod(sampler2D sampler, vec3 coord, float lod);
vec4 texture2DProjLod(sampler2D sampler, vec4 coord, float lod);
vec4 texture3DLod(sampler3D sampler, vec3 coord, float lod);
vec4 texture3DProjLod(sampler3D sampler, vec4 coord, float lod);
vec4 textureCubeLod(samplerCube sampler, vec3 coord, float lod);
vec4 shadow1DLod(sampler1DShadow sampler, vec3 coord, float lod);
vec4 shadow2DLod(sampler2DShadow sampler, vec3 coord, float lod);
vec4 shadow1DProjLod(sampler1DShadow sampler, vec4 coord, float lod);
vec4 shadow2DProjLod(sampler2DShadow sampler, vec4 coord, float lod);
