/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef CMAKEPROJECT_H
#define CMAKEPROJECT_H

#include "cmakeprojectmanager.h"
#include "cmakeprojectnodes.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <coreplugin/ifile.h>

#include <QtCore/QXmlStreamReader>

namespace CMakeProjectManager {
namespace Internal{

class CMakeFile;

class CMakeProject : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    CMakeProject(CMakeManager *manager, const QString &filename);
    ~CMakeProject();

    virtual QString name() const;
    virtual Core::IFile *file() const;
    virtual ProjectExplorer::IProjectManager *projectManager() const;

    virtual QList<Core::IFile *> dependencies(); //NBS TODO remove
    virtual QList<ProjectExplorer::Project *> dependsOn(); //NBS TODO implement dependsOn

    virtual bool isApplication() const;

    virtual ProjectExplorer::Environment environment(const QString &buildConfiguration) const;
    virtual QString buildDirectory(const QString &buildConfiguration) const;

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual QList<ProjectExplorer::BuildStepConfigWidget*> subConfigWidgets();

    // This method is called for new build configurations
    // You should probably set some default values in this method
    virtual void newBuildConfiguration(const QString &buildConfiguration);

//    // Returns the list of different views (such as "File View" or "Project View") the project supports.
//    virtual QStringList supportedModels() const = 0;
//
//    // Returns the tree representing the requested view.
//    virtual QModelIndex model(const QString &modelId) const = 0;

    virtual ProjectExplorer::ProjectNode *rootProjectNode() const;

//    // Conversion functions
//    virtual QModelIndex indexForNode(const Node *node, const QString &modelId) const = 0;
//    virtual Node *nodeForIndex(const QModelIndex &index) const = 0;
//    virtual Node *nodeForFile(const QString &filePath) const = 0;

    virtual QStringList files(FilesMode fileMode) const;

private:
    QString findCbpFile(const QDir &);
    QString createCbpFile(const QDir &);

    void buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list);
    ProjectExplorer::FolderNode *findOrCreateFolder(CMakeProjectNode *rootNode, QString directory);


    CMakeManager *m_manager;
    QString m_fileName;
    CMakeFile *m_file;

    // TODO probably need a CMake specific node structure
    CMakeProjectNode* m_rootNode;
    QStringList m_files;

protected:
    virtual void saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer);
    virtual void restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader);

};

class CMakeCbpParser : public QXmlStreamReader
{
public:
    bool parseCbpFile(const QString &fileName);
    QList<ProjectExplorer::FileNode *> fileList();
    QStringList includeFiles();
private:
    void parseCodeBlocks_project_file();
    void parseProject();
    void parseBuild();
    void parseTarget();
    void parseCompiler();
    void parseAdd();
    void parseUnit();
    void parseUnknownElement();

    QList<ProjectExplorer::FileNode *> m_fileList;
    QStringList m_includeFiles;
};

class CMakeFile : public Core::IFile
{
    Q_OBJECT
public:
    CMakeFile(CMakeProject *parent, QString fileName);

    bool save(const QString &fileName = QString());
    QString fileName() const;

    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;

    bool isModified() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;

    void modified(ReloadBehavior *behavior);
private:
    CMakeProject *m_project;
    QString m_fileName;
};

class CMakeBuildSettingsWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    CMakeBuildSettingsWidget();
    virtual QString displayName() const;

    // This is called to set up the config widget before showing it
    // buildConfiguration is QString::null for the non buildConfiguration specific page
    virtual void init(const QString &buildConfiguration);
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEPROJECT_H
