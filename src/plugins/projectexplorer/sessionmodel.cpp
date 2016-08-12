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

#include "sessionmodel.h"
#include "session.h"

#include "sessiondialog.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QFileInfo>
#include <QDir>

namespace ProjectExplorer {
namespace Internal {

SessionModel::SessionModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &SessionModel::resetSessions);
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
    QVariant result;
    if (index.isValid()) {
        QString sessionName = SessionManager::sessions().at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0: result = sessionName;
                break;
            } // switch (section)
            break;
        case DefaultSessionRole:
            result = SessionManager::isDefaultSession(sessionName);
            break;
        case LastSessionRole:
            result = SessionManager::lastSession() == sessionName;
            break;
        case ActiveSessionRole:
            result = SessionManager::activeSession() == sessionName;
            break;
        case ProjectsPathRole:
            result = pathsWithTildeHomePath(SessionManager::projectsForSessionName(sessionName));
            break;
        case ProjectsDisplayRole:
            result = pathsToBaseNames(SessionManager::projectsForSessionName(sessionName));
            break;
        } // switch (role)
    }

    return result;
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    static QHash<int, QByteArray> extraRoles{
        {Qt::DisplayRole, "sessionName"},
        {DefaultSessionRole, "defaultSession"},
        {ActiveSessionRole, "activeSession"},
        {LastSessionRole, "lastSession"},
        {ProjectsPathRole, "projectsPath"},
        {ProjectsDisplayRole, "projectsName"}
    };
    return QAbstractListModel::roleNames().unite(extraRoles);
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
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), nullptr);
    newSessionInputDialog.setWindowTitle(tr("New Session Name"));
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
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), nullptr);
    newSessionInputDialog.setWindowTitle(tr("New Session Name"));
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
} // namespace Internal
} // namespace ProjectExplorer
