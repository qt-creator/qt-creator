// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfind.h"

#include "allprojectsfind.h"
#include "project.h"
#include "projectexplorertr.h"
#include "projectmanager.h"
#include "projecttree.h"

#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/shutdownguard.h>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ProjectExplorer::Internal {

class CurrentProjectFind final : public AllProjectsFind
{
public:
    CurrentProjectFind();

private:
    QString id() const final;
    QString displayName() const final;

    bool isEnabled() const final;

    Utils::Store save() const final;
    void restore(const Utils::Store &s) final;

    // deprecated
    QByteArray settingsKey() const final;

    QString label() const final;

    TextEditor::FileContainerProvider fileContainerProvider() const final;
    void handleProjectChanged();
    void setupSearch(Core::SearchResult *search) final;
};

CurrentProjectFind::CurrentProjectFind()
{
    connect(ProjectTree::instance(), &ProjectTree::currentProjectChanged,
            this, &CurrentProjectFind::handleProjectChanged);
    connect(ProjectManager::instance(), &ProjectManager::projectDisplayNameChanged,
            this, [this](Project *p) {
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

const char kDefaultInclusion[] = "*";
const char kDefaultExclusion[] = "";

Store CurrentProjectFind::save() const
{
    Store s;
    writeCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
    return s;
}

void CurrentProjectFind::restore(const Store &s)
{
    readCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
}

QByteArray CurrentProjectFind::settingsKey() const
{
    return "CurrentProjectFind";
}

void setupCurrentProjectFind()
{
    static GuardedObject<CurrentProjectFind> theCurrentProjectFind;
}

} // ProjectExplorer::Internal
