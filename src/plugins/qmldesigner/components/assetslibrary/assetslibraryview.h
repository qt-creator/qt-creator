// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "createtexture.h"

#include <QPointer>

#include <mutex>

namespace QmlDesigner {

class AssetsLibraryWidget;
class AsynchronousImageCache;

class AssetsLibraryView : public AbstractView
{
    Q_OBJECT

public:
    AssetsLibraryView(ExternalDependenciesInterface &externalDependencies);
    ~AssetsLibraryView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void setResourcePath(const QString &resourcePath);
    void active3DSceneChanged(qint32 sceneId) override;

private:
    class ImageCacheData;
    ImageCacheData *imageCacheData();

    std::once_flag imageCacheFlag;
    std::unique_ptr<ImageCacheData> m_imageCacheData;
    QPointer<AssetsLibraryWidget> m_widget;
    QString m_lastResourcePath;
    CreateTextures m_createTextures;
    qint32 m_sceneId = -1;
};

}
