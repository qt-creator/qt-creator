// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "currentprojectfind.h"

#include "project.h"
#include "projecttree.h"
#include "session.h"

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
    connect(SessionManager::instance(), &SessionManager::projectDisplayNameChanged,
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
        return tr("Project \"%1\"").arg(p->displayName());
    else
        return tr("Current Project");
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

FileIterator *CurrentProjectFind::files(const QStringList &nameFilters,
                                        const QStringList &exclusionFilters,
                                        const QVariant &additionalParameters) const
{
    QTC_ASSERT(additionalParameters.isValid(),
               return new FileListIterator(FilePaths(), QList<QTextCodec *>()));
    const FilePath projectFile = FilePath::fromVariant(additionalParameters);
    for (Project *project : SessionManager::projects()) {
        if (project && projectFile == project->projectFilePath())
            return filesForProjects(nameFilters, exclusionFilters, {project});
    }
    return new FileListIterator(FilePaths(), QList<QTextCodec *>());
}

QString CurrentProjectFind::label() const
{
    Project *p = ProjectTree::currentProject();
    QTC_ASSERT(p, return QString());
    return tr("Project \"%1\":").arg(p->displayName());
}

void CurrentProjectFind::handleProjectChanged()
{
    emit enabledChanged(isEnabled());
    emit displayNameChanged();
}

void CurrentProjectFind::recheckEnabled(Core::SearchResult *search)
{
    const FilePath projectFile = FilePath::fromVariant(getAdditionalParameters(search));
    for (Project *project : SessionManager::projects()) {
        if (projectFile == project->projectFilePath()) {
            search->setSearchAgainEnabled(true);
            return;
        }
    }
    search->setSearchAgainEnabled(false);
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
