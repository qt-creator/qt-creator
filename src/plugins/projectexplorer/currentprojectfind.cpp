// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfind.h"

#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projecttree.h"

#include <utils/qtcassert.h>
#include <utils/filesearch.h>

#include <QDebug>
#include <QSettings>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;
using namespace Utils;

CurrentProjectFind::CurrentProjectFind()
{
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &CurrentProjectFind::handleProjectChanged);
    connect(ProjectManager::instance(), &ProjectManager::projectDisplayNameChanged,
            this, [this](ProjectExplorer::Project *p) {
        if (p == ProjectTree::currentProject())
            emit displayNameChanged();
    });
}

QString CurrentProjectFind::id() const
{
    return QLatin1String("Current Project");
}

QString CurrentProjectFind::displayName() const
{
    Project *p = ProjectTree::currentProject();
    if (p)
        return Tr::tr("Project \"%1\"").arg(p->displayName());
    else
        return Tr::tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return ProjectTree::currentProject() != nullptr && BaseFileFind::isEnabled();
}

QVariant CurrentProjectFind::additionalParameters() const
{
    Project *project = ProjectTree::currentProject();
    if (project)
        return project->projectFilePath().toVariant();
    return QVariant();
}

static FilePath currentProjectFilePath()
{
    Project *project = ProjectTree::currentProject();
    return project ? project->projectFilePath() : FilePath();
}

FileContainerProvider CurrentProjectFind::fileContainerProvider() const
{
    return [nameFilters = fileNameFilters(), exclusionFilters = fileExclusionFilters(),
            projectFile = currentProjectFilePath()] {
        for (Project *project : ProjectManager::projects()) {
            if (project && projectFile == project->projectFilePath())
                return filesForProjects(nameFilters, exclusionFilters, {project});
        }
        return FileContainer();
    };
}

QString CurrentProjectFind::label() const
{
    Project *p = ProjectTree::currentProject();
    QTC_ASSERT(p, return QString());
    return Tr::tr("Project \"%1\":").arg(p->displayName());
}

void CurrentProjectFind::handleProjectChanged()
{
    emit enabledChanged(isEnabled());
    emit displayNameChanged();
}

void CurrentProjectFind::setupSearch(Core::SearchResult *search)
{
    const FilePath projectFile = currentProjectFilePath();
    connect(this, &IFindFilter::enabledChanged, search, [search, projectFile] {
        const QList<Project *> projects = ProjectManager::projects();
        for (Project *project : projects) {
            if (projectFile == project->projectFilePath()) {
                search->setSearchAgainEnabled(true);
                return;
            }
        }
        search->setSearchAgainEnabled(false);
    });
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}
