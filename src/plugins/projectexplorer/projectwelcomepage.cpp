/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectwelcomepage.h"

#include "projectexplorerconstants.h"

#include <utils/stringutils.h>

#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QFileInfo>
#include <QDir>

#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <sessiondialog.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

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
    roleNames[ProjectsPathRole] = "projectsPath";
    roleNames[ProjectsDisplayRole] = "projectsName";
    setRoleNames(roleNames);
    connect(manager, SIGNAL(sessionLoaded(QString)), SLOT(resetSessions()));
}

int SessionModel::rowCount(const QModelIndex &) const
{
    return m_manager->sessions().count();
}

QStringList pathsToBaseNames(const QStringList &paths)
{
    QStringList stringList;
    foreach (const QString &path, paths)
        stringList.append(QFileInfo(path).completeBaseName());
    return stringList;
}



QStringList pathsWithTildeHomePath(const QStringList &paths)
{
    QStringList stringList;
    foreach (const QString &path, paths)
        stringList.append(Utils::withTildeHomePath(QDir::toNativeSeparators(path)));
    return stringList;
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == DefaultSessionRole ||
            role == LastSessionRole || role == ActiveSessionRole || role == ProjectsPathRole  || role == ProjectsDisplayRole) {
        QString sessionName = m_manager->sessions().at(index.row());
        if (role == Qt::DisplayRole)
            return sessionName;
        else if (role == DefaultSessionRole)
            return m_manager->isDefaultSession(sessionName);
        else if (role == LastSessionRole)
            return m_manager->lastSession() == sessionName;
        else if (role == ActiveSessionRole)
            return m_manager->activeSession() == sessionName;
        else if (role == ProjectsPathRole)
            return pathsWithTildeHomePath(m_manager->projectsForSessionName(sessionName));
        else if (role == ProjectsDisplayRole)
            return pathsToBaseNames(m_manager->projectsForSessionName(sessionName));
    }
    return QVariant();
}

bool SessionModel::isDefaultVirgin() const
{
    return m_manager->isDefaultVirgin();
}

void SessionModel::resetSessions()
{
    beginResetModel();
    endResetModel();
}

void SessionModel::cloneSession(const QString &session)
{
    SessionNameInputDialog newSessionInputDialog(m_manager->sessions(), 0);
    newSessionInputDialog.setWindowTitle(tr("New session name"));
    newSessionInputDialog.setValue(session + QLatin1String(" (2)"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || m_manager->sessions().contains(newSession))
            return;
        beginResetModel();
        m_manager->cloneSession(session, newSession);
        endResetModel();

        if (newSessionInputDialog.isSwitchToRequested())
            m_manager->loadSession(newSession);
    }
}

void SessionModel::deleteSession(const QString &session)
{
    beginResetModel();
    m_manager->deleteSession(session);
    endResetModel();
}

void SessionModel::renameSession(const QString &session)
{
    SessionNameInputDialog newSessionInputDialog(m_manager->sessions(), 0);
    newSessionInputDialog.setWindowTitle(tr("New session name"));
    newSessionInputDialog.setValue(session);

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || m_manager->sessions().contains(newSession))
            return;
        beginResetModel();
        m_manager->renameSession(session, newSession);
        endResetModel();

        if (newSessionInputDialog.isSwitchToRequested())
            m_manager->loadSession(newSession);
    }
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
    beginResetModel();
    endResetModel();
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
    ctx->setContextProperty(QLatin1String("sessionList"), m_sessionModel);
    ctx->setContextProperty(QLatin1String("projectList"), m_projectModel);
    ctx->setContextProperty(QLatin1String("projectWelcomePage"), this);
}

QUrl ProjectWelcomePage::pageLocation() const
{
    QString resourcePath = Core::ICore::resourcePath();
#ifdef Q_OS_WIN
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    resourcePath = Utils::normalizePathName(resourcePath);
#endif
     return QUrl::fromLocalFile(resourcePath + QLatin1String("/welcomescreen/develop.qml"));
}

ProjectWelcomePage::Id ProjectWelcomePage::id() const
{
    return Develop;
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
