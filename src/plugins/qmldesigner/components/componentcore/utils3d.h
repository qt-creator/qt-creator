// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

// please put here 3d related functions which have no clear place elsewhere

#include <abstractview.h>

namespace QmlDesigner {

class NodeMetaInfo;

namespace Utils3D {

inline constexpr AuxiliaryDataKeyView active3dSceneProperty{AuxiliaryDataType::Temporary,
                                                            "active3dScene"};
inline constexpr AuxiliaryDataKeyView matLibSelectedMaterialProperty{AuxiliaryDataType::Temporary,
                                                                     "matLibSelMat"};
inline constexpr AuxiliaryDataKeyView matLibSelectedTextureProperty{AuxiliaryDataType::Temporary,
                                                                    "matLibSelTex"};

ModelNode active3DSceneNode(AbstractView *view);
qint32 active3DSceneId(Model *model);

ModelNode materialLibraryNode(AbstractView *view);
void ensureMaterialLibraryNode(AbstractView *view);
bool isPartOfMaterialLibrary(const ModelNode &node);
ModelNode getTextureDefaultInstance(const QString &source, AbstractView *view);

ModelNode activeView3dNode(AbstractView *view);
QString activeView3dId(AbstractView *view);

ModelNode getMaterialOfModel(const ModelNode &model, int idx = 0);

// These methods handle selection of material library items for various material library views.
// This is separate selection from the normal selection handling.
void selectMaterial(const ModelNode &material);
void selectTexture(const ModelNode &texture);
ModelNode selectedMaterial(AbstractView *view);
ModelNode selectedTexture(AbstractView *view);

QList<ModelNode> getSelectedModels(AbstractView *view);
void applyMaterialToModels(AbstractView *view, const ModelNode &material,
                           const QList<ModelNode> &models, bool add = false);

#ifdef QDS_USE_PROJECTSTORAGE
ModelNode createMaterial(AbstractView *view, const TypeName &typeName);
#else
ModelNode createMaterial(AbstractView *view, const NodeMetaInfo &metaInfo);
#endif

} // namespace Utils3D
} // namespace QmlDesigner
