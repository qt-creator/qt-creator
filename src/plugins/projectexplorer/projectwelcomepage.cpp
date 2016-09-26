/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectwelcomepage.h"
#include "sessionmodel.h"
#include "projectexplorer.h"

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>

#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/algorithm.h>

#include <QQmlContext>
#include <QQmlEngine>

namespace ProjectExplorer {
namespace Internal {

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::recentProjectsChanged,
            this, &ProjectModel::resetProjects);
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return ProjectExplorerPlugin::recentProjects().count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    QPair<QString,QString> data = ProjectExplorerPlugin::recentProjects().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.second;
    case Qt::ToolTipRole:
        return data.first;
    case FilePathRole:
        return data.first;
    case PrettyFilePathRole:
        return Utils::withTildeHomePath(data.first);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{
        {Qt::DisplayRole, "displayName"},
        {FilePathRole, "filePath"},
        {PrettyFilePathRole, "prettyFilePath"}
    };

    return extraRoles;
}

void ProjectModel::resetProjects()
{
    beginResetModel();
    endResetModel();
}

///////////////////

void ProjectWelcomePage::facilitateQml(QQmlEngine *engine)
{
    m_sessionModel = new SessionModel(this);
    m_projectModel = new ProjectModel(this);

    QQmlContext *ctx = engine->rootContext();
    ctx->setContextProperty(QLatin1String("sessionList"), m_sessionModel);
    ctx->setContextProperty(QLatin1String("projectList"), m_projectModel);
    ctx->setContextProperty(QLatin1String("projectWelcomePage"), this);
}

QUrl ProjectWelcomePage::pageLocation() const
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    const QString resourcePath = Utils::FileUtils::normalizePathName(Core::ICore::resourcePath());
    return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/develop.qml"));
}

Core::Id ProjectWelcomePage::id() const
{
    return "Develop";
}

void ProjectWelcomePage::reloadWelcomeScreenData()
{
    if (m_sessionModel)
        m_sessionModel->resetSessions();
    if (m_projectModel)
        m_projectModel->resetProjects();
}

void ProjectWelcomePage::newProject()
{
    ProjectExplorerPlugin::openNewProjectDialog();
}

void ProjectWelcomePage::openProject()
{
     ProjectExplorerPlugin::openOpenProjectDialog();
}

} // namespace Internal
} // namespace ProjectExplorer
