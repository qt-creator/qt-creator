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

// Qt Creator
#include <app/app_version.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <utils/fileutils.h>
// Qt
#include <QString>
#include <QFuture>
#include <QFutureInterface>

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

#if defined(IDE_VERSION_MAJOR) && (IDE_VERSION_MAJOR == 3 && IDE_VERSION_MINOR == 0)
    Core::Id id() const;
#endif
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

    // Deserializes all project data from the map object
    // Calls the base ProjectExplorer::Project::fromMap function first.
    bool fromMap(QVariantMap const& map);

private:

    void setProjectName(QString const& name);

    // Corresponding project manager passed to the constructor
    ProjectManager* manager_;

    // By default, name of directory with Jamfile.
    // Boost.Build treats each Jamfile is a separate project,
    // where hierarchy of Jamfiles makes hierarchy of projects.
    QString projectName_;

    // Jamfile passed to the constructor (Jamroot, Jamfile, Jamfile.v2).
    QString filePath_;

    // Auxiliary file Jamfile.${JAMFILE_FILES_EXT} with list of source files.
    // Role of this file is similar to the .files file in the GenericProjectManager,
    // hence managing of this file is implemented in very similar manner.
    QString filesFilePath_;
    QStringList files_;
    QStringList filesRaw_;
    QHash<QString, QString> entriesRaw_;

    // Auxiliary file Jamfile.${JAMFILE_INCLUDES_EXT} with list of source files.
    QString includesFilePath_;

    ProjectFile* projectFile_;
    ProjectNode* projectNode_;

    QFuture<void> cppModelFuture_;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBPROJECT_HPP
