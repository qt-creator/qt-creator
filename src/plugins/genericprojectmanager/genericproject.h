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

#ifndef GENERICPROJECT_H
#define GENERICPROJECT_H

#include "genericprojectmanager.h"
#include "genericprojectnodes.h"
#include "makestep.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/toolchain.h>
#include <coreplugin/ifile.h>
#include <utils/pathchooser.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QStringListModel;
QT_END_NAMESPACE

namespace GenericProjectManager {
namespace Internal{

class GenericProjectFile;

class GenericProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    GenericProject(Manager *manager, const QString &filename);
    virtual ~GenericProject();

    QString filesFileName() const;
    QString includesFileName() const;
    QString configFileName() const;

    virtual QString name() const;
    virtual Core::IFile *file() const;
    virtual ProjectExplorer::IProjectManager *projectManager() const;

    virtual QList<ProjectExplorer::Project *> dependsOn();

    virtual bool isApplication() const;

    virtual ProjectExplorer::Environment environment(const QString &buildConfiguration) const;
    virtual QString buildDirectory(const QString &buildConfiguration) const;

    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual QList<ProjectExplorer::BuildStepConfigWidget*> subConfigWidgets();

    virtual void newBuildConfiguration(const QString &buildConfiguration);
    virtual GenericProjectNode *rootProjectNode() const;
    virtual QStringList files(FilesMode fileMode) const;

    QStringList targets() const;
    MakeStep *makeStep() const;
    QString buildParser(const QString &buildConfiguration) const;

    QStringList convertToAbsoluteFiles(const QStringList &paths) const;

    QStringList includePaths() const;
    void setIncludePaths(const QStringList &includePaths);

    QStringList defines() const;
    void setDefines(const QStringList &defines);

    QStringList allIncludePaths() const;
    QStringList projectIncludePaths() const;
    QStringList files() const;
    QStringList generated() const;    
    QString toolChainId() const;

public Q_SLOTS:
    void setToolChainId(const QString &toolChainId);
    void refresh();

protected:
    virtual void saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer);
    virtual void restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader);

private:
    void parseProject();

    QStringList readLines(const QString &absoluteFileName) const;

    Manager *_manager;
    QString _fileName;
    QString _filesFileName;
    QString _includesFileName;
    QString _configFileName;
    GenericProjectFile *_file;
    QString _projectName;

    QStringList _files;
    QStringList _generated;
    QStringList _includePaths;
    QStringList _projectIncludePaths;
    QStringList _defines;

    GenericProjectNode* _rootNode;
    ProjectExplorer::ToolChain *_toolChain;
    QString _toolChainId;
};

class GenericProjectFile : public Core::IFile
{
    Q_OBJECT

public:
    GenericProjectFile(GenericProject *parent, QString fileName);
    virtual ~GenericProjectFile();

    virtual bool save(const QString &fileName = QString());
    virtual QString fileName() const;

    virtual QString defaultPath() const;
    virtual QString suggestedFileName() const;
    virtual QString mimeType() const;

    virtual bool isModified() const;
    virtual bool isReadOnly() const;
    virtual bool isSaveAsAllowed() const;

    virtual void modified(ReloadBehavior *behavior);

private:
    GenericProject *_project;
    QString _fileName;
};

class GenericBuildSettingsWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    GenericBuildSettingsWidget(GenericProject *project);
    virtual ~GenericBuildSettingsWidget();

    virtual QString displayName() const;

    virtual void init(const QString &buildConfiguration);

private Q_SLOTS:
    void buildDirectoryChanged();
    void markDirty();
    void applyChanges();

private:
    GenericProject *_project;
    Core::Utils::PathChooser *_pathChooser;
    QString _buildConfiguration;
    QPushButton *_applyButton;
    QStringListModel *_includePathsModel;
    QStringListModel *_definesModel;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECT_H
