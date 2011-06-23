/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLJSMODELMANAGER_H
#define QMLJSMODELMANAGER_H

#include "qmljstools_global.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <cplusplus/ModelManagerInterface.h>

#include <QtCore/QFuture>
#include <QtCore/QFutureSynchronizer>
#include <QtCore/QMutex>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core {
class ICore;
class MimeType;
}

namespace QmlJSTools {
namespace Internal {

class PluginDumper;

class QMLJSTOOLS_EXPORT ModelManager: public QmlJS::ModelManagerInterface
{
    Q_OBJECT

public:
    ModelManager(QObject *parent = 0);

    void delayedInitialization();

    virtual WorkingCopy workingCopy() const;
    virtual QmlJS::Snapshot snapshot() const;

    virtual void updateSourceFiles(const QStringList &files,
                                   bool emitDocumentOnDiskChanged);
    virtual void fileChangedOnDisk(const QString &path);
    virtual void removeFiles(const QStringList &files);

    virtual QList<ProjectInfo> projectInfos() const;
    virtual ProjectInfo projectInfo(ProjectExplorer::Project *project) const;
    virtual void updateProjectInfo(const ProjectInfo &pinfo);

    void updateDocument(QmlJS::Document::Ptr doc);
    void updateLibraryInfo(const QString &path, const QmlJS::LibraryInfo &info);
    void emitDocumentChangedOnDisk(QmlJS::Document::Ptr doc);

    virtual QStringList importPaths() const;

    virtual void loadPluginTypes(const QString &libraryPath, const QString &importPath,
                                 const QString &importUri, const QString &importVersion);

    virtual CppQmlTypeHash cppQmlTypes() const;
    virtual BuiltinPackagesHash builtinPackages() const;

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
    void queueCppQmlTypeUpdate(const CPlusPlus::Document::Ptr &doc);
    void startCppQmlTypeUpdate();

private:
    static bool matchesMimeType(const Core::MimeType &fileMimeType, const Core::MimeType &knownMimeType);
    static void updateCppQmlTypes(ModelManager *qmlModelManager, CPlusPlus::CppModelManagerInterface *cppModelManager, QSet<QString> files);

    mutable QMutex m_mutex;
    Core::ICore *m_core;
    QmlJS::Snapshot _snapshot;
    QStringList m_allImportPaths;
    QStringList m_defaultImportPaths;

    QFutureSynchronizer<void> m_synchronizer;

    QTimer *m_updateCppQmlTypesTimer;
    QSet<QString> m_queuedCppDocuments;
    CppQmlTypeHash m_cppTypes;
    mutable QMutex m_cppTypesMutex;

    // project integration
    QMap<ProjectExplorer::Project *, ProjectInfo> m_projects;

    PluginDumper *m_pluginDumper;
};

} // namespace Internal
} // namespace QmlJSTools

#endif // QMLJSMODELMANAGER_H
