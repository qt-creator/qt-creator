/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "projectwelcomepage.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QFileInfo>
#include <QDir>

#include <coreplugin/icore.h>
#include <coreplugin/iwizardfactory.h>
#include <projectexplorer/session.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/sessiondialog.h>

#include <utils/fileutils.h>
#include <utils/stringutils.h>
#include <utils/algorithm.h>

namespace ProjectExplorer {
namespace Internal {

SessionModel::SessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(SessionManager::instance(), SIGNAL(sessionLoaded(QString)), SLOT(resetSessions()));
}

int SessionModel::rowCount(const QModelIndex &) const
{
    return SessionManager::sessions().count();
}

QStringList pathsToBaseNames(const QStringList &paths)
{
    return Utils::transform(paths, [](const QString &path) {
        return QFileInfo(path).completeBaseName();
    });
}



QStringList pathsWithTildeHomePath(const QStringList &paths)
{
    return Utils::transform(paths, [](const QString &path) {
        return Utils::withTildeHomePath(QDir::toNativeSeparators(path));
    });
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole || role == DefaultSessionRole ||
            role == LastSessionRole || role == ActiveSessionRole || role == ProjectsPathRole  || role == ProjectsDisplayRole) {
        QString sessionName = SessionManager::sessions().at(index.row());
        if (role == Qt::DisplayRole)
            return sessionName;
        else if (role == DefaultSessionRole)
            return SessionManager::isDefaultSession(sessionName);
        else if (role == LastSessionRole)
            return SessionManager::lastSession() == sessionName;
        else if (role == ActiveSessionRole)
            return SessionManager::activeSession() == sessionName;
        else if (role == ProjectsPathRole)
            return pathsWithTildeHomePath(SessionManager::projectsForSessionName(sessionName));
        else if (role == ProjectsDisplayRole)
            return pathsToBaseNames(SessionManager::projectsForSessionName(sessionName));
    }
    return QVariant();
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "sessionName";
    roleNames[DefaultSessionRole] = "defaultSession";
    roleNames[ActiveSessionRole] = "activeSession";
    roleNames[LastSessionRole] = "lastSession";
    roleNames[ProjectsPathRole] = "projectsPath";
    roleNames[ProjectsDisplayRole] = "projectsName";
    return roleNames;
}

bool SessionModel::isDefaultVirgin() const
{
    return SessionManager::isDefaultVirgin();
}

void SessionModel::resetSessions()
{
    beginResetModel();
    endResetModel();
}

void SessionModel::cloneSession(const QString &session)
{
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), 0);
    newSessionInputDialog.setWindowTitle(tr("New session name"));
    newSessionInputDialog.setValue(session + QLatin1String(" (2)"));

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || SessionManager::sessions().contains(newSession))
            return;
        beginResetModel();
        SessionManager::cloneSession(session, newSession);
        endResetModel();

        if (newSessionInputDialog.isSwitchToRequested())
            SessionManager::loadSession(newSession);
    }
}

void SessionModel::deleteSession(const QString &session)
{
    if (!SessionManager::confirmSessionDelete(session))
        return;
    beginResetModel();
    SessionManager::deleteSession(session);
    endResetModel();
}

void SessionModel::renameSession(const QString &session)
{
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), 0);
    newSessionInputDialog.setWindowTitle(tr("New session name"));
    newSessionInputDialog.setValue(session);

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || SessionManager::sessions().contains(newSession))
            return;
        beginResetModel();
        SessionManager::renameSession(session, newSession);
        endResetModel();

        if (newSessionInputDialog.isSwitchToRequested())
            SessionManager::loadSession(newSession);
    }
}

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

QHash<int, QByteArray> ProjectModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[Qt::DisplayRole] = "displayName";
    roleNames[FilePathRole] = "filePath";
    roleNames[PrettyFilePathRole] = "prettyFilePath";
    return roleNames;
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
    Core::ICore::showNewItemDialog(tr("New Project"),
                                   Core::IWizardFactory::wizardFactoriesOfKind(Core::IWizardFactory::ProjectWizard));
}

void ProjectWelcomePage::openProject()
{
     ProjectExplorerPlugin::openOpenProjectDialog();
}

} // namespace Internal
} // namespace ProjectExplorer
