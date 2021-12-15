/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qmldesignerprojectmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <projectstorage/filestatuscache.h>
#include <projectstorage/filesystem.h>
#include <projectstorage/nonlockingmutex.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstorageupdater.h>
#include <projectstorage/qmldocumentparser.h>
#include <projectstorage/qmltypesparser.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <asynchronousexplicitimagecache.h>
#include <imagecache/asynchronousimagefactory.h>
#include <imagecache/explicitimagecacheimageprovider.h>
#include <imagecache/imagecachecollector.h>
#include <imagecache/imagecacheconnectionmanager.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampproviderinterface.h>

#include <coreplugin/icore.h>

#include <QQmlEngine>

namespace QmlDesigner {

namespace {

QString defaultImagePath()
{
    return Core::ICore::resourcePath("qmldesigner/welcomepage/images/newThumbnail.png").toString();
}

::QmlProjectManager::QmlBuildSystem *getQmlBuildSystem(::ProjectExplorer::Target *target)
{
    return qobject_cast<::QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
}

class TimeStampProvider : public TimeStampProviderInterface
{
public:
    Sqlite::TimeStamp timeStamp(Utils::SmallStringView) const override
    {
        auto now = std::chrono::steady_clock::now();

        return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    }

    Sqlite::TimeStamp pause() const override
    {
        using namespace std::chrono_literals;
        return std::chrono::duration_cast<std::chrono::seconds>(1h).count();
    }
};

} // namespace

class PreviewImageCacheData
{
public:
    Sqlite::Database database{Utils::PathString{
                                  Core::ICore::cacheResourcePath("previewcache.db").toString()},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheConnectionManager connectionManager;
    ImageCacheCollector collector{connectionManager,
                                  ImageCacheCollectorNullImageHandling::DontCaptureNullImage};
    TimeStampProvider timeStampProvider;
    AsynchronousExplicitImageCache cache{storage};
    AsynchronousImageFactory factory{storage, timeStampProvider, collector};
};

class ProjectStorageData
{
public:
    ProjectStorageData(::ProjectExplorer::Project *project)
        : database{project->projectDirectory().pathAppended("projectstorage.db").toString()}
    {}

    Sqlite::Database database;
    ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    ProjectStorageUpdater::PathCache pathCache{storage};
    FileSystem fileSystem{pathCache};
    FileStatusCache fileStatusCache{fileSystem};
    QmlDocumentParser qmlDocumentParser{storage};
    QmlTypesParser qmlTypesParser{pathCache, storage};
    ProjectStorageUpdater updater{
        fileSystem, storage, fileStatusCache, pathCache, qmlDocumentParser, qmlTypesParser};
};

class QmlDesignerProjectManagerProjectData
{
public:
    QmlDesignerProjectManagerProjectData(ImageCacheStorage<Sqlite::Database> &storage,
                                         ::ProjectExplorer::Project *project)
        : factory{storage, timeStampProvider, collector}
        , projectStorageData{project}
    {}

    ImageCacheConnectionManager connectionManager;
    ImageCacheCollector collector{connectionManager,
                                  ImageCacheCollectorNullImageHandling::DontCaptureNullImage};
    TimeStampProvider timeStampProvider;
    AsynchronousImageFactory factory;
    ProjectStorageData projectStorageData;
    ::ProjectExplorer::Target *activeTarget = nullptr;
};

QmlDesignerProjectManager::QmlDesignerProjectManager()
    : m_imageCacheData{std::make_unique<PreviewImageCacheData>()}
{
    auto editorManager = ::Core::EditorManager::instance();
    QObject::connect(editorManager, &::Core::EditorManager::editorOpened, [&](auto *editor) {
        editorOpened(editor);
    });
    QObject::connect(editorManager, &::Core::EditorManager::currentEditorChanged, [&](auto *editor) {
        currentEditorChanged(editor);
    });
    QObject::connect(editorManager, &::Core::EditorManager::editorsClosed, [&](const auto &editors) {
        editorsClosed(editors);
    });
    auto sessionManager = ::ProjectExplorer::SessionManager::instance();
    QObject::connect(sessionManager,
                     &::ProjectExplorer::SessionManager::projectAdded,
                     [&](auto *project) { projectAdded(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::SessionManager::aboutToRemoveProject,
                     [&](auto *project) { aboutToRemoveProject(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::SessionManager::projectRemoved,
                     [&](auto *project) { projectRemoved(project); });
}

QmlDesignerProjectManager::~QmlDesignerProjectManager() = default;

void QmlDesignerProjectManager::registerPreviewImageProvider(QQmlEngine *engine) const
{
    auto imageProvider = std::make_unique<ExplicitImageCacheImageProvider>(m_imageCacheData->cache,
                                                                           QImage{defaultImagePath()});

    engine->addImageProvider("project_preview", imageProvider.release());
}

void QmlDesignerProjectManager::editorOpened(::Core::IEditor *) {}

void QmlDesignerProjectManager::currentEditorChanged(::Core::IEditor *)
{
    if (!m_projectData || !m_projectData->activeTarget)
        return;

    ::QmlProjectManager::QmlBuildSystem *qmlBuildSystem = getQmlBuildSystem(
        m_projectData->activeTarget);

    if (qmlBuildSystem) {
        m_imageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_imageCacheData->factory.generate(qmlBuildSystem->mainFilePath().toString().toUtf8());
    }
}

void QmlDesignerProjectManager::editorsClosed(const QList<::Core::IEditor *> &) {}

void QmlDesignerProjectManager::projectAdded(::ProjectExplorer::Project *project)
{
    m_projectData = std::make_unique<QmlDesignerProjectManagerProjectData>(m_imageCacheData->storage,
                                                                           project);
    m_projectData->activeTarget = project->activeTarget();
}

void QmlDesignerProjectManager::aboutToRemoveProject(::ProjectExplorer::Project *)
{
    if (m_projectData) {
        m_imageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_projectData.reset();
    }
}

void QmlDesignerProjectManager::projectRemoved(::ProjectExplorer::Project *) {}

} // namespace QmlDesigner
