// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Utils {
class FilePath;
}

namespace QmlDesigner {

class AbstractView;
class ModelNode;

enum class AddTextureMode { Image, Texture, LightProbe };

class CreateTexture : public QObject
{
    Q_OBJECT

public:
    CreateTexture(AbstractView *view);

    ModelNode execute(const QString &filePath, AddTextureMode mode = AddTextureMode::Texture, int sceneId = -1);
    ModelNode resolveSceneEnv(int sceneId);
    void assignTextureAsLightProbe(const ModelNode &texture, int sceneId);

private:
    bool addFileToProject(const QString &filePath);
    ModelNode createTextureFromImage(const Utils::FilePath &assetPath, AddTextureMode mode);

    AbstractView *m_view = nullptr;
};

class CreateTextures : public CreateTexture
{
public:
    using CreateTexture::CreateTexture;
    void execute(const QStringList &filePaths, AddTextureMode mode, int sceneId = -1);
};

} // namespace QmlDesigner
