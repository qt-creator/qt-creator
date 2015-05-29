//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBPROJECTNODE_HPP
#define BBPROJECTNODE_HPP

#include <projectexplorer/projectnodes.h>

#include <QFutureInterface>
#include <QList>
#include <QSet>
#include <QString>
#include <QStringList>

namespace Core { class IDocument; }

namespace ProjectExplorer { class RunConfiguration; }

namespace BoostBuildProjectManager {
namespace Internal {

class Project;

// An in-memory presentation of a Project.
// Represents a file or a folder of the project tree.
// No special operations (addFiles(), removeFiles(), renameFile(), etc.) are offered.
class ProjectNode : public ProjectExplorer::ProjectNode
{
public:
    ProjectNode(Project* project, Core::IDocument* projectFile);

    bool hasBuildTargets() const;
    QList<ProjectExplorer::ProjectAction> supportedActions(Node* node) const;
    bool canAddSubProject(QString const& filePath) const;
    bool addSubProjects(QStringList const& filePaths);
    bool removeSubProjects(QStringList const& filePaths);
    bool addFiles(QStringList const& filePaths, QStringList* notAdded /*= 0*/);
    bool removeFiles(QStringList const& filePaths, QStringList* notRemoved /*= 0*/);
    bool deleteFiles(QStringList const& filePaths);
    bool renameFile(QString const& filePath, QString const& newFilePath);
    QList<ProjectExplorer::RunConfiguration*> runConfigurationsFor(Node* node);

    void refresh(QSet<QString> oldFileList);

private:
    ProjectExplorer::FolderNode*
    createFolderByName(QStringList const& components, int end);

    ProjectExplorer::FolderNode*
    findFolderByName(QStringList const& components, int end) const;

    void removeEmptySubFolders(FolderNode* parent, FolderNode* subParent);

    Project *m_project;
    Core::IDocument *m_projectFile;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBPROJECTNODE_HPP
