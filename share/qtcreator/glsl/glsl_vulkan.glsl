// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Vulkan additions

// Built-in constants.
in int gl_VertexIndex;
in int gl_InstanceIndex;

const highp int gl_MaxInputAttachments = 1;

// subpass input functions.
vec4 subpassLoad(subpassInput subpass);
vec4 subpassLoad(subpassInputMS subpass, int _sample);
ivec4 subpassLoad(isubpassInput subpass);
ivec4 subpassLoad(isubpassInputMS subpass, int _sample);
uvec4 subpassLoad(usubpassInput subpass);
uvec4 subpassLoad(usubpassInputMS subpass, int _sample);

