//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBPROJECT_HPP
#define BBPROJECT_HPP

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <utils/fileutils.h>

#include <QFuture>
#include <QString>

namespace BoostBuildProjectManager {
namespace Internal {

class ProjectFile;
class ProjectManager;
class ProjectNode;

// Represents a project node in the project explorer.
class Project : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    Project(ProjectManager* manager, QString const& fileName);
    ~Project();

    QString displayName() const;
    Core::IDocument* document() const;
    ProjectExplorer::IProjectManager* projectManager() const;
    ProjectExplorer::ProjectNode* rootProjectNode() const;
    QStringList files(FilesMode fileMode) const;

    bool needsConfiguration() const;

    void refresh();

    QString filesFilePath() const;
    QStringList files() const;

    QString includesFilePath() const;

    static QString defaultProjectName(QString const& fileName);
    static QString defaultBuildDirectory(QString const& top);
    static QString defaultWorkingDirectory(QString const& top);

protected:
    QVariantMap toMap() const;
    bool fromMap(QVariantMap const& map);

private:
    void setProjectName(QString const& name);

    // Corresponding project manager passed to the constructor
    ProjectManager *m_manager;

    // By default, name of directory with Jamfile.
    // Boost.Build treats each Jamfile is a separate project,
    // where hierarchy of Jamfiles makes hierarchy of projects.
    QString m_projectName;

    // Jamfile passed to the constructor (Jamroot, Jamfile, Jamfile.v2).
    QString m_filePath;

    // Auxiliary file Jamfile.${JAMFILE_FILES_EXT} with list of source files.
    // Role of this file is similar to the .files file in the GenericProjectManager,
    // hence managing of this file is implemented in very similar manner.
    QString m_filesFilePath;
    QStringList m_files;
    QStringList m_filesRaw;
    QHash<QString, QString> m_entriesRaw;

    // Auxiliary file Jamfile.${JAMFILE_INCLUDES_EXT} with list of source files.
    QString m_includesFilePath;

    ProjectFile *m_projectFile;
    ProjectNode *m_projectNode;

    QFuture<void> m_cppModelFuture;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBPROJECT_HPP
