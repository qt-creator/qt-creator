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

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QFileInfo>
#include <QDir>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

SessionModel::SessionModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    m_sortedSessions = SessionManager::sessions();
    connect(SessionManager::instance(), &SessionManager::sessionLoaded,
            this, &SessionModel::resetSessions);
}

int SessionModel::indexOfSession(const QString &session)
{
    return m_sortedSessions.indexOf(session);
}

QString SessionModel::sessionAt(int row) const
{
    return m_sortedSessions.value(row, QString());
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
    return m_sortedSessions.count();
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
        QString sessionName = m_sortedSessions.at(index.row());

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
    static const QHash<int, QByteArray> extraRoles{
        {Qt::DisplayRole, "sessionName"},
        {DefaultSessionRole, "defaultSession"},
        {ActiveSessionRole, "activeSession"},
        {LastSessionRole, "lastSession"},
        {ProjectsPathRole, "projectsPath"},
        {ProjectsDisplayRole, "projectsName"}
    };
    QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
    Utils::addToHash(&roles, extraRoles);
    return roles;
}

void SessionModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();
    const auto cmp = [column, order](const QString &s1, const QString &s2) {
        bool isLess;
        if (column == 0)
            isLess = s1 < s2;
        else
            isLess = SessionManager::sessionDateTime(s1) < SessionManager::sessionDateTime(s2);
        if (order == Qt::DescendingOrder)
            isLess = !isLess;
        return isLess;
    };
    Utils::sort(m_sortedSessions, cmp);
    m_currentSortColumn = column;
    m_currentSortOrder = order;
    endResetModel();
}

bool SessionModel::isDefaultVirgin() const
{
    return SessionManager::isDefaultVirgin();
}

void SessionModel::resetSessions()
{
    beginResetModel();
    m_sortedSessions = SessionManager::sessions();
    endResetModel();
}

void SessionModel::newSession(QWidget *parent)
{
    SessionNameInputDialog sessionInputDialog(parent);
    sessionInputDialog.setWindowTitle(tr("New Session Name"));
    sessionInputDialog.setActionText(tr("&Create"), tr("Create and &Open"));

    runSessionNameInputDialog(&sessionInputDialog, [](const QString &newName) {
        SessionManager::createSession(newName);
    });
}

void SessionModel::cloneSession(QWidget *parent, const QString &session)
{
    SessionNameInputDialog sessionInputDialog(parent);
    sessionInputDialog.setWindowTitle(tr("New Session Name"));
    sessionInputDialog.setActionText(tr("&Clone"), tr("Clone and &Open"));
    sessionInputDialog.setValue(session + " (2)");

    runSessionNameInputDialog(&sessionInputDialog, [session](const QString &newName) {
        SessionManager::cloneSession(session, newName);
    });
}

void SessionModel::deleteSessions(const QStringList &sessions)
{
    if (!SessionManager::confirmSessionDelete(sessions))
        return;
    beginResetModel();
    SessionManager::deleteSessions(sessions);
    m_sortedSessions = SessionManager::sessions();
    sort(m_currentSortColumn, m_currentSortOrder);
    endResetModel();
}

void SessionModel::renameSession(QWidget *parent, const QString &session)
{
    SessionNameInputDialog sessionInputDialog(parent);
    sessionInputDialog.setWindowTitle(tr("Rename Session"));
    sessionInputDialog.setActionText(tr("&Rename"), tr("Rename and &Open"));
    sessionInputDialog.setValue(session);

    runSessionNameInputDialog(&sessionInputDialog, [session](const QString &newName) {
        SessionManager::renameSession(session, newName);
    });
}

void SessionModel::switchToSession(const QString &session)
{
    SessionManager::loadSession(session);
    emit sessionSwitched();
}

void SessionModel::runSessionNameInputDialog(SessionNameInputDialog *sessionInputDialog, std::function<void(const QString &)> createSession)
{
    if (sessionInputDialog->exec() == QDialog::Accepted) {
        QString newSession = sessionInputDialog->value();
        if (newSession.isEmpty() || SessionManager::sessions().contains(newSession))
            return;
        beginResetModel();
        createSession(newSession);
        m_sortedSessions = SessionManager::sessions();
        endResetModel();
        sort(m_currentSortColumn, m_currentSortOrder);

        if (sessionInputDialog->isSwitchToRequested())
            switchToSession(newSession);
        emit sessionCreated(newSession);
    }
}

} // namespace Internal
} // namespace ProjectExplorer
