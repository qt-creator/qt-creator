// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmldesignerprojectmanager.h"
#include "projectstorageerrornotifier.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <modulesstorage/modulesstorage.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <projectstorage/filestatuscache.h>
#include <projectstorage/filesystem.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstoragepathwatcher.h>
#include <projectstorage/projectstorageupdater.h>
#include <projectstorage/qmldocumentparser.h>
#include <projectstorage/qmltypesparser.h>
#include <qmlprojectmanager/qmlproject.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>
#include <sourcepathstorage/nonlockingmutex.h>
#include <sourcepathstorage/sourcepathcache.h>
#include <sqlitedatabase.h>

#include <asynchronousexplicitimagecache.h>
#include <asynchronousimagecache.h>
#include <imagecache/asynchronousimagefactory.h>
#include <imagecache/explicitimagecacheimageprovider.h>
#include <imagecache/imagecachedispatchcollector.h>
#include <imagecache/imagecachegenerator.h>
#include <imagecache/imagecachestorage.h>
#include <imagecache/timestampprovider.h>
#include <imagecachecollectors/imagecachecollector.h>
#include <imagecachecollectors/meshimagecachecollector.h>
#include <imagecachecollectors/textureimagecachecollector.h>

#include <qmldesignerutils/asset.h>

#include <coreplugin/icore.h>

#include <QDirIterator>
#include <QLibraryInfo>
#include <QQmlEngine>
#include <QStandardPaths>

#include <sourcelocation.h>

using namespace std::chrono;
using namespace std::chrono_literals;

namespace QmlDesigner {

using ProjectManagingTracing::category;

namespace {

ProjectExplorer::Target *activeTarget(ProjectExplorer::Project *project)
{
    if (project)
        return project->activeTarget();

    return {};
}

QString resourcePath(const QString &relativePath)
{
    return Core::ICore::resourcePath(relativePath).cleanPath().path();
}

QString cacheResourcePath(const QString &relativePath)
{
    return Core::ICore::cacheResourcePath(relativePath).cleanPath().path();
}

QString previewDefaultImagePath()
{
    return resourcePath("qmldesigner/welcomepage/images/newThumbnail.png");
}

QString previewBrokenImagePath()
{
    return resourcePath("qmldesigner/welcomepage/images/noPreview.png");
}

::QmlProjectManager::QmlBuildSystem *getQmlBuildSystem(const ::ProjectExplorer::Target *target)
{
    return qobject_cast<::QmlProjectManager::QmlBuildSystem *>(target->buildSystem());
}

class PreviewTimeStampProvider : public TimeStampProviderInterface
{
public:
    Sqlite::TimeStamp timeStamp(Utils::SmallStringView) const override
    {
        return duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();
    }

    Sqlite::TimeStamp pause() const override
    {
        return duration_cast<seconds>(1h).count();
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
                              &auxiliaryData) { return filePath.ends_with(".qml"); },
                       &nodeInstanceCollector),
        std::make_pair(
            [](Utils::SmallStringView filePath,
               [[maybe_unused]] Utils::SmallStringView state,
               [[maybe_unused]] const QmlDesigner::ImageCache::AuxiliaryData &auxiliaryData) {
                return filePath.ends_with(".mesh") || filePath.starts_with("#");
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
    ImageCacheData()
        : meshImageCollector{QSize{300, 300}, QSize{600, 600}}
        , nodeInstanceCollector{QSize{300, 300}, QSize{600, 600}}
    {}

public:
    Sqlite::Database database{Utils::PathString{cacheResourcePath("imagecache-v2.db")},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
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
    PreviewImageCacheData()
        : collector{QSize{300, 300},
                    QSize{1000, 1000},
                    ImageCacheCollectorNullImageHandling::CaptureNullImage}
    {
        timer.setSingleShot(true);
    }

public:
    Sqlite::Database database{Utils::PathString{cacheResourcePath("previewcache.db")},
                              Sqlite::JournalMode::Wal,
                              Sqlite::LockingMode::Normal};
    ImageCacheStorage<Sqlite::Database> storage{database};
    ImageCacheCollector collector;
    PreviewTimeStampProvider timeStampProvider;
    AsynchronousExplicitImageCache cache{storage};
    AsynchronousImageFactory factory{storage, timeStampProvider, collector};
    QTimer timer;
};

namespace {

Sqlite::JournalMode projectStorageJournalMode()
{
    if (qEnvironmentVariableIsEmpty("QDS_STORE_PROJECTSTORAGE_IN_PROJECT"))
        return Sqlite::JournalMode::Memory;

    return Sqlite::JournalMode::Wal;
}

[[maybe_unused]] QString qmlPath(::ProjectExplorer::Target *target)
{
    const QString envPath = QString::fromLocal8Bit(qgetenv("QDS_QML_IMPORT_OVERRIDE")).trimmed();
    if (!envPath.isEmpty()) {
        qDebug() << "Using QDS_QML_IMPORT_OVERRIDE:" << envPath;
        return envPath;
    }

#ifdef USE_QML_IMPORT_OVERRIDE
    const QString internalOverridePath = Core::ICore::resourcePath(
                                             "qmldesigner/qml-import-override")
                                             .path();

    qDebug() << "Using internal QDS_QML_IMPORT_OVERRIDE:" << internalOverridePath;
    return internalOverridePath;
#endif

    if (const auto qt = QtSupport::QtKitAspect::qtVersion(target->kit()))
        return qt->qmlPath().cleanPath().path();

    return QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
}

[[maybe_unused]] QString qmlPath(const ::ProjectExplorer::Project *project)
{
    auto qt = QtSupport::QtKitAspect::qtVersion(project->activeKit());
    if (qt)
        return qt->qmlPath().cleanPath().path();

    return QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
}

class ProjectStorageData
{
public:
    ProjectStorageData(const ::ProjectExplorer::Project *project,
                       PathCacheType &pathCache,
                       ModulesStorage &modulesStorage)
        : database{project->projectDirectory().pathAppended("projectstorage.db").cleanPath().path(),
                   projectStorageJournalMode()}
        , errorNotifier{pathCache, modulesStorage}
        , storage{database, errorNotifier, modulesStorage, database.isInitialized()}
        , fileSystem{pathCache}
        , fileStatusCache(fileSystem)
        , qmlDocumentParser{storage, pathCache}
        , qmlTypesParser{storage}
        // , pathWatcher{pathCache, fileStatusCache, &updater}
        , projectPartId{ProjectPartId::create(
              pathCache
                  .directoryPathId(Utils::PathString{project->projectDirectory().cleanPath().path()})
                  .internalId())}
        , qtPartId{ProjectPartId::create(
              pathCache.directoryPathId(Utils::PathString{qmlPath(project)}).internalId())}
        // , updater{fileSystem,
        //           storage,
        //           fileStatusCache,
        //           pathCache,
        //           modulesStorage,
        //           qmlDocumentParser,
        //           qmlTypesParser,
        //           pathWatcher,
        //           errorNotifier,
        //           projectPartId,
        //           qtPartId}
    {}
    Sqlite::Database database;
    ProjectStorageErrorNotifier errorNotifier;
    ProjectStorage storage;
    FileSystem fileSystem;
    FileStatusCache fileStatusCache;
    QmlDocumentParser qmlDocumentParser;
    QmlTypesParser qmlTypesParser;
    // ProjectStoragePathWatcher<Utils::FileSystemWatcher, QTimer, PathCacheType> pathWatcher;
    ProjectPartId projectPartId;
    ProjectPartId qtPartId;
    // ProjectStorageUpdater updater;
};

std::unique_ptr<ProjectStorageData> createProjectStorageData(const ::ProjectExplorer::Project *project,
                                                             PathCacheType &pathCache,
                                                             ModulesStorage &modulesStorage)
{
    if constexpr (useProjectStorage()) {
        return std::make_unique<ProjectStorageData>(project, pathCache, modulesStorage);
    } else {
        return {};
    }
}

Utils::PathString createDatabasePath(std::string_view name)
{
    auto directory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    QDir{}.mkpath(directory);

    Utils::PathString path = directory;

    path.append('/');
    path.append(name);

    return path;
}

} // namespace

class QmlDesignerProjectManager::QmlDesignerProjectManagerProjectData
{
public:
    QmlDesignerProjectManagerProjectData(ImageCacheStorage<Sqlite::Database> &storage,
                                         const ::ProjectExplorer::Project *project,
                                         PathCacheType &pathCache,
                                         ModulesStorage &modulesStorage)
        : collector{QSize{300, 300},
                    QSize{1000, 1000},
                    ImageCacheCollectorNullImageHandling::CaptureNullImage}
        , factory{storage, timeStampProvider, collector}
        , projectStorageData{createProjectStorageData(project, pathCache, modulesStorage)}
        , activeTarget{project->activeTarget()}
    {}

    ImageCacheCollector collector;
    PreviewTimeStampProvider timeStampProvider;
    AsynchronousImageFactory factory;
    std::unique_ptr<ProjectStorageData> projectStorageData;
    QPointer<::ProjectExplorer::Target> activeTarget;
};

class QmlDesignerProjectManager::Data
{
public:
    Sqlite::Database sourcePathDatabase{createDatabasePath("source_path_v1.db"),
                                        Sqlite::JournalMode::Wal,
                                        Sqlite::LockingMode::Normal};
    SourcePathStorage sourcePathStorage{sourcePathDatabase, sourcePathDatabase.isInitialized()};
    PathCache pathCache{sourcePathStorage};
    Sqlite::Database modulesDatabase{createDatabasePath("modules.db"),
                                     Sqlite::JournalMode::Wal,
                                     Sqlite::LockingMode::Normal};
    ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
};

QmlDesignerProjectManager::QmlDesignerProjectManager()
{
    NanotraceHR::Tracer tracer{"qml designer project manager constructor", category()};

    m_data = std::make_unique<Data>();
    m_previewImageCacheData = std::make_unique<PreviewImageCacheData>();

    auto editorManager = ::Core::EditorManager::instance();
    QObject::connect(editorManager, &::Core::EditorManager::editorOpened, &dummy, [&](auto *editor) {
        editorOpened(editor);
    });
    QObject::connect(editorManager,
                     &::Core::EditorManager::currentEditorChanged,
                     &dummy,
                     [&](auto *editor) { currentEditorChanged(editor); });
    QObject::connect(editorManager,
                     &::Core::EditorManager::editorsClosed,
                     &dummy,
                     [&](const auto &editors) { editorsClosed(editors); });
    auto sessionManager = ::ProjectExplorer::ProjectManager::instance();
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::projectAdded,
                     &dummy,
                     [&](auto *project) { projectAdded(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::aboutToRemoveProject,
                     &dummy,
                     [&](auto *project) { aboutToRemoveProject(project); });
    QObject::connect(sessionManager,
                     &::ProjectExplorer::ProjectManager::projectRemoved,
                     &dummy,
                     [&](auto *project) { projectRemoved(project); });

    QObject::connect(&m_previewImageCacheData->timer, &QTimer::timeout, &dummy, [&]() {
        generatePreview();
    });
}

QmlDesignerProjectManager::~QmlDesignerProjectManager()
{
    NanotraceHR::Tracer tracer{"qml designer project manager destructor", category()};
}

void QmlDesignerProjectManager::registerPreviewImageProvider(QQmlEngine *engine) const
{
    NanotraceHR::Tracer tracer{"qml designer project manager register preview image provider",
                               category()};

    auto imageProvider = std::make_unique<ExplicitImageCacheImageProvider>(
        m_previewImageCacheData->cache,
        QImage{previewDefaultImagePath()},
        QImage{previewBrokenImagePath()});

    engine->addImageProvider("project_preview", imageProvider.release());
}

AsynchronousImageCache &QmlDesignerProjectManager::asynchronousImageCache()
{
    NanotraceHR::Tracer tracer{"qml designer project manager asynchronous image cache", category()};

    return imageCacheData()->asynchronousImageCache;
}

ModulesStorage &QmlDesignerProjectManager::modulesStorage()
{
    NanotraceHR::Tracer tracer{"qml designer project manager modules storage", category()};

    return m_data->modulesStorage;
}

namespace {
[[maybe_unused]] ProjectStorageType *dummyProjectStorage()
{
    return nullptr;
}

[[maybe_unused]] PathCacheType *dummyPathCache()
{
    return nullptr;
}

[[maybe_unused]] ModulesStorage *dummyModulesStorage()
{
    return nullptr;
}

[[maybe_unused]] ProjectStorageTriggerUpdateInterface *dummyTriggerUpdate()
{
    return nullptr;
}

} // namespace

ProjectStorageDependencies QmlDesignerProjectManager::projectStorageDependencies()
{
    NanotraceHR::Tracer tracer{"qml designer project manager project storage dependencies",
                               category()};

    // if constexpr (useProjectStorage()) {
    //     return {m_projectData->projectStorageData->storage,
    //             m_data->pathCache,
    //             m_data->modulesStorage,
    //             m_projectData->projectStorageData->pathWatcher};
    // } else {
        return {*dummyProjectStorage(),
                *dummyPathCache(),
                *dummyModulesStorage(),
                *dummyTriggerUpdate()};
    // }
}

void QmlDesignerProjectManager::editorOpened(::Core::IEditor *)
{
    NanotraceHR::Tracer tracer{"qml designer project manager editor opened", category()};
}

void QmlDesignerProjectManager::currentEditorChanged(::Core::IEditor *)
{
    NanotraceHR::Tracer tracer{"qml designer project manager current editor changed", category()};

    m_previewImageCacheData->timer.start(10s);
}

void QmlDesignerProjectManager::editorsClosed(const QList<::Core::IEditor *> &) {}

namespace {

[[maybe_unused]] QString projectDirectory(::ProjectExplorer::Target *target)
{
    ::QmlProjectManager::QmlBuildSystem *buildSystem = getQmlBuildSystem(target);

    return buildSystem->canonicalProjectDir().cleanPath().path();
}

[[maybe_unused]] void qtQmldirPaths(::ProjectExplorer::Target *target, QStringList &qmldirPaths)
{
    if constexpr (useProjectStorage()) {
        auto qmlRootPath = qmlPath(target);
        qmldirPaths.push_back(qmlRootPath + "/QML");
        qmldirPaths.push_back(qmlRootPath + "/Qt");
        qmldirPaths.push_back(qmlRootPath + "/QtCharts");
        qmldirPaths.push_back(qmlRootPath + "/QtGraphs");
        qmldirPaths.push_back(qmlRootPath + "/QtCore");
        qmldirPaths.push_back(qmlRootPath + "/QtQml");
        qmldirPaths.push_back(qmlRootPath + "/QtQuick");
        qmldirPaths.push_back(qmlRootPath + "/QtQuick3D");
        qmldirPaths.push_back(qmlRootPath + "/Qt5Compat");
        qmldirPaths.push_back(qmlRootPath + "/QtMultimedia");
        qmldirPaths.push_back(qmlRootPath + "/QtQuickUltralite");
    }
}

[[maybe_unused]] void qtQmldirPathsForLiteDesigner(QStringList &qmldirPaths)
{
    if constexpr (useProjectStorage()) {
        auto qmlRootPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
        qmldirPaths.push_back(qmlRootPath + "/QML");
        qmldirPaths.push_back(qmlRootPath + "/QtQml");
        qmldirPaths.push_back(qmlRootPath + "/QtQuick");
    }
}

[[maybe_unused]] QStringList directories(::ProjectExplorer::Target *target)
{
    if (!target)
        return {};

    QStringList qmldirPaths;
    qmldirPaths.reserve(100);

    qtQmldirPaths(target, qmldirPaths);

    return qmldirPaths;
}

[[maybe_unused]] QStringList directoriesForLiteDesigner()
{
    QStringList qmldirPaths;
    qmldirPaths.reserve(100);

    qtQmldirPathsForLiteDesigner(qmldirPaths);

    std::sort(qmldirPaths.begin(), qmldirPaths.end());
    qmldirPaths.erase(std::unique(qmldirPaths.begin(), qmldirPaths.end()), qmldirPaths.end());

    return qmldirPaths;
}

// QString propertyEditorResourcesPath()
// {
// #ifdef SHARE_QML_PATH
//     if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
//         return (Utils::FilePath{SHARE_QML_PATH}.cleanPath() / "propertyEditorQmlSources").path();

// #endif
//     return resourcePath("qmldesigner/propertyEditorQmlSources");
// }

// QString qtCreatorItemLibraryPath()
// {
//     return resourcePath("qmldesigner/itemLibrary");
// }

} // namespace

void QmlDesignerProjectManager::projectAdded(const ::ProjectExplorer::Project *project)
{
    NanotraceHR::Tracer tracer{"qml designer project manager project added", category()};

    m_projectData = std::make_unique<QmlDesignerProjectManagerProjectData>(
        m_previewImageCacheData->storage, project, m_data->pathCache, m_data->modulesStorage);

    QObject::connect(project, &::ProjectExplorer::Project::activeTargetChanged, [&](auto *target) {
        activeTargetChanged(target);
    });

    QObject::connect(project, &::ProjectExplorer::Project::aboutToRemoveTarget, [&](auto *target) {
        aboutToRemoveTarget(target);
    });

    if (auto target = project->activeTarget(); target)
        activeTargetChanged(target);
}

void QmlDesignerProjectManager::aboutToRemoveProject(const ::ProjectExplorer::Project *)
{
    NanotraceHR::Tracer tracer{"qml designer project manager about to remove project", category()};

    if (m_projectData) {
        m_previewImageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_projectData.reset();
    }
}

void QmlDesignerProjectManager::projectRemoved(const ::ProjectExplorer::Project *)
{
    NanotraceHR::Tracer tracer{"qml designer project manager project removed", category()};
}

void QmlDesignerProjectManager::generatePreview()
{
    NanotraceHR::Tracer tracer{"qml designer project manager generate preview", category()};

    if (!m_projectData || !m_projectData->activeTarget)
        return;

    ::QmlProjectManager::QmlBuildSystem *qmlBuildSystem = getQmlBuildSystem(
        m_projectData->activeTarget);

    if (qmlBuildSystem) {
        m_previewImageCacheData->collector.setTarget(m_projectData->activeTarget);
        m_previewImageCacheData->factory.generate(
            qmlBuildSystem->mainFilePath().cleanPath().path().toUtf8());
    }
}

QmlDesignerProjectManager::ImageCacheData *QmlDesignerProjectManager::imageCacheData()
{
    NanotraceHR::Tracer tracer{"qml designer project manager image cache data", category()};

    std::call_once(imageCacheFlag, [this] {
        m_imageCacheData = std::make_unique<ImageCacheData>();
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
                             &dummy,
                             setTargetInImageCache);
        }
        QObject::connect(ProjectExplorer::ProjectManager::instance(),
                         &ProjectExplorer::ProjectManager::startupProjectChanged,
                         &dummy,
                         [=](ProjectExplorer::Project *project) {
                             setTargetInImageCache(activeTarget(project));
                         });
    });
    return m_imageCacheData.get();
}

void QmlDesignerProjectManager::activeTargetChanged(ProjectExplorer::Target *target)
{
    NanotraceHR::Tracer tracer{"qml designer project manager active target changed", category()};

    if (!m_projectData || !m_projectData->projectStorageData)
        return;

    QObject::disconnect(m_projectData->activeTarget, nullptr, nullptr, nullptr);

    m_projectData->activeTarget = target;

    if (target)
        QObject::connect(target, &::ProjectExplorer::Target::kitChanged, [&]() { kitChanged(); });

    update();
}

void QmlDesignerProjectManager::aboutToRemoveTarget(ProjectExplorer::Target *target)
{
    NanotraceHR::Tracer tracer{"qml designer project manager about to remove target", category()};

    QObject::disconnect(target, nullptr, nullptr, nullptr);
    if (QObject *buildSystem = getQmlBuildSystem(target))
        QObject::disconnect(buildSystem, nullptr, nullptr, nullptr);
}

void QmlDesignerProjectManager::kitChanged()
{
    NanotraceHR::Tracer tracer{"qml designer project manager kit changed", category()};

    QStringList qmldirPaths;
    qmldirPaths.reserve(100);

    update();
}

void QmlDesignerProjectManager::projectChanged()
{
    NanotraceHR::Tracer tracer{"qml designer project manager project changed", category()};

    update();
}

void QmlDesignerProjectManager::update()
{
    NanotraceHR::Tracer tracer{"qml designer project manager update", category()};

    if (!m_projectData || !m_projectData->projectStorageData)
        return;

    // try {
    //     m_projectData->projectStorageData->updater.update(
    //         {directories(m_projectData->activeTarget),
    //          propertyEditorResourcesPath(),
    //          {qtCreatorItemLibraryPath()},
    //          projectDirectory(m_projectData->activeTarget)});
    // } catch (const Sqlite::Exception &exception) {
    //     const auto &location = exception.location();
    //     std::cout << location.file_name() << ":" << location.function_name() << ":"
    //               << location.line() << ": " << exception.what() << "\n";
    // } catch (const std::exception &exception) {
    //     auto location = Sqlite::source_location::current();
    //     std::cout << location.file_name() << ":" << location.function_name() << ":"
    //               << location.line() << ": " << exception.what() << "\n";
    // }
}

} // namespace QmlDesigner
