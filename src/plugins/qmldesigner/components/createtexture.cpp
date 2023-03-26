// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createtexture.h"

#include "abstractview.h"
#include "asset.h"
#include "documentmanager.h"
#include "modelnode.h"
#include "modelnodeoperations.h"
#include "nodelistproperty.h"
#include "nodemetainfo.h"
#include "qmlobjectnode.h"
#include "variantproperty.h"

#include <coreplugin/messagebox.h>

#include <QTimer>

namespace QmlDesigner {

CreateTexture::CreateTexture(AbstractView *view)
    : m_view{view}
{}

ModelNode CreateTexture::execute(const QString &filePath, AddTextureMode mode, int sceneId)
{
    Asset asset(filePath);
    if (!asset.isValidTextureSource())
        return {};

    Utils::FilePath assetPath = Utils::FilePath::fromString(filePath);
    if (!assetPath.isChildOf(DocumentManager::currentResourcePath())) {
        if (!addFileToProject(filePath))
            return {};

        // after importing change assetPath to path in project
        QString assetName = assetPath.fileName();
        assetPath = ModelNodeOperations::getImagesDefaultDirectory().pathAppended(assetName);
    }

    ModelNode texture = createTextureFromImage(assetPath, mode);
    if (!texture.isValid())
        return {};

    if (mode == AddTextureMode::LightProbe && sceneId != -1)
        assignTextureAsLightProbe(texture, sceneId);

    QTimer::singleShot(0, m_view, [this, texture]() {
        if (m_view->model())
            m_view->emitCustomNotification("select_texture", {texture}, {true});
    });

    return texture;
}

bool CreateTexture::addFileToProject(const QString &filePath)
{
    AddFilesResult result = ModelNodeOperations::addImageToProject(
                {filePath}, ModelNodeOperations::getImagesDefaultDirectory().toString(), false);

    if (result.status() == AddFilesResult::Failed) {
        Core::AsynchronousMessageBox::warning(QObject::tr("Failed to Add Texture"),
                                              QObject::tr("Could not add %1 to project.").arg(filePath));
        return false;
    }

    return true;
}

ModelNode CreateTexture::createTextureFromImage(const  Utils::FilePath &assetPath, AddTextureMode mode)
{
    if (mode != AddTextureMode::Texture && mode != AddTextureMode::LightProbe)
        return {};

    ModelNode matLib = m_view->materialLibraryNode();
    if (!matLib.isValid())
        return {};

    NodeMetaInfo metaInfo = m_view->model()->qtQuick3DTextureMetaInfo();

    QString textureSource = assetPath.relativePathFrom(DocumentManager::currentFilePath()).toString();

    ModelNode newTexNode = m_view->getTextureDefaultInstance(textureSource);
    if (!newTexNode.isValid()) {
        newTexNode = m_view->createModelNode("QtQuick3D.Texture",
                                             metaInfo.majorVersion(),
                                             metaInfo.minorVersion());

        newTexNode.setIdWithoutRefactoring(m_view->model()->generateNewId(assetPath.baseName()));

        VariantProperty sourceProp = newTexNode.variantProperty("source");
        sourceProp.setValue(textureSource);
        matLib.defaultNodeListProperty().reparentHere(newTexNode);
    }

    return newTexNode;
}

void CreateTexture::assignTextureAsLightProbe(const ModelNode &texture, int sceneId)
{
    ModelNode sceneEnvNode = resolveSceneEnv(sceneId);
    QmlObjectNode sceneEnv = sceneEnvNode;
    if (sceneEnv.isValid()) {
        sceneEnv.setBindingProperty("lightProbe", texture.id());
        sceneEnv.setVariantProperty("backgroundMode",
                                    QVariant::fromValue(Enumeration("SceneEnvironment",
                                                                    "SkyBox")));
    }
}

ModelNode CreateTexture::resolveSceneEnv(int sceneId)
{
    ModelNode activeSceneEnv;
    ModelNode selectedNode = m_view->firstSelectedModelNode();

    if (selectedNode.metaInfo().isQtQuick3DSceneEnvironment()) {
        activeSceneEnv = selectedNode;
    } else if (sceneId != -1) {
        ModelNode activeScene = m_view->active3DSceneNode();
        if (activeScene.isValid()) {
            QmlObjectNode view3D;
            if (activeScene.metaInfo().isQtQuick3DView3D()) {
                view3D = activeScene;
            } else {
                ModelNode sceneParent = activeScene.parentProperty().parentModelNode();
                if (sceneParent.metaInfo().isQtQuick3DView3D())
                    view3D = sceneParent;
            }
            if (view3D.isValid())
                activeSceneEnv = m_view->modelNodeForId(view3D.expression("environment"));
        }
    }

    return activeSceneEnv;
}

void CreateTextures::execute(const QStringList &filePaths, AddTextureMode mode, int sceneId)
{
    for (const QString &path : filePaths)
        CreateTexture::execute(path, mode, sceneId);
}

} // namespace QmlDesigner
