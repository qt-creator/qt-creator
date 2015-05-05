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
#ifndef BBPROJECTFILE_HPP
#define BBPROJECTFILE_HPP

#include <coreplugin/idocument.h>

namespace BoostBuildProjectManager {
namespace Internal {

class Project;

// Describes the root of a project for use in Project class.
// In the context of Boost.Build the implementation is mostly empty,
// as the modification of a project is done by editing Jamfile.v2 files.
class ProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    ProjectFile(Project* project, QString const& fileName);

    bool save(QString* errorString, QString const& fileName, bool autoSave);
    QString defaultPath() const;
    QString suggestedFileName() const;
    QString mimeType() const;
    bool isModified() const;
    bool isSaveAsAllowed() const;
    bool reload(QString* errorString, ReloadFlag flag, ChangeType type);

private:
    Project* project_;
};

} // namespace Internal
} // namespace AutotoolsProjectManager

#endif // BBPROJECTFILE_HPP
