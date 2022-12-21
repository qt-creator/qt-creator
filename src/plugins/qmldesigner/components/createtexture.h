// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>

namespace QmlDesigner {

class AbstractView;

enum class AddTextureMode { Image, Texture, LightProbe };

class CreateTexture
{
public:
    CreateTexture(AbstractView *view, bool importFiles = false);
    void execute(const QString &filePath, AddTextureMode mode, int sceneId);
    ModelNode resolveSceneEnv(int sceneId);
    void assignTextureAsLightProbe(const ModelNode &texture, int sceneId);

private:
    bool addFileToProject(const QString &filePath);
    ModelNode createTextureFromImage(const QString &assetPath, AddTextureMode mode);

private:
    AbstractView *m_view = nullptr;
    bool m_importFile = false;
};

class CreateTextures : public CreateTexture
{
public:
    using CreateTexture::CreateTexture;
    void execute(const QStringList &filePaths, AddTextureMode mode, int sceneId)
    {
        for (const QString &path : filePaths)
            CreateTexture::execute(path, mode, sceneId);
    }
};

}
