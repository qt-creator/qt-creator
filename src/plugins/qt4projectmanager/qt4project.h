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

#ifndef QT4PROJECT_H
#define QT4PROJECT_H

#include "qt4projectmanager_global.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QFutureInterface>
#include <QtCore/QTimer>
#include <QtCore/QFuture>

QT_BEGIN_NAMESPACE
struct ProFileOption;
QT_END_NAMESPACE

namespace QtSupport {
class ProFileReader;
}

namespace Qt4ProjectManager {
class Qt4ProFileNode;
class Qt4PriFileNode;
class Qt4BaseTarget;
class Qt4BuildConfiguration;

namespace Internal {
    class DeployHelperRunStep;
    class FileItem;
    class GCCPreprocessor;
    struct Qt4ProjectFiles;
    class Qt4ProjectConfigWidget;
    class Qt4ProjectFile;
    class Qt4NodesWatcher;
}

class QMakeStep;
class MakeStep;

class Qt4Manager;
class Qt4Project;
class Qt4RunStep;

namespace Internal {
class CentralizedFolderWatcher;
}

class  QT4PROJECTMANAGER_EXPORT Qt4Project : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    Qt4Project(Qt4Manager *manager, const QString &proFile);
    virtual ~Qt4Project();

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    Qt4Manager *qt4ProjectManager() const;

    Qt4BaseTarget *activeTarget() const;

    QList<Core::IFile *> dependencies();     //NBS remove
    QList<ProjectExplorer::Project *>dependsOn();

    ProjectExplorer::ProjectNode *rootProjectNode() const;
    Qt4ProFileNode *rootQt4ProjectNode() const;
    bool validParse(const QString &proFilePath) const;
    bool parseInProgress(const QString &proFilePath) const;

    virtual QStringList files(FilesMode fileMode) const;
    virtual QString generatedUiHeader(const QString &formFile) const;

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    QList<Qt4ProFileNode *> allProFiles() const;
    QList<Qt4ProFileNode *> applicationProFiles() const;
    bool hasApplicationProFile(const QString &path) const;
    QStringList applicationProFilePathes(const QString &prepend = QString()) const;

    void notifyChanged(const QString &name);

    /// \internal
    QtSupport::ProFileReader *createProFileReader(Qt4ProFileNode *qt4ProFileNode, Qt4BuildConfiguration *bc = 0);
    /// \internal
    void destroyProFileReader(QtSupport::ProFileReader *reader);

    /// \internal
    void scheduleAsyncUpdate(Qt4ProjectManager::Qt4ProFileNode *node);
    /// \internal
    void incrementPendingEvaluateFutures();
    /// \internal
    void decrementPendingEvaluateFutures();
    /// \internal
    bool wasEvaluateCanceled();

    // For Qt4ProFileNode after a on disk change
    void updateFileList();
    void updateCodeModels();

    void watchFolders(const QStringList &l, Qt4PriFileNode *node);
    void unwatchFolders(const QStringList &l, Qt4PriFileNode *node);

signals:
    void proParsingDone();
    void proFileUpdated(Qt4ProjectManager::Qt4ProFileNode *node, bool, bool);
    void buildDirectoryInitialized();
    void fromMapFinished();

public slots:
    void proFileParseError(const QString &errorMessage);
    void update();

protected:
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileEvaluateNeeded(Qt4ProjectManager::Qt4BaseTarget *target);

    void asyncUpdate();

    void onAddedTarget(ProjectExplorer::Target *t);
    void activeTargetWasChanged();

private:
    void scheduleAsyncUpdate();

    void updateCppCodeModel();
    void updateQmlJSCodeModel();


    static void collectAllfProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node);
    static void collectApplicationProFiles(QList<Qt4ProFileNode *> &list, Qt4ProFileNode *node);
    static void findProFile(const QString& fileName, Qt4ProFileNode *root, QList<Qt4ProFileNode *> &list);
    static bool hasSubNode(Qt4PriFileNode *root, const QString &path);

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void addDefaultBuild();

    static QString qmakeVarName(ProjectExplorer::FileType type);

    Qt4Manager *m_manager;
    Qt4ProFileNode *m_rootProjectNode;
    Internal::Qt4NodesWatcher *m_nodesWatcher;

    Internal::Qt4ProjectFile *m_fileInfo;

    // Current configuration
    QString m_oldQtIncludePath;
    QString m_oldQtLibsPath;

    // cached lists of all of files
    Internal::Qt4ProjectFiles *m_projectFiles;

    // cached data during project rescan
    ProFileOption *m_proFileOption;
    int m_proFileOptionRefCnt;

    QTimer m_asyncUpdateTimer;
    QFutureInterface<void> *m_asyncUpdateFutureInterface;
    int m_pendingEvaluateFuturesCount;
    enum AsyncUpdateState { NoState, Base, AsyncFullUpdatePending, AsyncPartialUpdatePending, AsyncUpdateInProgress, ShuttingDown };
    AsyncUpdateState m_asyncUpdateState;
    bool m_cancelEvaluate;
    bool m_codeModelCanceled;
    QList<Qt4ProFileNode *> m_partialEvaluate;

    QFuture<void> m_codeModelFuture;

    Internal::CentralizedFolderWatcher *m_centralizedFolderWatcher;

    friend class Internal::Qt4ProjectFile;
    friend class Internal::Qt4ProjectConfigWidget;
};

} // namespace Qt4ProjectManager


#endif // QT4PROJECT_H
