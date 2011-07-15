/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "projectwelcomepage.h"

#include <utils/stringutils.h>

#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>

#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>

namespace ProjectExplorer {
namespace Internal {

SessionModel::SessionModel(SessionManager *manager, QObject *parent)
    : QAbstractListModel(parent), m_manager(manager)
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "sessionName";
    roleNames[DefaultSessionRole] = "defaultSession";
    roleNames[ActiveSessionRole] = "activeSession";
    roleNames[LastSessionRole] = "lastSession";
    setRoleNames(roleNames);
    connect(manager, SIGNAL(sessionLoaded()), SLOT(resetSessions()));
}

int SessionModel::rowCount(const QModelIndex &) const
{
    return m_manager->sessions().count();
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == DefaultSessionRole ||
            role == LastSessionRole || role == ActiveSessionRole) {
        QString sessionName = m_manager->sessions().at(index.row());
        if (role == Qt::DisplayRole)
            return sessionName;
        else if (role == DefaultSessionRole)
            return m_manager->isDefaultSession(sessionName);
        else if (role == LastSessionRole)
            return m_manager->lastSession() == sessionName;
        else if (role == ActiveSessionRole)
            return m_manager->activeSession() == sessionName;
    }
    return QVariant();
}

bool SessionModel::isDefaultVirgin() const
{
    return m_manager->isDefaultVirgin();
}

void SessionModel::resetSessions()
{
    reset();
}


ProjectModel::ProjectModel(ProjectExplorerPlugin *plugin, QObject *parent)
    : QAbstractListModel(parent), m_plugin(plugin)
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "displayName";
    roleNames[FilePathRole] = "filePath";
    roleNames[PrettyFilePathRole] = "prettyFilePath";
    setRoleNames(roleNames);
    connect(plugin, SIGNAL(recentProjectsChanged()), SLOT(resetProjects()));
}

int ProjectModel::rowCount(const QModelIndex &) const
{
    return m_plugin->recentProjects().count();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const
{
    QPair<QString,QString> data = m_plugin->recentProjects().at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return data.second;
        break;
    case FilePathRole:
        return data.first;
    case PrettyFilePathRole:
        return Utils::withTildeHomePath(data.first);
    default:
        return QVariant();
    }

    return QVariant();
}

void ProjectModel::resetProjects()
{
    reset();
}

///////////////////

ProjectWelcomePage::ProjectWelcomePage() :
    m_sessionModel(0), m_projectModel(0)
{
}

void ProjectWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    ProjectExplorerPlugin *pePlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    m_sessionModel = new SessionModel(pePlugin->session(), this);
    m_projectModel = new ProjectModel(pePlugin, this);

    QDeclarativeContext *ctx = engine->rootContext();
    ctx->setContextProperty("sessionList", m_sessionModel);
    ctx->setContextProperty("projectList", m_projectModel);
    ctx->setContextProperty("projectWelcomePage", this);
}

void ProjectWelcomePage::reloadWelcomeScreenData()
{
    if (m_sessionModel)
        m_sessionModel->resetSessions();
    if (m_projectModel)
        m_projectModel->resetProjects();
}

} // namespace Internal
} // namespace ProjectExplorer
