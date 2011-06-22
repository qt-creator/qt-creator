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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECT_H
#define QT4PROJECT_H

#include "qt4nodes.h"
#include "qt4target.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <coreplugin/ifile.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>
#include <QtCore/QFutureInterface>
#include <QtCore/QTimer>
#include <QtCore/QFuture>

QT_BEGIN_NAMESPACE
struct ProFileOption;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

namespace Internal {
    class ProFileReader;
    class DeployHelperRunStep;
    class FileItem;
    class Qt4ProFileNode;
    class Qt4PriFileNode;
    class GCCPreprocessor;
    struct Qt4ProjectFiles;
    class Qt4ProjectConfigWidget;

    class Qt4NodesWatcher;
}

class QMakeStep;
class MakeStep;

class Qt4Manager;
class Qt4Project;
class Qt4RunStep;

class Qt4ProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    Qt4ProjectFile(Qt4Project *project, const QString &filePath, QObject *parent = 0);

    bool save(const QString &fileName = QString());
    QString fileName() const;
    virtual void rename(const QString &newName);

    QString defaultPath() const;
    QString suggestedFileName() const;
    virtual QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);

private:
    const QString m_mimeType;
    Qt4Project *m_project;
    QString m_filePath;
};

/// Watches folders for Qt4PriFile nodes
/// use one file system watcher to watch all folders
/// such minimizing system ressouce usage
namespace Internal {
class CentralizedFolderWatcher : public QObject
{
    Q_OBJECT
public:
    CentralizedFolderWatcher();
    ~CentralizedFolderWatcher();
    void watchFolders(const QList<QString> &folders, Qt4PriFileNode *node);
    void unwatchFolders(const QList<QString> &folders, Qt4PriFileNode *node);

private slots:
    void folderChanged(const QString &folder);
    void onTimer();
    void delayedFolderChanged(const QString &folder);

private:
    QSet<QString> recursiveDirs(const QString &folder);
    QFileSystemWatcher m_watcher;
    QMultiMap<QString, Qt4PriFileNode *> m_map;

    QSet<QString> m_recursiveWatchedFolders;
    QTimer m_compressTimer;
    QSet<QString> m_changedFolders;
};

}

class Qt4Project : public ProjectExplorer::Project
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

    Internal::Qt4ProFileNode *rootProjectNode() const;
    bool validParse(const QString &proFilePath) const;

    virtual QStringList files(FilesMode fileMode) const;
    virtual QString generatedUiHeader(const QString &formFile) const;

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    QList<Internal::Qt4ProFileNode *> allProFiles() const;
    QList<Internal::Qt4ProFileNode *> applicationProFiles() const;
    bool hasApplicationProFile(const QString &path) const;
    QStringList applicationProFilePathes(const QString &prepend = QString()) const;

    void notifyChanged(const QString &name);

    /// \internal
    Internal::ProFileReader *createProFileReader(Internal::Qt4ProFileNode *qt4ProFileNode, Qt4BuildConfiguration *bc = 0);
    /// \internal
    void destroyProFileReader(Internal::ProFileReader *reader);

    /// \internal
    void scheduleAsyncUpdate(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
    /// \internal
    void incrementPendingEvaluateFutures();
    /// \internal
    void decrementPendingEvaluateFutures();
    /// \internal
    bool wasEvaluateCanceled();

    QString defaultTopLevelBuildDirectory() const;
    static QString defaultTopLevelBuildDirectory(const QString &profilePath);

    Internal::CentralizedFolderWatcher *centralizedFolderWatcher();

    // For Qt4ProFileNode after a on disk change
    void updateFileList();

signals:
    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *node, bool);
    void proFileInvalidated(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
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

    void updateCodeModels();
    void updateCppCodeModel();
    void updateQmlJSCodeModel();


    static void collectAllfProFiles(QList<Internal::Qt4ProFileNode *> &list, Internal::Qt4ProFileNode *node);
    static void collectApplicationProFiles(QList<Internal::Qt4ProFileNode *> &list, Internal::Qt4ProFileNode *node);
    static void findProFile(const QString& fileName, Internal::Qt4ProFileNode *root, QList<Internal::Qt4ProFileNode *> &list);
    static bool hasSubNode(Internal::Qt4PriFileNode *root, const QString &path);

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void addDefaultBuild();

    static QString qmakeVarName(ProjectExplorer::FileType type);

    Qt4Manager *m_manager;
    Internal::Qt4ProFileNode *m_rootProjectNode;
    Internal::Qt4NodesWatcher *m_nodesWatcher;

    Qt4ProjectFile *m_fileInfo;

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
    QList<Internal::Qt4ProFileNode *> m_partialEvaluate;

    QFuture<void> m_codeModelFuture;

    Internal::CentralizedFolderWatcher m_centralizedFolderWatcher;

    friend class Qt4ProjectFile;
    friend class Internal::Qt4ProjectConfigWidget;
};

} // namespace Qt4ProjectManager


#endif // QT4PROJECT_H
