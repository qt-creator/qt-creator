/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSMODELMANAGERINTERFACE_H
#define QMLJSMODELMANAGERINTERFACE_H

#include "qmljs_global.h"
#include "qmljsbundle.h"
#include "qmljsconstants.h"
#include "qmljsdocument.h"
#include "qmljsqrcparser.h"
#include "qmljsviewercontext.h"

#include <cplusplus/CppDocument.h>
#include <utils/environment.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QHash>
#include <QMultiHash>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QStringList>
#include <QTimer>

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

        operator bool() const
        { return ! project.isNull(); }

        bool isValid() const
        { return ! project.isNull(); }

        bool isNull() const
        { return project.isNull(); }

        QStringList completeImportPaths();

    public: // attributes
        QPointer<ProjectExplorer::Project> project;
        QStringList sourceFiles;
        QStringList importPaths;
        QStringList activeResourceFiles;
        QStringList allResourceFiles;

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
        { _elements.insert(fileName, qMakePair(source, revision)); }

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

public:
    ModelManagerInterface(QObject *parent = 0);
    virtual ~ModelManagerInterface();

    static Language::Enum guessLanguageOfFile(const QString &fileName);
    static QStringList globPatternsForLanguages(const QList<Language::Enum> languages);
    static ModelManagerInterface *instance();
    static void writeWarning(const QString &msg);
    static WorkingCopy workingCopy();

    QmlJS::Snapshot snapshot() const;
    QmlJS::Snapshot newestSnapshot() const;

    void updateSourceFiles(const QStringList &files,
                           bool emitDocumentOnDiskChanged);
    void fileChangedOnDisk(const QString &path);
    void removeFiles(const QStringList &files);
    QStringList filesAtQrcPath(const QString &path, const QLocale *locale = 0,
                               ProjectExplorer::Project *project = 0,
                               QrcResourceSelector resources = AllQrcResources);
    QMap<QString,QStringList> filesInQrcPath(const QString &path,
                                             const QLocale *locale = 0,
                                             ProjectExplorer::Project *project = 0,
                                             bool addDirs = false,
                                             QrcResourceSelector resources = AllQrcResources);

    QList<ProjectInfo> projectInfos() const;
    ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    void updateProjectInfo(const ProjectInfo &pinfo, ProjectExplorer::Project *p);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);
    void updateQrcFile(const QString &path);
    ProjectInfo projectInfoForPath(QString path) const;

    QStringList importPaths() const;
    QmlJS::QmlLanguageBundles activeBundles() const;
    QmlJS::QmlLanguageBundles extendedBundles() const;

    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion);

    CppDataHash cppData() const;
    LibraryInfo builtins(const Document::Ptr &doc) const;
    ViewerContext completeVContext(const ViewerContext &vCtx,
                                   const Document::Ptr &doc = Document::Ptr(0)) const;
    ViewerContext defaultVContext(Language::Enum language = Language::Qml,
                                  const Document::Ptr &doc = Document::Ptr(0),
                                  bool autoComplete = true) const;
    void setDefaultVContext(const ViewerContext &vContext);
    virtual ProjectInfo defaultProjectInfo() const;

    // Blocks until all parsing threads are done. Used for testing.
    void joinAllThreads();
public slots:
    virtual void resetCodeModel();
    void removeProjectInfo(ProjectExplorer::Project *project);
signals:
    void documentUpdated(QmlJS::Document::Ptr doc);
    void documentChangedOnDisk(QmlJS::Document::Ptr doc);
    void aboutToRemoveFiles(const QStringList &files);
    void libraryInfoUpdated(const QString &path, const QmlJS::LibraryInfo &info);
    void projectInfoUpdated(const ProjectInfo &pinfo);
    void projectPathChanged(const QString &projectPath);
protected slots:
    void maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);
    void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan);
    void asyncReset();
    virtual void startCppQmlTypeUpdate();
protected:
    virtual QHash<QString,Language::Enum> languageForSuffix() const;
    virtual void writeMessageInternal(const QString &msg) const;
    virtual WorkingCopy workingCopyInternal() const;
    virtual void addTaskInternal(QFuture<void> result, const QString &msg, const char *taskId) const;

    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

    static void parseLoop(QSet<QString> &scannedPaths, QSet<QString> &newLibraries,
                          WorkingCopy workingCopyInternal, QStringList files, ModelManagerInterface *modelManager,
                          QmlJS::Language::Enum mainLanguage, bool emitDocChangedOnDisk,
                          Utils::function<bool (qreal)> reportProgress);
    static void parse(QFutureInterface<void> &future,
                      WorkingCopy workingCopyInternal,
                      QStringList files,
                      ModelManagerInterface *modelManager,
                      QmlJS::Language::Enum mainLanguage,
                      bool emitDocChangedOnDisk);
    static void importScan(QFutureInterface<void> &future,
                    WorkingCopy workingCopyInternal,
                    QStringList paths,
                    ModelManagerInterface *modelManager,
                    QmlJS::Language::Enum mainLanguage,
                    bool emitDocChangedOnDisk);
    static void updateCppQmlTypes(QFutureInterface<void> &interface,
                                  ModelManagerInterface *qmlModelManager,
                                  CPlusPlus::Snapshot snapshot,
                                  QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > documents);

    void maybeScan(const QStringList &importPaths, Language::Enum defaultLanguage);
    void updateImportPaths();
    void loadQmlTypeDescriptionsInternal(const QString &path);

private:
    mutable QMutex m_mutex;
    QmlJS::Snapshot m_validSnapshot;
    QmlJS::Snapshot m_newestSnapshot;
    QStringList m_allImportPaths;
    QStringList m_defaultImportPaths;
    QmlJS::QmlLanguageBundles m_activeBundles;
    QmlJS::QmlLanguageBundles m_extendedBundles;
    QHash<Language::Enum, QmlJS::ViewerContext> m_defaultVContexts;
    bool m_shouldScanImports;
    QSet<QString> m_scannedPaths;

    QTimer *m_updateCppQmlTypesTimer;
    QTimer *m_asyncResetTimer;
    QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > m_queuedCppDocuments;
    QFuture<void> m_cppQmlTypesUpdater;
    QrcCache m_qrcCache;

    CppDataHash m_cppDataHash;
    mutable QMutex m_cppDataMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;
    QMultiHash<QString, ProjectExplorer::Project *> m_fileToProject;

    PluginDumper *m_pluginDumper;

    QFutureSynchronizer<void> m_synchronizer;
};

} // namespace QmlJS

#endif // QMLJSMODELMANAGERINTERFACE_H
