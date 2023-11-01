// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignerprojectmanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectstorage/filestatuscache.h>
#include <projectstorage/filesystem.h>
#include <projectstorage/nonlockingmutex.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragepathwatcher.h>
#include <projectstorage/projectstorageupdater.h>
#include <projectstorage/qmldocumentparser.h>
#include <projectstorage/qmltypesparser.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <asynchronousexplicitimagecache.h>
#include <asynchronousimagecache.h>
#include <imagecache/asynchronousimagefactory.h>
#include <imagecache/explicitimagecacheimageprovider.h>
#include <imagecache/imagecachecollector.h>
#include <imagecache/imagecacheconnectionmanager.h>
#include <imagecache/imagecachedispatchcollector.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/meshimagecachecollector.h>
#include <imagecache/textureimagecachecollector.h>
#include <imagecache/timestampprovider.h>

#include <utils/asset.h>

#include <coreplugin/icore.h>

#include <QDirIterator>
#include <QFileSystemWatcher>
#include <QQmlEngine>

namespace QmlDesigner {

namespace {

ProjectExplorer::Target *activeTarget(ProjectExplorer::Project *project)
{
    if (project)
        return project->activeTarget();

    return {};
}

QString previewDefaultImagePath()
{
    return Core::ICore::resourcePath("qmldesigner/welcomepage/images/newThumbnail.png").toString();
}

QString previewBrokenImagePath()
{
    return Core::ICore::resourcePath("qmldesigner/welcomepage/images/noPreview.png").toString();
}

::QmlProjectManager::QmlBuildSystem *getQmlBuildSystem(::ProjectExplorer::Target *target)
{
    return qobject_cast<::QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
}

class PreviewTimeStampProvider : public TimeStampProviderInterface
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

auto makeCollectorDispatcherChain(ImageCacheCollector &nodeInstanceCollector,
                                  MeshImageCacheCollector &meshImageCollector,
                                  TextureImageCacheCollector &textureImageCollector)
{
    return std::make_tuple(
        std::make_pair([](Utils::SmallStringView filePath,
                          [[maybe_unused]] Utils::SmallStringView state,
                          [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData
                              &auxiliaryData) { return filePath.endsWith(".qml"); },
                       &nodeInstanceCollector),
        std::make_pair(
            [](Utils::SmallStringView filePath,
               [[maybe_unused]] Utils::SmallStringView state,
               [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData) {
                return filePath.endsWith(".mesh") || filePath.startsWith("#");
            },
            &meshImageCollector),
        std::make_pair(
            [](Utils::SmallStringView filePath,
               [[maybe_unused]] Utils::SmallStringView state,
               [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData) {
                return Asset{QString(filePath)}.isValidTextureSource();
            },
            &textureImageCollector));
        }
} // namespace

class QmlDesignerProjectManager::ImageCacheData
{
public:
    ImageCacheData(ExternalDependenciesInterface &externalDependencies)
        : meshImageCollector{connectionManager, QSize{300, 300}, QSize{600, 600}, externalDependencies}
        , nodeInstanceCollector{connectionManager, QSize{300, 300}, QSize{600, 600}, externalDependencies}
    {}

public:
    Sqlite::Database database{Utils::PathString{
                                  Core::ICore::cacheResourcePath("imagecache-v2.db").toString()},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheConnectionManager connectionManager;
    MeshImageCacheCollector meshImageCollector;
    TextureImageCacheCollector textureImageCollector;
    ImageCacheCollector nodeInstanceCollector;
    ImageCacheDispatchCollector<decltype(makeCollectorDispatcherChain(nodeInstanceCollector,
                                                                      meshImageCollector,
                                                                      textureImageCollector))>
        dispatchCollector{makeCollectorDispatcherChain(nodeInstanceCollector,
                                                       meshImageCollector,
                                                       textureImageCollector)};
    ImageCacheGenerator generator{dispatchCollector, storage};
    TimeStampProvider timeStampProvider;
    AsynchronousImageCache asynchronousImageCache{storage, generator, timeStampProvider};
};

class QmlDesignerProjectManager::PreviewImageCacheData
{
public:
    PreviewImageCacheData(ExternalDependenciesInterface &externalDependencies)
        : collector{connectionManager,
                    QSize{300, 300},
                    QSize{1000, 1000},
                    externalDependencies,
                    ImageCacheCollectorNullImageHandling::CaptureNullImage}
    {
        timer.setSingleShot(true);
    }

public:
    Sqlite::Database database{Utils::PathString{
                                  Core::ICore::cacheResourcePath("previewcache.db").toString()},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheConnectionManager connectionManager;
    ImageCacheCollector collector;
    PreviewTimeStampProvider timeStampProvider;
    AsynchronousExplicitImageCache cache{storage};
    AsynchronousImageFactory factory{storage, timeStampProvider, collector};
    QTimer timer;
};

namespace {
class ProjectStorageData
{
public:
    ProjectStorageData(::ProjectExplorer::Project *project)
        : database{project->projectDirectory().pathAppended("projectstorage.db").toString()}
        , projectPartId{ProjectPartId::create(
              pathCache.sourceId(SourcePath{project->projectDirectory().toString() + "/."}).internalId())}
    {}
    Sqlite::Database database;
    ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    PathCacheType pathCache{storage};
    FileSystem fileSystem{pathCache};
    FileStatusCache fileStatusCache{fileSystem};
    QmlDocumentParser qmlDocumentParser{storage, pathCache};
    QmlTypesParser qmlTypesParser{storage};
    ProjectStoragePathWatcher<QFileSystemWatcher, QTimer, ProjectStorageUpdater::PathCache>
        pathWatcher{pathCache, fileSystem, &updater};
    ProjectPartId projectPartId;
    ProjectStorageUpdater updater{fileSystem,
                                  storage,
                                  fileStatusCache,
                                  pathCache,
                                  qmlDocumentParser,
                                  qmlTypesParser,
                                  pathWatcher,
                                  projectPartId};
};

std::unique_ptr<ProjectStorageData> createProjectStorageData(::ProjectExplorer::Project *project)
{
    if constexpr (useProjectStorage()) {
        return std::make_unique<ProjectStorageData>(project);
    } else {
        return {};
    }
}
} // namespace

class QmlDesignerProjectManager::QmlDesignerProjectManagerProjectData
{
public:
    QmlDesignerProjectManagerProjectData(ImageCacheStorage<Sqlite::Database> &storage,
                                         ::ProjectExplorer::Project *project,
                                         ExternalDependenciesInterface &externalDependencies)
        : collector{connectionManager,
                    QSize{300, 300},
                    QSize{1000, 1000},
                    externalDependencies,
                    ImageCacheCollectorNullImageHandling::CaptureNullImage}
        , factory{storage, timeStampProvider, collector}
        , projectStorageData{createProjectStorageData(project)}
    {}

    ImageCacheConnectionManager connectionManager;
    ImageCacheCollector collector;
    PreviewTimeStampProvider timeStampProvider;
    AsynchronousImageFactory factory;
    std::unique_ptr<ProjectStorageData> projectStorageData;
    QPointer<::ProjectExplorer::Target> activeTarget;
};

QmlDesignerProjectManager::QmlDesignerProjectManager(ExternalDependenciesInterface &externalDependencies)
    : m_previewImageCacheData{std::make_unique<PreviewImageCacheData>(externalDependencies)}
    , m_externalDependencies{externalDependencies}
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
    auto sessionManager = ::ProjectExplorer::ProjectManager::instance();
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::projectAdded,
                     [&](auto *project) { projectAdded(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::aboutToRemoveProject,
                     [&](auto *project) { aboutToRemoveProject(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::projectRemoved,
                     [&](auto *project) { projectRemoved(project); });

    QObject::connect(&m_previewImageCacheData->timer,
                     &QTimer::timeout,
                     this,
                     &QmlDesignerProjectManager::generatePreview);
}

QmlDesignerProjectManager::~QmlDesignerProjectManager() = default;

void QmlDesignerProjectManager::registerPreviewImageProvider(QQmlEngine *engine) const
{
    auto imageProvider = std::make_unique<ExplicitImageCacheImageProvider>(
        m_previewImageCacheData->cache,
        QImage{previewDefaultImagePath()},
        QImage{previewBrokenImagePath()});

    engine->addImageProvider("project_preview", imageProvider.release());
}

AsynchronousImageCache &QmlDesignerProjectManager::asynchronousImageCache()
{
    return imageCacheData()->asynchronousImageCache;
}

namespace {
ProjectStorage<Sqlite::Database> *dummyProjectStorage()
{
    return nullptr;
}

ProjectStorageUpdater::PathCache *dummyPathCache()
{
    return nullptr;
}

} // namespace

ProjectStorageDependencies QmlDesignerProjectManager::projectStorageDependencies()
{
    if constexpr (useProjectStorage()) {
        return {m_projectData->projectStorageData->storage,
                m_projectData->projectStorageData->pathCache};
    } else {
        return {*dummyProjectStorage(), *dummyPathCache()};
    }
}

void QmlDesignerProjectManager::editorOpened(::Core::IEditor *) {}

void QmlDesignerProjectManager::currentEditorChanged(::Core::IEditor *)
{
    using namespace std::chrono_literals;
    m_previewImageCacheData->timer.start(10s);
}

void QmlDesignerProjectManager::editorsClosed(const QList<::Core::IEditor *> &) {}

namespace {

QtSupport::QtVersion *getQtVersion(::ProjectExplorer::Target *target)
{
    if (target)
        return QtSupport::QtKitAspect::qtVersion(target->kit());

    return {};
}

[[maybe_unused]] QtSupport::QtVersion *getQtVersion(::ProjectExplorer::Project *project)
{
    return getQtVersion(project->activeTarget());
}

Utils::FilePath qmlPath(::ProjectExplorer::Target *target)
{
    auto qt = QtSupport::QtKitAspect::qtVersion(target->kit());
    if (qt)
        return qt->qmlPath();

    return {};
}

template<typename... Path>
bool skipDirectoriesWith(const QStringView directoryPath, const Path &...paths)
{
    return (directoryPath.contains(paths) || ...);
}

template<typename... Path>
bool skipDirectoriesEndsWith(const QStringView directoryPath, const Path &...paths)
{
    return (directoryPath.endsWith(paths) || ...);
}

bool skipPath(const QString &directoryPath)
{
    return skipDirectoriesWith(directoryPath,
                               u"QtApplicationManager",
                               u"QtInterfaceFramework",
                               u"QtOpcUa",
                               u"Qt3D",
                               u"Scene2D",
                               u"Scene3D",
                               u"QtWayland",
                               u"Qt5Compat",
                               u"QtCharts",
                               u"QtLocation",
                               u"QtPositioning",
                               u"MaterialEditor",
                               u"QtTextToSpeech",
                               u"QtWebEngine",
                               u"Qt/labs",
                               u"QtDataVisualization")
           || skipDirectoriesEndsWith(directoryPath, u"designer");
}

void collectQmldirPaths(const QString &path, QStringList &qmldirPaths)
{
    QDirIterator dirIterator{path, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories};

    while (dirIterator.hasNext()) {
        auto directoryPath = dirIterator.next();

        QString qmldirPath = directoryPath + "/qmldir";
        if (!skipPath(directoryPath) && QFileInfo::exists(qmldirPath))
            qmldirPaths.push_back(directoryPath);
    }
}

void projectQmldirPaths(::ProjectExplorer::Target *target, QStringList &qmldirPaths)
{
    ::QmlProjectManager::QmlBuildSystem *buildSystem = getQmlBuildSystem(target);

    const Utils::FilePath projectDirectoryPath = buildSystem->canonicalProjectDir();
    const QStringList importPaths = buildSystem->importPaths();
    const QDir projectDirectory(projectDirectoryPath.toString());

    for (const QString &importPath : importPaths)
        collectQmldirPaths(importPath, qmldirPaths);
}

void qtQmldirPaths(::ProjectExplorer::Target *target, QStringList &qmldirPaths)
{
    if constexpr (useProjectStorage())
        collectQmldirPaths(qmlPath(target).toString(), qmldirPaths);
}

QStringList directories(::ProjectExplorer::Target *target)
{
    if (!target)
        return {};

    QStringList qmldirPaths;
    qmldirPaths.reserve(100);

    qtQmldirPaths(target, qmldirPaths);
    projectQmldirPaths(target, qmldirPaths);

    std::sort(qmldirPaths.begin(), qmldirPaths.end());
    qmldirPaths.erase(std::unique(qmldirPaths.begin(), qmldirPaths.end()), qmldirPaths.end());

    return qmldirPaths;
}

QStringList qmlTypes(::ProjectExplorer::Target *target)
{
    if (!target)
        return {};

    QStringList qmldirPaths;
    qmldirPaths.reserve(2);

    const QString installDirectory = qmlPath(target).toString();

    qmldirPaths.append(installDirectory + "/builtins.qmltypes");
    qmldirPaths.append(installDirectory + "/jsroot.qmltypes");

    qmldirPaths.append(
        Core::ICore::resourcePath("qmldesigner/projectstorage/fake.qmltypes").toString());

    return qmldirPaths;
}

QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE")) {
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
    }
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

} // namespace

void QmlDesignerProjectManager::projectAdded(::ProjectExplorer::Project *project)
{
    m_projectData = std::make_unique<QmlDesignerProjectManagerProjectData>(m_previewImageCacheData->storage,
                                                                           project,
                                                                           m_externalDependencies);
    m_projectData->activeTarget = project->activeTarget();

    QObject::connect(project, &::ProjectExplorer::Project::fileListChanged, [&]() {
        fileListChanged();
    });

    QObject::connect(project, &::ProjectExplorer::Project::activeTargetChanged, [&](auto *target) {
        activeTargetChanged(target);
    });

    QObject::connect(project, &::ProjectExplorer::Project::aboutToRemoveTarget, [&](auto *target) {
        aboutToRemoveTarget(target);
    });

    if (auto target = project->activeTarget(); target)
        activeTargetChanged(target);
}

void QmlDesignerProjectManager::aboutToRemoveProject(::ProjectExplorer::Project *)
{
    if (m_projectData) {
        m_previewImageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_projectData.reset();
    }
}

void QmlDesignerProjectManager::projectRemoved(::ProjectExplorer::Project *) {}

void QmlDesignerProjectManager::generatePreview()
{
    if (!m_projectData || !m_projectData->activeTarget)
        return;

    ::QmlProjectManager::QmlBuildSystem *qmlBuildSystem = getQmlBuildSystem(
        m_projectData->activeTarget);

    if (qmlBuildSystem) {
        m_previewImageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_previewImageCacheData->factory.generate(qmlBuildSystem->mainFilePath().toString().toUtf8());
    }
}

QmlDesignerProjectManager::ImageCacheData *QmlDesignerProjectManager::imageCacheData()
{
    std::call_once(imageCacheFlag, [this]() {
        m_imageCacheData = std::make_unique<ImageCacheData>(m_externalDependencies);
        auto setTargetInImageCache =
            [imageCacheData = m_imageCacheData.get()](ProjectExplorer::Target *target) {
                if (target == imageCacheData->nodeInstanceCollector.target())
                    return;

                if (target)
                    imageCacheData->asynchronousImageCache.clean();

                // TODO wrap in function in image cache data
                imageCacheData->meshImageCollector.setTarget(target);
                imageCacheData->nodeInstanceCollector.setTarget(target);
            };

        if (auto project = ProjectExplorer::ProjectManager::startupProject(); project) {
            // TODO wrap in function in image cache data
            m_imageCacheData->meshImageCollector.setTarget(project->activeTarget());
            m_imageCacheData->nodeInstanceCollector.setTarget(project->activeTarget());
            QObject::connect(project,
                             &ProjectExplorer::Project::activeTargetChanged,
                             this,
                             setTargetInImageCache);
        }
        QObject::connect(ProjectExplorer::ProjectManager::instance(),
                         &ProjectExplorer::ProjectManager::startupProjectChanged,
                         this,
                         [=](ProjectExplorer::Project *project) {
                             setTargetInImageCache(activeTarget(project));
                         });
    });
    return m_imageCacheData.get();
}

void QmlDesignerProjectManager::fileListChanged()
{
    update();
}

void QmlDesignerProjectManager::activeTargetChanged(ProjectExplorer::Target *target)
{
    if (!m_projectData || !m_projectData->projectStorageData)
        return;

    QObject::disconnect(m_projectData->activeTarget, nullptr, nullptr, nullptr);

    m_projectData->activeTarget = target;

    if (target) {
        QObject::connect(target, &::ProjectExplorer::Target::kitChanged, [&]() { kitChanged(); });
        QObject::connect(getQmlBuildSystem(target),
                         &::QmlProjectManager::QmlBuildSystem::projectChanged,
                         [&]() { projectChanged(); });
    }

    update();
}

void QmlDesignerProjectManager::aboutToRemoveTarget(ProjectExplorer::Target *target)
{
    QObject::disconnect(target, nullptr, nullptr, nullptr);
    QObject::disconnect(getQmlBuildSystem(target), nullptr, nullptr, nullptr);
}

void QmlDesignerProjectManager::kitChanged()
{
    QStringList qmldirPaths;
    qmldirPaths.reserve(100);

    update();
}

void QmlDesignerProjectManager::projectChanged()
{
    update();
}

void QmlDesignerProjectManager::update()
{
    if (!m_projectData || !m_projectData->projectStorageData)
        return;

    m_projectData->projectStorageData->updater.update(directories(m_projectData->activeTarget),
                                                      qmlTypes(m_projectData->activeTarget),
                                                      propertyEditorResourcesPath());
}

} // namespace QmlDesigner
