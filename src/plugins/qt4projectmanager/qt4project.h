/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "profileevaluator.h"
#include "qt4nodes.h"
#include "qt4target.h"
#include "qmakestep.h"
#include "makestep.h"
#include "qtversionmanager.h"

#include <coreplugin/ifile.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtCore/QMap>
#include <QtGui/QDirModel>
#include "qtextended_integration.h"

namespace Core {
    class IWizard;
}

namespace CppTools {
    class ICppCodeModel;
}

QT_BEGIN_NAMESPACE
struct ProFileOption;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

namespace Internal {
    class ProFileReader;
    class DeployHelperRunStep;
    class FileItem;
    class Qt4ProFileNode;
    class Qt4RunConfiguration;
    class GCCPreprocessor;
    struct Qt4ProjectFiles;
    class Qt4ProjectConfigWidget;

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

    void modified(Core::IFile::ReloadBehavior *behavior);

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

    // returns the CONFIG variable from the .pro file
    QStringList qmakeConfig() const;

    ProjectExplorer::BuildConfigWidget *createConfigWidget();
    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    QList<Internal::Qt4ProFileNode *> applicationProFiles() const;
    bool hasApplicationProFile(const QString &path) const;
    QStringList applicationProFilePathes(const QString &prepend = QString()) const;

    void notifyChanged(const QString &name);

    virtual QByteArray predefinedMacros(const QString &fileName) const;
    virtual QStringList includePaths(const QString &fileName) const;
    virtual QStringList frameworkPaths(const QString &fileName) const;

    Internal::ProFileReader *createProFileReader(Internal::Qt4ProFileNode *qt4ProFileNode);
    void destroyProFileReader(Internal::ProFileReader *reader);

signals:
    /// convenience signal, emitted if either the active buildconfiguration emits
    /// targetInformationChanged() or if the active build configuration changes
    /// (which can happen by the active target changing, too).
    void targetInformationChanged();
    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
    void buildDirectoryInitialized();

public slots:
    void proFileParseError(const QString &errorMessage);
    void update();
    void changeTargetInformation();

private slots:
    void updateCodeModel();
    void scheduleUpdateCodeModel(Qt4ProjectManager::Internal::Qt4ProFileNode *);
    void qtVersionsChanged();
    void updateFileList();
    void onAddedTarget(ProjectExplorer::Target *t);

    void foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &);
    void checkForNewApplicationProjects();
    void checkForDeletedApplicationProjects();
    void projectTypeChanged(Qt4ProjectManager::Internal::Qt4ProFileNode *node,
                            const Qt4ProjectManager::Internal::Qt4ProjectType oldType,
                            const Qt4ProjectManager::Internal::Qt4ProjectType newType);
    void activeTargetWasChanged();

protected:
    virtual bool fromMap(const QVariantMap &map);

private:
    static void collectApplicationProFiles(QList<Internal::Qt4ProFileNode *> &list, Internal::Qt4ProFileNode *node);
    static void findProFile(const QString& fileName, Internal::Qt4ProFileNode *root, QList<Internal::Qt4ProFileNode *> &list);
    static bool hasSubNode(Internal::Qt4PriFileNode *root, const QString &path);

    QList<Internal::Qt4ProFileNode *> m_applicationProFileChange;
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer() const;

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

    QTimer m_updateCodeModelTimer;
    QList<Qt4ProjectManager::Internal::Qt4ProFileNode *> m_proFilesForCodeModelUpdate;

    QMap<QString, Internal::CodeModelInfo> m_codeModelInfo;

    friend class Qt4ProjectFile;
    friend class Internal::Qt4ProjectConfigWidget;

    // cached data during project rescan
    ProFileOption *m_proFileOption;
    int m_proFileOptionRefCnt;
};

} // namespace Qt4ProjectManager

#endif // QT4PROJECT_H
