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

#ifndef QMLJSMODELMANAGER_H
#define QMLJSMODELMANAGER_H

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsqrcparser.h>
#include <qmljs/qmljsconstants.h>

#include <cplusplus/CppDocument.h>
#include <utils/qtcoverride.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QTimer)
QT_FORWARD_DECLARE_CLASS(QLocale)

namespace Core { class MimeType; }

namespace CPlusPlus { class CppModelManagerInterface; }

namespace QmlJS { class QrcParser; }

namespace QmlJSTools {

namespace Internal {

class PluginDumper;

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);
    ~ModelManager();

    void delayedInitialization();

    WorkingCopy workingCopy() const QTC_OVERRIDE;
    QmlJS::Snapshot snapshot() const QTC_OVERRIDE;
    QmlJS::Snapshot newestSnapshot() const QTC_OVERRIDE;

    void updateSourceFiles(const QStringList &files,
                           bool emitDocumentOnDiskChanged) QTC_OVERRIDE;
    void fileChangedOnDisk(const QString &path) QTC_OVERRIDE;
    void removeFiles(const QStringList &files) QTC_OVERRIDE;
    QStringList filesAtQrcPath(const QString &path, const QLocale *locale = 0,
                               ProjectExplorer::Project *project = 0,
                               QrcResourceSelector resources = AllQrcResources) QTC_OVERRIDE;
    QMap<QString,QStringList> filesInQrcPath(const QString &path,
                                             const QLocale *locale = 0,
                                             ProjectExplorer::Project *project = 0,
                                             bool addDirs = false,
                                             QrcResourceSelector resources = AllQrcResources) QTC_OVERRIDE;

    QList<ProjectInfo> projectInfos() const QTC_OVERRIDE;
    ProjectInfo projectInfo(ProjectExplorer::Project *project) const QTC_OVERRIDE;
    void updateProjectInfo(const ProjectInfo &pinfo) QTC_OVERRIDE;
    Q_SLOT virtual void removeProjectInfo(ProjectExplorer::Project *project);
    virtual ProjectInfo projectInfoForPath(QString path);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);
    void updateQrcFile(const QString &path);

    QStringList importPaths() const QTC_OVERRIDE;
    QmlJS::QmlLanguageBundles activeBundles() const QTC_OVERRIDE;
    QmlJS::QmlLanguageBundles extendedBundles() const QTC_OVERRIDE;

    void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                         const QString &importUri, const QString &importVersion) QTC_OVERRIDE;

    CppDataHash cppData() const QTC_OVERRIDE;

    QmlJS::LibraryInfo builtins(const QmlJS::Document::Ptr &doc) const QTC_OVERRIDE;

    QmlJS::ViewerContext completeVContext(
            const QmlJS::ViewerContext &vCtx,
            const QmlJS::Document::Ptr &doc = QmlJS::Document::Ptr(0)) const QTC_OVERRIDE;
    QmlJS::ViewerContext defaultVContext(
            bool autoComplete = true,
            const QmlJS::Document::Ptr &doc = QmlJS::Document::Ptr(0)) const QTC_OVERRIDE;
    void setDefaultVContext(const QmlJS::ViewerContext &vContext) QTC_OVERRIDE;

    void joinAllThreads() QTC_OVERRIDE;

public slots:
    void resetCodeModel() QTC_OVERRIDE;

Q_SIGNALS:
    void projectPathChanged(const QString &projectPath);

protected:
    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

    static void parseLoop(QSet<QString> &scannedPaths, QSet<QString> &newLibraries,
                          WorkingCopy workingCopy, QStringList files, ModelManager *modelManager,
                          QmlJS::Language::Enum mainLanguage, bool emitDocChangedOnDisk,
                          Utils::function<bool (qreal)> reportProgress);
    static void parse(QFutureInterface<void> &future,
                      WorkingCopy workingCopy,
                      QStringList files,
                      ModelManager *modelManager,
                      QmlJS::Language::Enum mainLanguage,
                      bool emitDocChangedOnDisk);
    static void importScan(QFutureInterface<void> &future,
                    WorkingCopy workingCopy,
                    QStringList paths,
                    ModelManager *modelManager,
                    QmlJS::Language::Enum mainLanguage,
                    bool emitDocChangedOnDisk);

    void loadQmlTypeDescriptions();
    void loadQmlTypeDescriptions(const QString &path);

    void updateImportPaths();

private slots:
    void maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);
    void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan);
    void startCppQmlTypeUpdate();
    void asyncReset();

private:
    static bool matchesMimeType(const Core::MimeType &fileMimeType, const Core::MimeType &knownMimeType);
    static void updateCppQmlTypes(QFutureInterface<void> &interface,
                                  ModelManager *qmlModelManager,
                                  CPlusPlus::Snapshot snapshot,
                                  QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > documents);

    mutable QMutex m_mutex;
    QmlJS::Snapshot _validSnapshot;
    QmlJS::Snapshot _newestSnapshot;
    QStringList m_allImportPaths;
    QStringList m_defaultImportPaths;
    QmlJS::QmlLanguageBundles m_activeBundles;
    QmlJS::QmlLanguageBundles m_extendedBundles;
    QmlJS::ViewerContext m_vContext;
    bool m_shouldScanImports;
    QSet<QString> m_scannedPaths;

    QTimer *m_updateCppQmlTypesTimer;
    QTimer *m_asyncResetTimer;
    QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > m_queuedCppDocuments;
    QFuture<void> m_cppQmlTypesUpdater;
    QmlJS::QrcCache m_qrcCache;

    CppDataHash m_cppDataHash;
    mutable QMutex m_cppDataMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    PluginDumper *m_pluginDumper;

    QFutureSynchronizer<void> m_synchronizer;

    QHash<QString, QmlJS::Language::Enum> languageForSuffix() const QTC_OVERRIDE;
};

} // namespace Internal

QMLJSTOOLS_EXPORT QmlJS::ModelManagerInterface::ProjectInfo defaultProjectInfoForProject(
        ProjectExplorer::Project *project);
QMLJSTOOLS_EXPORT void setupProjectInfoQmlBundles(QmlJS::ModelManagerInterface::ProjectInfo &projectInfo);

} // namespace QmlJSTools

#endif // QMLJSMODELMANAGER_H
