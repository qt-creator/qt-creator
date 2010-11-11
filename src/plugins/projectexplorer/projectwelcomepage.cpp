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

#include <coreplugin/multifeedrssmodel.h>

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
    roleNames[CurrentSessionRole] = "currentSession";
    setRoleNames(roleNames);
    connect(manager, SIGNAL(sessionLoaded()), SLOT(resetSessions()));
}

int SessionModel::rowCount(const QModelIndex &) const
{
    return qMin(m_manager->sessions().count(), 12);
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == DefaultSessionRole || role == CurrentSessionRole) {
        QString sessionName = m_manager->sessions().at(index.row());
        if (role == Qt::DisplayRole)
            return sessionName;
        else if (role == DefaultSessionRole)
            return m_manager->isDefaultSession(sessionName);
        else if (role == CurrentSessionRole)
            return sessionName == m_manager->currentSession();
    } else
        return QVariant();
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
    return qMin(m_plugin->recentProjects().count(), 6);
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

ProjectWelcomePage::ProjectWelcomePage()
{
}

void ProjectWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    static const char feedGroupName[] = "Feeds";

    QDeclarativeContext *ctx = engine->rootContext();
    ProjectExplorerPlugin *pePlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    ctx->setContextProperty("sessionList", new SessionModel(pePlugin->session(), this));
    ctx->setContextProperty("projectList", new ProjectModel(pePlugin, this));
    Core::MultiFeedRssModel *rssModel = new Core::MultiFeedRssModel(this);
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings->childGroups().contains(feedGroupName)) {
        int size = settings->beginReadArray(feedGroupName);
        for (int i = 0; i < size; ++i)
        {
            settings->setArrayIndex(i);
            rssModel->addFeed(settings->value("url").toString());
        }
        settings->endArray();
    } else {
        rssModel->addFeed(QLatin1String("http://labs.trolltech.com/blogs/feed"));
        rssModel->addFeed(QLatin1String("http://feeds.feedburner.com/TheQtBlog?format=xml"));
    }

    ctx->setContextProperty("aggregatedFeedsModel", rssModel);
    ctx->setContextProperty("projectWelcomePage", this);
}

void ProjectWelcomePage::setWelcomePageData(const WelcomePageData &welcomePageData)
{
    m_welcomePageData = welcomePageData;
}

} // namespace Internal
} // namespace ProjectExplorer
