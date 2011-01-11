/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef GENERICPROJECT_H
#define GENERICPROJECT_H

#include "genericprojectmanager.h"
#include "genericprojectnodes.h"
#include "generictarget.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/toolchaintype.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/ifile.h>

#include <QtCore/QFuture>

namespace Utils {
class PathChooser;
}

namespace ProjectExplorer {
class ToolChain;
}

namespace GenericProjectManager {
namespace Internal {
class GenericBuildConfiguration;
class GenericProject;
class GenericTarget;
class GenericMakeStep;
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

    QString displayName() const;
    QString id() const;
    Core::IFile *file() const;
    ProjectExplorer::IProjectManager *projectManager() const;
    GenericTarget *activeTarget() const;

    QList<ProjectExplorer::Project *> dependsOn();

    QList<ProjectExplorer::BuildConfigWidget*> subConfigWidgets();

    GenericProjectNode *rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

    QStringList buildTargets() const;
    ProjectExplorer::ToolChain *toolChain() const;

    bool addFiles(const QStringList &filePaths);
    bool removeFiles(const QStringList &filePaths);

    enum RefreshOptions {
        Files         = 0x01,
        Configuration = 0x02,
        Everything    = Files | Configuration
    };

    void refresh(RefreshOptions options);

    QStringList includePaths() const;
    void setIncludePaths(const QStringList &includePaths);

    QByteArray defines() const;
    QStringList allIncludePaths() const;
    QStringList projectIncludePaths() const;
    QStringList files() const;
    QStringList generated() const;
    ProjectExplorer::ToolChainType toolChainType() const;
    void setToolChainType(ProjectExplorer::ToolChainType type);

    QVariantMap toMap() const;

protected:
    virtual bool fromMap(const QVariantMap &map);

private:
    bool saveRawFileList(const QStringList &rawFileList);
    void parseProject(RefreshOptions options);
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = 0) const;

    Manager *m_manager;
    QString m_fileName;
    QString m_filesFileName;
    QString m_includesFileName;
    QString m_configFileName;
    GenericProjectFile *m_file;
    QString m_projectName;

    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
    QStringList m_generated;
    QStringList m_includePaths;
    QStringList m_projectIncludePaths;
    QByteArray m_defines;

    GenericProjectNode *m_rootNode;
    ProjectExplorer::ToolChain *m_toolChain;
    ProjectExplorer::ToolChainType m_toolChainType;
    QFuture<void> m_codeModelFuture;
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
    virtual void rename(const QString &newName);

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const;
    void reload(ReloadFlag flag, ChangeType type);

private:
    GenericProject *m_project;
    QString m_fileName;
};

class GenericBuildSettingsWidget : public ProjectExplorer::BuildConfigWidget
{
    Q_OBJECT

public:
    GenericBuildSettingsWidget(GenericTarget *target);
    virtual ~GenericBuildSettingsWidget();

    virtual QString displayName() const;

    virtual void init(ProjectExplorer::BuildConfiguration *bc);

private Q_SLOTS:
    void buildDirectoryChanged();
    void toolChainSelected(int index);

private:
    GenericTarget *m_target;
    Utils::PathChooser *m_pathChooser;
    GenericBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECT_H
