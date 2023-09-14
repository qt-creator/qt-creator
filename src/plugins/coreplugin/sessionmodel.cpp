// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sessionmodel.h"

#include "actionmanager/actionmanager.h"
#include "session.h"
#include "sessiondialog.h"

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/stringutils.h>

#include <QFileInfo>
#include <QDir>

using namespace Utils;
using namespace Core::Internal;

namespace Core {

namespace PE {
struct Tr
{
    Q_DECLARE_TR_FUNCTIONS(QtC::ProjectExplorer)
};
} // namespace PE

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
            case 0:
                result = PE::Tr::tr("Session");
                break;
            case 1:
                result = PE::Tr::tr("Last Modified");
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

QStringList pathsToBaseNames(const FilePaths &paths)
{
    return Utils::transform(paths, &FilePath::completeBaseName);
}

QStringList pathsWithTildeHomePath(const FilePaths &paths)
{
    return Utils::transform(paths, &FilePath::withTildeHomePath);
}

QVariant SessionModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        QString sessionName = m_sortedSessions.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            switch (index.column()) {
            case 0:
                return sessionName;
            case 1:
                return SessionManager::sessionDateTime(sessionName);
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
            return font;
        }
        case DefaultSessionRole:
            return SessionManager::isDefaultSession(sessionName);
        case LastSessionRole:
            return SessionManager::lastSession() == sessionName;
        case ActiveSessionRole:
            return SessionManager::activeSession() == sessionName;
        case ShortcutRole: {
            const Id sessionBase = SESSION_BASE_ID;
            if (Command *cmd = ActionManager::command(sessionBase.withSuffix(index.row() + 1)))
                return cmd->keySequence().toString(QKeySequence::NativeText);
        } break;
        } // switch (role)
    }
    return {};
}

QHash<int, QByteArray> SessionModel::roleNames() const
{
    static const QHash<int, QByteArray> extraRoles{{Qt::DisplayRole, "sessionName"},
                                                   {DefaultSessionRole, "defaultSession"},
                                                   {ActiveSessionRole, "activeSession"},
                                                   {LastSessionRole, "lastSession"}};
    QHash<int, QByteArray> roles = QAbstractTableModel::roleNames();
    Utils::addToHash(&roles, extraRoles);
    return roles;
}

void SessionModel::sort(int column, Qt::SortOrder order)
{
    beginResetModel();
    const auto cmp = [column, order](const QString &s1, const QString &s2) {
        bool isLess;
        if (column == 0) {
            if (s1 == s2)
                return false;
            isLess = s1 < s2;
        }
        else {
            const auto s1time = SessionManager::sessionDateTime(s1);
            const auto s2time = SessionManager::sessionDateTime(s2);
            if (s1time == s2time)
                return false;
            isLess = s1time < s2time;
        }
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
    sessionInputDialog.setWindowTitle(PE::Tr::tr("New Session Name"));
    sessionInputDialog.setActionText(PE::Tr::tr("&Create"), PE::Tr::tr("Create and &Open"));

    runSessionNameInputDialog(&sessionInputDialog, [](const QString &newName) {
        SessionManager::createSession(newName);
    });
}

void SessionModel::cloneSession(QWidget *parent, const QString &session)
{
    SessionNameInputDialog sessionInputDialog(parent);
    sessionInputDialog.setWindowTitle(PE::Tr::tr("New Session Name"));
    sessionInputDialog.setActionText(PE::Tr::tr("&Clone"), PE::Tr::tr("Clone and &Open"));
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
    sessionInputDialog.setWindowTitle(PE::Tr::tr("Rename Session"));
    sessionInputDialog.setActionText(PE::Tr::tr("&Rename"), PE::Tr::tr("Rename and &Open"));
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

void SessionModel::runSessionNameInputDialog(SessionNameInputDialog *sessionInputDialog,
                                             std::function<void(const QString &)> createSession)
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

} // namespace Core
