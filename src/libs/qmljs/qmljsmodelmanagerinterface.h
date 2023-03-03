// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include "qmljsbundle.h"
#include "qmljsdocument.h"
#include "qmljsdialect.h"

#include <cplusplus/CppDocument.h>
#include <utils/environment.h>
#include <utils/futuresynchronizer.h>
#include <utils/qrcparser.h>
#include <utils/filepath.h>

#include <QFuture>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QThreadPool>
#include <QVersionNumber>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace ProjectExplorer { class Project; }

namespace QmlJS {

class Snapshot;
class PluginDumper;

class QMLJS_EXPORT ModelManagerInterface : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(ModelManagerInterface)

public:
    ModelManagerInterface(ModelManagerInterface &&) = delete;
    ModelManagerInterface &operator=(ModelManagerInterface &&) = delete;

    enum QrcResourceSelector {
        ActiveQrcResources,
        AllQrcResources
    };

    struct ProjectInfo
    {
        QPointer<ProjectExplorer::Project> project;
        QList<Utils::FilePath> sourceFiles;
        PathsAndLanguages importPaths;
        QList<Utils::FilePath> activeResourceFiles;
        QList<Utils::FilePath> allResourceFiles;
        QList<Utils::FilePath> generatedQrcFiles;
        QHash<Utils::FilePath, QString> resourceFileContents;
        QList<Utils::FilePath> applicationDirectories;
        QHash<QString, QString> moduleMappings; // E.g.: QtQuick.Controls -> MyProject.MyControls

        // whether trying to run qmldump makes sense
        bool tryQmlDump = false;
        bool qmlDumpHasRelocatableFlag = true;
        Utils::FilePath qmlDumpPath;
        Utils::Environment qmlDumpEnvironment;

        Utils::FilePath qtQmlPath;
        Utils::FilePath qmllsPath;
        QString qtVersionString;
        QmlJS::QmlLanguageBundles activeBundle;
        QmlJS::QmlLanguageBundles extendedBundle;
    };

    class WorkingCopy
    {
    public:
        using Table = QHash<Utils::FilePath, QPair<QString, int>>;

        void insert(const Utils::FilePath &fileName, const QString &source, int revision = 0)
        { m_elements.insert(fileName, {source, revision}); }

        bool contains(const Utils::FilePath &fileName) const
        { return m_elements.contains(fileName); }

        QString source(const Utils::FilePath &fileName) const
        { return m_elements.value(fileName).first; }

        QPair<QString, int> get(const Utils::FilePath &fileName) const
        { return m_elements.value(fileName); }

        Table all() const
        { return m_elements; }

    private:
        Table m_elements;
    };

    struct CppData
    {
        QList<LanguageUtils::FakeMetaObject::ConstPtr> exportedTypes;
        QHash<QString, QString> contextProperties;
    };

    using CppDataHash = QHash<QString, CppData>;

public:
    ModelManagerInterface(QObject *parent = nullptr);
    ~ModelManagerInterface() override;

    static Dialect guessLanguageOfFile(const Utils::FilePath &fileName);
    static QStringList globPatternsForLanguages(const QList<Dialect> &languages);
    static ModelManagerInterface *instance();
    static ModelManagerInterface *instanceForFuture(const QFuture<void> &future);
    static void writeWarning(const QString &msg);
    static WorkingCopy workingCopy();
    static Utils::FilePath qmllsForBinPath(const Utils::FilePath &binPath, const QVersionNumber &v);

    QmlJS::Snapshot snapshot() const;
    QmlJS::Snapshot newestSnapshot() const;
    QThreadPool *threadPool();

    void activateScan();
    void updateSourceFiles(const QList<Utils::FilePath> &files, bool emitDocumentOnDiskChanged);
    void fileChangedOnDisk(const Utils::FilePath &path);
    void removeFiles(const QList<Utils::FilePath> &files);
    QStringList qrcPathsForFile(const Utils::FilePath &file,
                                const QLocale *locale = nullptr,
                                ProjectExplorer::Project *project = nullptr,
                                QrcResourceSelector resources = AllQrcResources);
    QStringList filesAtQrcPath(const QString &path, const QLocale *locale = nullptr,
                               ProjectExplorer::Project *project = nullptr,
                               QrcResourceSelector resources = AllQrcResources);
    QMap<QString, QStringList> filesInQrcPath(const QString &path,
                                              const QLocale *locale = nullptr,
                                              ProjectExplorer::Project *project = nullptr,
                                              bool addDirs = false,
                                              QrcResourceSelector resources = AllQrcResources);
    Utils::FilePath fileToSource(const Utils::FilePath &file);

    QList<ProjectInfo> projectInfos() const;
    bool containsProject(ProjectExplorer::Project *project) const;
    ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    void updateProjectInfo(const ProjectInfo &pinfo, ProjectExplorer::Project *p);

    void updateDocument(const QmlJS::Document::Ptr& doc);
    void updateLibraryInfo(const Utils::FilePath &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);
    void updateQrcFile(const Utils::FilePath &path);
    ProjectInfo projectInfoForPath(const Utils::FilePath &path) const;
    QList<ProjectInfo> allProjectInfosForPath(const Utils::FilePath &path) const;

    QList<Utils::FilePath> importPathsNames() const;
    QmlJS::QmlLanguageBundles activeBundles() const;
    QmlJS::QmlLanguageBundles extendedBundles() const;

    void loadPluginTypes(const Utils::FilePath &libraryPath,
                         const Utils::FilePath &importPath,
                         const QString &importUri,
                         const QString &importVersion);

    CppDataHash cppData() const;
    LibraryInfo builtins(const Document::Ptr &doc) const;
    ViewerContext completeVContext(const ViewerContext &vCtx,
                                   const Document::Ptr &doc = Document::Ptr(nullptr)) const;
    ViewerContext defaultVContext(Dialect language = Dialect::Qml,
                                  const Document::Ptr &doc = Document::Ptr(nullptr),
                                  bool autoComplete = true) const;
    ViewerContext projectVContext(Dialect language, const Document::Ptr &doc) const;

    void setDefaultVContext(const ViewerContext &vContext);
    virtual ProjectInfo defaultProjectInfo() const;
    virtual ProjectInfo defaultProjectInfoForProject(ProjectExplorer::Project *project,
                                                     const Utils::FilePaths &hiddenRccFolders) const;

    // Blocks until all parsing threads are done. Use for testing only!
    void test_joinAllThreads();

    template <typename T>
    void addFuture(const QFuture<T> &future) { addFuture(QFuture<void>(future)); }
    void addFuture(const QFuture<void> &future);

    QmlJS::Document::Ptr ensuredGetDocumentForPath(const Utils::FilePath &filePath);
    static void importScan(const WorkingCopy &workingCopy, const PathsAndLanguages &paths,
                           ModelManagerInterface *modelManager, bool emitDocChanged,
                           bool libOnly = true, bool forceRescan = false);
    static void importScanAsync(QPromise<void> &promise, const WorkingCopy& workingCopyInternal,
                                const PathsAndLanguages& paths, ModelManagerInterface *modelManager,
                                bool emitDocChanged, bool libOnly = true, bool forceRescan = false);

    virtual void resetCodeModel();
    void removeProjectInfo(ProjectExplorer::Project *project);
    void maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);

    QFuture<void> refreshSourceFiles(const QList<Utils::FilePath> &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

signals:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void documentChangedOnDisk(QmlJS::Document::Ptr doc);
    void aboutToRemoveFiles(const QList<Utils::FilePath> &files);
    void libraryInfoUpdated(const Utils::FilePath &path, const QmlJS::LibraryInfo &info);
    void projectInfoUpdated(const ProjectInfo &pinfo);
    void projectPathChanged(const Utils::FilePath &projectPath);

protected:
    Q_INVOKABLE void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan);
    Q_INVOKABLE void asyncReset();
    virtual void startCppQmlTypeUpdate();
    QMutex *mutex() const;
    virtual QHash<QString,Dialect> languageForSuffix() const;
    virtual void writeMessageInternal(const QString &msg) const;
    virtual WorkingCopy workingCopyInternal() const;
    virtual void addTaskInternal(const QFuture<void> &result, const QString &msg,
                                 const char *taskId) const;

    static void parseLoop(QSet<Utils::FilePath> &scannedPaths,
                          QSet<Utils::FilePath> &newLibraries,
                          const WorkingCopy &workingCopyInternal,
                          QList<Utils::FilePath> files,
                          ModelManagerInterface *modelManager,
                          QmlJS::Dialect mainLanguage,
                          bool emitDocChangedOnDisk,
                          const std::function<bool(qreal)> &reportProgress);
    static void parse(QPromise<void> &promise,
                      const WorkingCopy &workingCopyInternal,
                      QList<Utils::FilePath> files,
                      ModelManagerInterface *modelManager,
                      QmlJS::Dialect mainLanguage,
                      bool emitDocChangedOnDisk);
    static void updateCppQmlTypes(QPromise<void> &promise, ModelManagerInterface *qmlModelManager,
                const CPlusPlus::Snapshot &snapshot,
                const QHash<QString, QPair<CPlusPlus::Document::Ptr, bool>> &documents);

    void maybeScan(const PathsAndLanguages &importPaths);
    void updateImportPaths();
    void loadQmlTypeDescriptionsInternal(const QString &path);
    void setDefaultProject(const ProjectInfo &pInfo, ProjectExplorer::Project *p);

private:
    void joinAllThreads(bool cancelOnWait = false);
    void iterateQrcFiles(ProjectExplorer::Project *project,
                         QrcResourceSelector resources,
                         const std::function<void(Utils::QrcParser::ConstPtr)> &callback);
    ViewerContext getVContext(const ViewerContext &vCtx, const Document::Ptr &doc, bool limitToProject) const;

    mutable QMutex m_mutex;
    QmlJS::Snapshot m_validSnapshot;
    QmlJS::Snapshot m_newestSnapshot;
    PathsAndLanguages m_allImportPaths;
    QList<Utils::FilePath> m_applicationPaths;
    QList<Utils::FilePath> m_defaultImportPaths;
    QmlJS::QmlLanguageBundles m_activeBundles;
    QmlJS::QmlLanguageBundles m_extendedBundles;
    QHash<Dialect, QmlJS::ViewerContext> m_defaultVContexts;
    bool m_shouldScanImports = false;
    QSet<Utils::FilePath> m_scannedPaths;

    QTimer *m_updateCppQmlTypesTimer = nullptr;
    QTimer *m_asyncResetTimer = nullptr;
    QHash<QString, QPair<CPlusPlus::Document::Ptr, bool>> m_queuedCppDocuments;
    QFuture<void> m_cppQmlTypesUpdater;
    Utils::QrcCache m_qrcCache;
    QHash<Utils::FilePath, QString> m_qrcContents;

    CppDataHash m_cppDataHash;
    QHash<QString, QList<CPlusPlus::Document::Ptr>> m_cppDeclarationFiles;
    mutable QMutex m_cppDataMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;
    ProjectInfo m_defaultProjectInfo;
    ProjectExplorer::Project *m_defaultProject = nullptr;
    QMultiHash<Utils::FilePath, ProjectExplorer::Project *> m_fileToProject;

    PluginDumper *m_pluginDumper = nullptr;

    mutable QMutex m_futuresMutex;
    Utils::FutureSynchronizer m_futureSynchronizer;

    bool m_indexerDisabled = false;
    QThreadPool m_threadPool;
};

} // namespace QmlJS
