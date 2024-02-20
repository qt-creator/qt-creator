// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// please put here 3d related functions which have no clear place elsewhere

#include <abstractview.h>

namespace QmlDesigner {
namespace Utils3D {

inline constexpr AuxiliaryDataKeyView active3dSceneProperty{AuxiliaryDataType::Temporary,
                                                            "active3dScene"};

ModelNode active3DSceneNode(AbstractView *view);
qint32 active3DSceneId(Model *model);

ModelNode materialLibraryNode(AbstractView *view);
void ensureMaterialLibraryNode(AbstractView *view);
bool isPartOfMaterialLibrary(const ModelNode &node);
ModelNode getTextureDefaultInstance(const QString &source, AbstractView *view);

} // namespace Utils3D
} // namespace QmlDesigner
