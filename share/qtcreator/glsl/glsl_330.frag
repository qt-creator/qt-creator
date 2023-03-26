// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Fragment shader special variables.
vec4  gl_FragCoord;
bool  gl_FrontFacing;
float gl_ClipDistance[];
vec4  gl_FragColor;
vec4  gl_FragData[gl_MaxDrawBuffers];
float gl_FragDepth;

// Varying variables.
varying vec4  gl_Color;
varying vec2  gl_PointCoord;
varying int   gl_PrimitiveID;

// Fragment processing functions.
float dFdx(float p);
vec2 dFdx(vec2 p);
vec3 dFdx(vec3 p);
vec4 dFdx(vec4 p);
float dFdy(float p);
vec2 dFdy(vec2 p);
vec3 dFdy(vec3 p);
vec4 dFdy(vec4 p);
float fwidth(float p);
vec2 fwidth(vec2 p);
vec3 fwidth(vec3 p);
vec4 fwidth(vec4 p);
