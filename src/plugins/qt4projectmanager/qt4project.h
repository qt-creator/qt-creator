/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
    class Qt4RunConfiguration;
    class GCCPreprocessor;
    struct Qt4ProjectFiles;
    class Qt4ProjectConfigWidget;

    class Qt4NodesWatcher;

    class CodeModelInfo
    {
    public:
        QByteArray defines;
        QStringList includes;
        QStringList frameworkPaths;
        QStringList precompiledHeader;
    };
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

    Internal::Qt4TargetFactory *targetFactory() const;

    Internal::Qt4Target *activeTarget() const;

    QList<Core::IFile *> dependencies();     //NBS remove
    QList<ProjectExplorer::Project *>dependsOn();

    bool isApplication() const;

    Internal::Qt4ProFileNode *rootProjectNode() const;

    virtual QStringList files(FilesMode fileMode) const;
    virtual QString generatedUiHeader(const QString &formFile) const;

    ProjectExplorer::BuildConfigWidget *createConfigWidget();
    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    QList<Internal::Qt4ProFileNode *> applicationProFiles() const;
    bool hasApplicationProFile(const QString &path) const;
    QStringList applicationProFilePathes(const QString &prepend = QString()) const;

    void notifyChanged(const QString &name);

    virtual QByteArray predefinedMacros(const QString &fileName) const;
    virtual QStringList includePaths(const QString &fileName) const;
    virtual QStringList frameworkPaths(const QString &fileName) const;

    /// \internal
    Internal::ProFileReader *createProFileReader(Internal::Qt4ProFileNode *qt4ProFileNode);
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

signals:
    /// emitted after parse
    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
    void buildDirectoryInitialized();

public slots:
    void proFileParseError(const QString &errorMessage);
    void update();

protected:
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileEvaluateNeeded(Qt4ProjectManager::Internal::Qt4Target *target);

    void asyncUpdate();

    void qtVersionsChanged();
    void onAddedTarget(ProjectExplorer::Target *t);
    void activeTargetWasChanged();

private:
    void scheduleAsyncUpdate();

    void checkForNewApplicationProjects();
    void checkForDeletedApplicationProjects();
    void updateCodeModel();
    void updateFileList();

    static void collectApplicationProFiles(QList<Internal::Qt4ProFileNode *> &list, Internal::Qt4ProFileNode *node);
    static void findProFile(const QString& fileName, Internal::Qt4ProFileNode *root, QList<Internal::Qt4ProFileNode *> &list);
    static bool hasSubNode(Internal::Qt4PriFileNode *root, const QString &path);

    static bool equalFileList(const QStringList &a, const QStringList &b);

    void addDefaultBuild();

    static QString qmakeVarName(ProjectExplorer::FileType type);

    Qt4Manager *m_manager;
    Internal::Qt4ProFileNode *m_rootProjectNode;
    Internal::Qt4NodesWatcher *m_nodesWatcher;
    Internal::Qt4TargetFactory *m_targetFactory;

    Qt4ProjectFile *m_fileInfo;
    bool m_isApplication;

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

    QMap<QString, Internal::CodeModelInfo> m_codeModelInfo;
    QFuture<void> m_codeModelFuture;

    friend class Qt4ProjectFile;
    friend class Internal::Qt4ProjectConfigWidget;
};

} // namespace Qt4ProjectManager

#endif // QT4PROJECT_H
