/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QT4PROJECT_H
#define QT4PROJECT_H

#include "qtversionmanager.h"
#include "qt4nodes.h"
#include "gccpreprocessor.h"
#include "qmakestep.h"
#include "makestep.h"

#include <coreplugin/ifile.h>
#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectnodes.h>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtGui/QDirModel>
#include "qtextended_integration.h"

namespace Core {
    class IWizard;
}

namespace CppTools {
    class ICppCodeModel;
}

QT_BEGIN_NAMESPACE
class ProFile;
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
}

class QMakeStep;
class MakeStep;

class Qt4Manager;
class Qt4Project;
class Qt4RunStep;

class Qt4ProjectFile
    : public Core::IFile
{
    Q_OBJECT

    // needed for createProFileReader
    friend class Internal::Qt4RunConfiguration;

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

class Qt4Project
  : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    explicit Qt4Project(Qt4Manager *manager, const QString &proFile);
    virtual ~Qt4Project();

    QString name() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    Qt4Manager *qt4ProjectManager() const;

    QList<Core::IFile *> dependencies();     //NBS remove
    QList<ProjectExplorer::Project *>dependsOn();

    bool isApplication() const;

    Internal::Qt4ProFileNode *rootProjectNode() const;

    virtual QStringList files(FilesMode fileMode) const;

    //building environment
    ProjectExplorer::Environment environment(const QString &buildConfiguration) const;
    ProjectExplorer::Environment baseEnvironment(const QString &buildConfiguration) const;
    void setUserEnvironmentChanges(const QString &buildConfig, const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges(const QString &buildConfig) const;

    virtual QString buildDirectory(const QString &buildConfiguration) const;

    //Qt4Project specific(?)
    bool useSystemEnvironment(const QString &buildConfiguration) const;
    void setUseSystemEnvironment(const QString &buildConfiguration, bool b);

    // returns the CONFIG variable from the .pro file
    QStringList qmakeConfig() const;
    // returns the qtdir (depends on the current QtVersion)
    QString qtDir(const QString &buildConfiguration) const;
    //returns the qtVersion, if the project is set to use the default qt version, then
    // that is returned
    // to check wheter the project uses the default qt version use qtVersionId
    Internal::QtVersion *qtVersion(const QString &buildConfiguration) const;

    // returns the id of the qt version, if the project is using the default qt version
    // this function returns 0
    int qtVersionId(const QString &buildConfiguration) const;
    //returns the name of the qt version, might be QString::Null, which means default qt version
    // qtVersion is in general the better method to use
    QString qtVersionName(const QString &buildConfiguration) const;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    QList<ProjectExplorer::BuildStepConfigWidget*> subConfigWidgets();

    void setQtVersion(const QString &buildConfiguration, int id);
    virtual void newBuildConfiguration(const QString &buildConfiguration);

    QList<Internal::Qt4ProFileNode *> applicationProFiles() const;
    Internal::ProFileReader *createProFileReader() const;

    // Those functions arein a few places.
    // The drawback is that we shouldn't actually depend on them beeing always there
    // That is generally the stuff that is asked should normally be transfered to
    // Qt4Project *
    // So that we can later enable people to build qt4projects the way they would like
    QMakeStep *qmakeStep() const;
    MakeStep *makeStep() const;

    void notifyChanged(const QString &name);

    // called by qt4ProjectNode to add ui_*.h files to the codemodel
    void addUiFilesToCodeModel(const QStringList &files);

public slots:
    void update();
    void proFileParseError(const QString &errorMessage);
    void scheduleUpdateCodeModel();

private slots:
    void updateCodeModel();
    void defaultQtVersionChanged();
    void qtVersionsChanged();
    void updateFileList();

    void foldersAboutToBeAdded(FolderNode *, const QList<FolderNode*> &);
    void checkForNewApplicationProjects();
    void checkForDeletedApplicationProjects();
    void projectTypeChanged(Qt4ProjectManager::Internal::Qt4ProFileNode *node,
                            const Qt4ProjectManager::Internal::Qt4ProjectType oldType,
                            const Qt4ProjectManager::Internal::Qt4ProjectType newType);
    void proFileUpdated(Qt4ProjectManager::Internal::Qt4ProFileNode *node);
    void addUiFiles();

protected:
    virtual void restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &settingsReader);
    virtual void saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer);

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

    Qt4ProjectFile *m_fileInfo;
    bool m_isApplication;

    // Current configuration
    QString m_oldQtIncludePath;
    QString m_oldQtLibsPath;

    // cached lists of all of files
    Internal::Qt4ProjectFiles *m_projectFiles;

    QTimer m_updateCodeModelTimer;
    QTimer m_addUiFilesTimer;
    QStringList m_uiFilesToAdd;
    Internal::GCCPreprocessor m_preproc;

    friend class Qt4ProjectFile;
};

} // namespace Qt4ProjectManager

#endif // QT4PROJECT_H
