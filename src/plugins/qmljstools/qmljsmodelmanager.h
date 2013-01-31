/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <cplusplus/CppDocument.h>

#include <QFuture>
#include <QFutureSynchronizer>
#include <QMutex>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core {
class MimeType;
}

namespace CPlusPlus {
class CppModelManagerInterface;
}

namespace QmlJSTools {

QMLJSTOOLS_EXPORT QmlJS::Document::Language languageOfFile(const QString &fileName);
QMLJSTOOLS_EXPORT QStringList qmlAndJsGlobPatterns();

namespace Internal {

class PluginDumper;

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);
    ~ModelManager();

    void delayedInitialization();

    virtual WorkingCopy workingCopy() const;
    virtual QmlJS::Snapshot snapshot() const;
    virtual QmlJS::Snapshot newestSnapshot() const;

    virtual void updateSourceFiles(const QStringList &files,
                                   bool emitDocumentOnDiskChanged);
    virtual void fileChangedOnDisk(const QString &path);
    virtual void removeFiles(const QStringList &files);

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);
    Q_SLOT virtual void removeProjectInfo(ProjectExplorer::Project *project);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);

    virtual QStringList importPaths() const;

    virtual void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                                 const QString &importUri, const QString &importVersion);

    virtual CppDataHash cppData() const;

    virtual QmlJS::LibraryInfo builtins(const QmlJS::Document::Ptr &doc) const;

    virtual void joinAllThreads();

public slots:
    virtual void resetCodeModel();

Q_SIGNALS:
    void projectPathChanged(const QString &projectPath);

protected:
    QFuture<void> refreshSourceFiles(const QStringList &sourceFiles,
                                     bool emitDocumentOnDiskChanged);

    static void parse(QFutureInterface<void> &future,
                      WorkingCopy workingCopy,
                      QStringList files,
                      ModelManager *modelManager,
                      bool emitDocChangedOnDisk);

    void loadQmlTypeDescriptions();
    void loadQmlTypeDescriptions(const QString &path);

    void updateImportPaths();

private slots:
    void maybeQueueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);
    void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc, bool scan);
    void startCppQmlTypeUpdate();

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

    QFutureSynchronizer<void> m_synchronizer;

    QTimer *m_updateCppQmlTypesTimer;
    QHash<QString, QPair<CPlusPlus::Document::Ptr, bool> > m_queuedCppDocuments;
    QFuture<void> m_cppQmlTypesUpdater;

    CppDataHash m_cppDataHash;
    mutable QMutex m_cppDataMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    PluginDumper *m_pluginDumper;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSMODELMANAGER_H
