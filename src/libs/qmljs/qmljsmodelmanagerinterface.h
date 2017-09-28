/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "qmljs_global.h"
#include "qmljsbundle.h"
#include "qmljsdocument.h"
#include "qmljsqrcparser.h"
#include "qmljsdialect.h"

#include <cplusplus/CppDocument.h>
#include <utils/environment.h>

#include <QFuture>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace ProjectExplorer { class Project; }

namespace QmlJS {

class Snapshot;
class PluginDumper;

class QMLJS_EXPORT ModelManagerInterface: public QObject
{
    Q_OBJECT

public:
    enum QrcResourceSelector {
        ActiveQrcResources,
        AllQrcResources
    };

    class ProjectInfo
    {
    public:
        ProjectInfo()
            : tryQmlDump(false), qmlDumpHasRelocatableFlag(true)
        { }

        ProjectInfo(QPointer<ProjectExplorer::Project> project)
            : project(project)
            , tryQmlDump(false), qmlDumpHasRelocatableFlag(true)
        { }

        explicit operator bool() const
        { return ! project.isNull(); }

        bool isValid() const
        { return ! project.isNull(); }

        bool isNull() const
        { return project.isNull(); }

    public: // attributes
        QPointer<ProjectExplorer::Project> project;
        QStringList sourceFiles;
        PathsAndLanguages importPaths;
        QStringList activeResourceFiles;
        QStringList allResourceFiles;
        QHash<QString, QString> resourceFileContents;

        // whether trying to run qmldump makes sense
        bool tryQmlDump;
        bool qmlDumpHasRelocatableFlag;
        QString qmlDumpPath;
        ::Utils::Environment qmlDumpEnvironment;

        QString qtImportsPath;
        QString qtQmlPath;
        QString qtVersionString;
        QmlJS::QmlLanguageBundles activeBundle;
        QmlJS::QmlLanguageBundles extendedBundle;
    };

    class WorkingCopy
    {
    public:
        typedef QHash<QString, QPair<QString, int> > Table;

        void insert(const QString &fileName, const QString &source, int revision = 0)
        { _elements.insert(fileName, {source, revision}); }

        bool contains(const QString &fileName) const
        { return _elements.contains(fileName); }

        QString source(const QString &fileName) const
        { return _elements.value(fileName).first; }

        QPair<QString, int> get(const QString &fileName) const
        { return _elements.value(fileName); }

        Table all() const
        { return _elements; }

    private:
        Table _elements;
    };

    class CppData
    {
    public:
        QList<LanguageUtils::FakeMetaObject::ConstPtr> exportedTypes;
        QHash<QString, QString> contextProperties;
    };

    typedef QHash<QString, CppData> CppDataHash;
    typedef QHashIterator<QString, CppData> CppDataHashIterator;

public:
    ModelManagerInterface(QObject *parent = 0);
    ~ModelManagerInterface() override;

    static Dialect guessLanguageOfFile(const QString &fileName);
    static QStringList globPatternsForLanguages(const QList<Dialect> languages);
    static ModelManagerInterface *instance();
    static void writeWarning(const QString &msg);
    static WorkingCopy workingCopy();

    QmlJS::Snapshot snapshot() const;
    QmlJS::Snapshot newestSnapshot() const;

    void activateScan();
    void updateSourceFiles(const QStringList &files,
                           bool emitDocumentOnDiskChanged);
    void fileChangedOnDisk(const QString &path);
    void removeFiles(const QStringList &files);
    QStringList qrcPathsForFile(const QString &file, const QLocale *locale = 0,
                                ProjectExplorer::Project *project = 0,
                                QrcResourceSelector resources = AllQrcResources);
    QStringList filesAtQrcPath(const QString &path, const QLocale *locale = 0,
                               ProjectExplorer::Project *project = 0,
                               QrcResourceSelector resources = AllQrcResources);
    QMap<QString,QStringList> filesInQrcPath(const QString &path,
                                             const QLocale *locale = 0,
                                             ProjectExplorer::Project *project = 0,
                                             bool addDirs = false,
                                             QrcResourceSelector resources = AllQrcResources);

    QList<ProjectInfo> projectInfos() const;
    ProjectInfo projectInfo(ProjectExplorer::Project *project,
                            const ModelManagerInterface::ProjectInfo &defaultValue = ProjectInfo()) const;
    void updateProjectInfo(const ProjectInfo &pinfo, ProjectExplorer::Project *p);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);
    void updateQrcFile(const QString &path);
    ProjectInfo projectInfoForPath(const QString &path) const;
    QList<ProjectInfo> allProjectInfosForPath(const QString &path) const;
    bool isIdle() const ;

    QStringList importPathsNames() const;
    QmlJS::QmlLanguageBundles activeBundles() const;
    QmlJS::QmlLanguageBundles extendedBundles() const;

    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion);

    CppDataHash cppData() const;
    LibraryInfo builtins(const Document::Ptr &doc) const;
    ViewerContext completeVContext(const ViewerContext &vCtx,
                                   const Document::Ptr &doc = Document::Ptr(0)) const;
    ViewerContext defaultVContext(Dialect language = Dialect::Qml,
                                  const Document::Ptr &doc = Document::Ptr(0),
                                  bool autoComplete = true) const;
    void setDefaultVContext(const ViewerContext &vContext);
    virtual ProjectInfo defaultProjectInfo() const;
    virtual ProjectInfo defaultProjectInfoForProject(ProjectExplorer::Project *project) const;


    // Blocks until all parsing threads are done. Used for testing.
    void joinAllThreads();

    QmlJS::Document::Ptr ensuredGetDocumentForPath(const QString &filePath);
    static void importScan(QFutureInterface<void> &future,
                    WorkingCopy workingCopyInternal,
                    PathsAndLanguages paths,
                    ModelManagerInterface *modelManager,
                    bool emitDocChangedOnDisk, bool libOnly = true, bool forceRescan = false);

    virtual void resetCodeModel();
    void removeProjectInfo(ProjectExplorer::Project *project);
    void maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);

signals:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void documentChangedOnDisk(QmlJS::Document::Ptr doc);
    void aboutToRemoveFiles(const QStringList &files);
    void libraryInfoUpdated(const QString &path, const QmlJS::LibraryInfo &info);
    void projectInfoUpdated(const ProjectInfo &pinfo);
    void projectPathChanged(const QString &projectPath);

protected:
    Q_INVOKABLE void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan);
    Q_INVOKABLE void asyncReset();
    virtual void startCppQmlTypeUpdate();
    QMutex *mutex() const;
    virtual QHash<QString,Dialect> languageForSuffix() const;
    virtual void writeMessageInternal(const QString &msg) const;
    virtual WorkingCopy workingCopyInternal() const;
    virtual void addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const;

    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

    static void parseLoop(QSet<QString> &scannedPaths, QSet<QString> &newLibraries,
                          WorkingCopy workingCopyInternal, QStringList files, ModelManagerInterface *modelManager,
                          QmlJS::Dialect mainLanguage, bool emitDocChangedOnDisk,
                          std::function<bool (qreal)> reportProgress);
    static void parse(QFutureInterface<void> &future,
                      WorkingCopy workingCopyInternal,
                      QStringList files,
                      ModelManagerInterface *modelManager,
                      QmlJS::Dialect mainLanguage,
                      bool emitDocChangedOnDisk);
    static void updateCppQmlTypes(QFutureInterface<void> &futureInterface,
                                  ModelManagerInterface *qmlModelManager,
                                  CPlusPlus::Snapshot snapshot,
                                  QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > documents);

    void maybeScan(const PathsAndLanguages &importPaths);
    void updateImportPaths();
    void loadQmlTypeDescriptionsInternal(const QString &path);
    void setDefaultProject(const ProjectInfo &pInfo, ProjectExplorer::Project *p);

private:
    void cleanupFutures();
    void iterateQrcFiles(ProjectExplorer::Project *project,
                         QrcResourceSelector resources,
                         std::function<void(QrcParser::ConstPtr)> callback);

    mutable QMutex m_mutex;
    QmlJS::Snapshot m_validSnapshot;
    QmlJS::Snapshot m_newestSnapshot;
    PathsAndLanguages m_allImportPaths;
    QStringList m_defaultImportPaths;
    QmlJS::QmlLanguageBundles m_activeBundles;
    QmlJS::QmlLanguageBundles m_extendedBundles;
    QHash<Dialect, QmlJS::ViewerContext> m_defaultVContexts;
    bool m_shouldScanImports;
    QSet<QString> m_scannedPaths;

    QTimer *m_updateCppQmlTypesTimer;
    QTimer *m_asyncResetTimer;
    QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > m_queuedCppDocuments;
    QFuture<void> m_cppQmlTypesUpdater;
    QrcCache m_qrcCache;
    QHash<QString, QString> m_qrcContents;

    CppDataHash m_cppDataHash;
    QHash<QString, QList<CPlusPlus::Document::Ptr> > m_cppDeclarationFiles;
    mutable QMutex m_cppDataMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;
    ProjectInfo m_defaultProjectInfo;
    ProjectExplorer::Project *m_defaultProject;
    QMultiHash<QString, ProjectExplorer::Project *> m_fileToProject;

    PluginDumper *m_pluginDumper;

    QList<QFuture<void>> m_futures;
    bool m_indexerEnabled;
};

} // namespace QmlJS
