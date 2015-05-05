//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the  GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#include "b2projectmanager.h"
#include "b2projectmanagerconstants.h"
#include "b2project.h"
#include "b2utility.h"
// Qt Creator
#include <projectexplorer/iprojectmanager.h>
// Qt
#include <QFileInfo>
#include <QString>

namespace BoostBuildProjectManager {
namespace Internal {

ProjectManager::ProjectManager()
{
}

QString ProjectManager::mimeType() const
{
    BBPM_QDEBUG(Constants::MIMETYPE_PROJECT);

    return QLatin1String(Constants::MIMETYPE_PROJECT);
}

ProjectExplorer::Project*
ProjectManager::openProject(QString const& fileName, QString* errorString)
{
    BBPM_QDEBUG("opening project:" << fileName);

    if (!QFileInfo(fileName).isFile()) {
        if (errorString)
            *errorString = tr("Failed opening project \"%1\": Project is not a file.")
                .arg(fileName);
        return 0;
    }

    return new Project(this, fileName);
}

void ProjectManager::registerProject(Project* project)
{
    Q_ASSERT(project);
    projects_.append(project);
}

void ProjectManager::unregisterProject(Project* project)
{
    Q_ASSERT(project);
    projects_.removeAll(project);
}

} // namespace Internal
} // namespace BoostBuildProjectManager
