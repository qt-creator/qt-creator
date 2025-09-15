// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelfwd.h"
#include <projectstoragefwd.h>
#include <qmldesigner_global.h>

#include <QList>
#include <QObject>
#include <QTimer>

#include <memory>
#include <mutex>

QT_FORWARD_DECLARE_CLASS(QQmlEngine)

namespace Core {
class IEditor;
} // namespace Core

namespace ProjectExplorer {
class Project;
class Target;
} // namespace ProjectExplorer

namespace QmlDesigner {

class ViewManager;

class QMLDESIGNER_EXPORT QmlDesignerProjectManager
{
    class QmlDesignerProjectManagerProjectData;
    class PreviewImageCacheData;
    class ImageCacheData;
    class Data;

public:
    QmlDesignerProjectManager(ViewManager &viewManager);
    ~QmlDesignerProjectManager();

    void registerPreviewImageProvider(QQmlEngine *engine) const;

    class AsynchronousImageCache &asynchronousImageCache();
    ModulesStorage &modulesStorage();
    ProjectStorageDependencies projectStorageDependencies();

private:
    void editorOpened(::Core::IEditor *editor);
    void currentEditorChanged(::Core::IEditor *);
    void editorsClosed(const QList<Core::IEditor *> &editor);
    void projectAdded(const ::ProjectExplorer::Project *project);
    void aboutToRemoveProject(const ::ProjectExplorer::Project *project);
    void projectRemoved(const ::ProjectExplorer::Project *project);
    void generatePreview();
    ImageCacheData *imageCacheData();

    void activeTargetChanged(::ProjectExplorer::Target *target);
    void aboutToRemoveTarget(::ProjectExplorer::Target *target);
    void kitChanged();
    void projectChanged();

private:
    void update();

private:
    std::unique_ptr<Data> m_data;
    std::once_flag imageCacheFlag;
    std::unique_ptr<ImageCacheData> m_imageCacheData;
    std::unique_ptr<PreviewImageCacheData> m_previewImageCacheData;
    std::unique_ptr<QmlDesignerProjectManagerProjectData> m_projectData;
    QObject dummy;
};
} // namespace QmlDesigner
