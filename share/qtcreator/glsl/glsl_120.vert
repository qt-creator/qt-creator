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
