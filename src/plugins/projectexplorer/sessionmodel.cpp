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

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/id.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QFileInfo>
#include <QDir>

using namespace Core;

namespace ProjectExplorer {
namespace Internal {

SessionModel::SessionModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &SessionModel::resetSessions);
}

int SessionModel::indexOfSession(const QString &session)
{
    return SessionManager::sessions().indexOf(session);
}

QString SessionModel::sessionAt(int row)
{
    return SessionManager::sessions().value(row, QString());
}

QVariant SessionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    QVariant result;
    if (orientation == Qt::Horizontal) {
        switch (role) {
        case Qt::DisplayRole:
            switch (section) {
            case 0: result = tr("Session");
                break;
            case 1: result = tr("Last Modified");
                break;
            } // switch (section)
            break;
        } // switch (role) {
    }
    return result;
}

int SessionModel::columnCount(const QModelIndex &) const
{
    static int sectionCount = 0;
    if (sectionCount == 0) {
        // headers sections defining possible columns
        while (!headerData(sectionCount, Qt::Horizontal, Qt::DisplayRole).isNull())
            sectionCount++;
    }

    return sectionCount;
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
            case 1: result = SessionManager::sessionDateTime(sessionName);
                break;
            } // switch (section)
            break;
        case Qt::FontRole: {
            QFont font;
            if (SessionManager::isDefaultSession(sessionName))
                font.setItalic(true);
            else
                font.setItalic(false);
            if (SessionManager::activeSession() == sessionName && !SessionManager::isDefaultVirgin())
                font.setBold(true);
            else
                font.setBold(false);
            result = font;
        } break;
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
        case ShortcutRole: {
            const Id sessionBase = SESSION_BASE_ID;
            if (Command *cmd = ActionManager::command(sessionBase.withSuffix(index.row() + 1)))
                result = cmd->keySequence().toString(QKeySequence::NativeText);
        } break;
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
    return QAbstractTableModel::roleNames().unite(extraRoles);
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

void SessionModel::newSession()
{
    runNewSessionDialog("", [](const QString &newName) {
        SessionManager::createSession(newName);
    });
}

void SessionModel::cloneSession(const QString &session)
{
    runNewSessionDialog(session + " (2)", [session](const QString &newName) {
        SessionManager::cloneSession(session, newName);
    });
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
    runNewSessionDialog(session, [session](const QString &newName) {
        SessionManager::renameSession(session, newName);
    });
}

void SessionModel::switchToSession(const QString &session)
{
    SessionManager::loadSession(session);
    emit sessionSwitched();
}

void SessionModel::runNewSessionDialog(const QString &suggestedName, std::function<void(const QString &)> createSession)
{
    SessionNameInputDialog newSessionInputDialog(SessionManager::sessions(), nullptr);
    newSessionInputDialog.setWindowTitle(tr("New Session Name"));
    newSessionInputDialog.setValue(suggestedName);

    if (newSessionInputDialog.exec() == QDialog::Accepted) {
        QString newSession = newSessionInputDialog.value();
        if (newSession.isEmpty() || SessionManager::sessions().contains(newSession))
            return;
        beginResetModel();
        createSession(newSession);
        endResetModel();

        if (newSessionInputDialog.isSwitchToRequested())
            switchToSession(newSession);
        emit sessionCreated(newSession);
    }
}

} // namespace Internal
} // namespace ProjectExplorer
