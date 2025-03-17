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
void selectTexture(const ModelNode &texture);
ModelNode selectedTexture(AbstractView *view);

ModelNode resolveSceneEnv(AbstractView *view, int sceneId);

QList<ModelNode> getSelectedModels(AbstractView *view);
QList<ModelNode> getSelectedTextures(AbstractView *view);
QList<ModelNode> getSelectedMaterials(AbstractView *view);
void applyMaterialToModels(AbstractView *view, const ModelNode &material,
                           const QList<ModelNode> &models, bool add = false);

void assignTextureAsLightProbe(AbstractView *view, const ModelNode &texture, int sceneId);

void openNodeInPropertyEditor(const ModelNode &node);

#ifdef QDS_USE_PROJECTSTORAGE
ModelNode createMaterial(AbstractView *view, const TypeName &typeName);
#else
ModelNode createMaterial(AbstractView *view, const NodeMetaInfo &metaInfo);
#endif

ModelNode createMaterial(AbstractView *view);
void renameMaterial(const ModelNode &material, const QString &newName);
void duplicateMaterial(AbstractView *view, const ModelNode &material);

bool addQuick3DImportAndView3D(AbstractView *view, bool suppressWarningDialog = false);
void assignMaterialTo3dModel(AbstractView *view, const ModelNode &modelNode,
                             const ModelNode &materialNode = {});


} // namespace Utils3D
} // namespace QmlDesigner
