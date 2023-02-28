// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "session.h"

#include "session_p.h"

#include "projectexplorerconstants.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QMessageBox>
#include <QPushButton>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

const char DEFAULT_SESSION[] = "default";
const char LAST_ACTIVE_TIMES_KEY[] = "LastActiveTimes";

/*!
     \class ProjectExplorer::SessionManager

     \brief The SessionManager class manages sessions.

     TODO the interface of this class is not really great.
     The implementation suffers from that all the functions from the
     public interface just wrap around functions which do the actual work.
     This could be improved.
*/

static SessionManager *m_instance = nullptr;
SessionManagerPrivate *sb_d = nullptr;

SessionManager::SessionManager()
{
    m_instance = this;
    sb_d = new SessionManagerPrivate;

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &SessionManager::saveActiveMode);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, [] {
        QVariantMap times;
        for (auto it = sb_d->m_lastActiveTimes.cbegin(); it != sb_d->m_lastActiveTimes.cend(); ++it)
            times.insert(it.key(), it.value());
        ICore::settings()->setValue(LAST_ACTIVE_TIMES_KEY, times);
    });

    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &SessionManager::markSessionFileDirty);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &SessionManager::markSessionFileDirty);
}

SessionManager::~SessionManager()
{
    emit m_instance->aboutToUnloadSession(sb_d->m_sessionName);
    delete sb_d->m_writer;
    delete sb_d;
    sb_d = nullptr;
}

SessionManager *SessionManager::instance()
{
   return m_instance;
}

bool SessionManager::isDefaultVirgin()
{
    return isDefaultSession(sb_d->m_sessionName) && sb_d->m_virginSession;
}

bool SessionManager::isDefaultSession(const QString &session)
{
    return session == QLatin1String(DEFAULT_SESSION);
}

void SessionManager::saveActiveMode(Id mode)
{
    if (mode != Core::Constants::MODE_WELCOME)
        setValue(QLatin1String("ActiveMode"), mode.toString());
}

bool SessionManager::loadingSession()
{
    return sb_d->m_loadingSession;
}

/*!
    Lets other plugins store persistent values within the session file.
*/

void SessionManager::setValue(const QString &name, const QVariant &value)
{
    if (sb_d->m_values.value(name) == value)
        return;
    sb_d->m_values.insert(name, value);
}

QVariant SessionManager::value(const QString &name)
{
    auto it = sb_d->m_values.constFind(name);
    return (it == sb_d->m_values.constEnd()) ? QVariant() : *it;
}

QString SessionManager::activeSession()
{
    return sb_d->m_sessionName;
}

QStringList SessionManager::sessions()
{
    if (sb_d->m_sessions.isEmpty()) {
        // We are not initialized yet, so do that now
        const FilePaths sessionFiles =
                ICore::userResourcePath().dirEntries({{"*qws"}}, QDir::Time | QDir::Reversed);
        const QVariantMap lastActiveTimes = ICore::settings()->value(LAST_ACTIVE_TIMES_KEY).toMap();
        for (const FilePath &file : sessionFiles) {
            const QString &name = file.completeBaseName();
            sb_d->m_sessionDateTimes.insert(name, file.lastModified());
            const auto lastActiveTime = lastActiveTimes.find(name);
            sb_d->m_lastActiveTimes.insert(name, lastActiveTime != lastActiveTimes.end()
                    ? lastActiveTime->toDateTime()
                    : file.lastModified());
            if (name != QLatin1String(DEFAULT_SESSION))
                sb_d->m_sessions << name;
        }
        sb_d->m_sessions.prepend(QLatin1String(DEFAULT_SESSION));
    }
    return sb_d->m_sessions;
}

QDateTime SessionManager::sessionDateTime(const QString &session)
{
    return sb_d->m_sessionDateTimes.value(session);
}

QDateTime SessionManager::lastActiveTime(const QString &session)
{
    return sb_d->m_lastActiveTimes.value(session);
}

FilePath SessionManager::sessionNameToFileName(const QString &session)
{
    return ICore::userResourcePath(session + ".qws");
}

/*!
    Creates \a session, but does not actually create the file.
*/

bool SessionManager::createSession(const QString &session)
{
    if (sessions().contains(session))
        return false;
    Q_ASSERT(sb_d->m_sessions.size() > 0);
    sb_d->m_sessions.insert(1, session);
    sb_d->m_lastActiveTimes.insert(session, QDateTime::currentDateTime());
    return true;
}

bool SessionManager::renameSession(const QString &original, const QString &newName)
{
    if (!cloneSession(original, newName))
        return false;
    if (original == activeSession())
        ProjectManager::loadSession(newName);
    emit instance()->sessionRenamed(original, newName);
    return deleteSession(original);
}


/*!
    \brief Shows a dialog asking the user to confirm deleting the session \p session
*/
bool SessionManager::confirmSessionDelete(const QStringList &sessions)
{
    const QString title = sessions.size() == 1 ? Tr::tr("Delete Session") : Tr::tr("Delete Sessions");
    const QString question = sessions.size() == 1
            ? Tr::tr("Delete session %1?").arg(sessions.first())
            : Tr::tr("Delete these sessions?\n    %1").arg(sessions.join("\n    "));
    return QMessageBox::question(ICore::dialogParent(),
                                 title,
                                 question,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

/*!
     Deletes \a session name from session list and the file from disk.
*/
bool SessionManager::deleteSession(const QString &session)
{
    if (!sb_d->m_sessions.contains(session))
        return false;
    sb_d->m_sessions.removeOne(session);
    sb_d->m_lastActiveTimes.remove(session);
    emit instance()->sessionRemoved(session);
    FilePath sessionFile = sessionNameToFileName(session);
    if (sessionFile.exists())
        return sessionFile.removeFile();
    return false;
}

void SessionManager::deleteSessions(const QStringList &sessions)
{
    for (const QString &session : sessions)
        deleteSession(session);
}

bool SessionManager::cloneSession(const QString &original, const QString &clone)
{
    if (!sb_d->m_sessions.contains(original))
        return false;

    FilePath sessionFile = sessionNameToFileName(original);
    // If the file does not exist, we can still clone
    if (!sessionFile.exists() || sessionFile.copyFile(sessionNameToFileName(clone))) {
        sb_d->m_sessions.insert(1, clone);
        sb_d->m_sessionDateTimes.insert(clone, sessionNameToFileName(clone).lastModified());
        return true;
    }
    return false;
}

void SessionManagerPrivate::restoreValues(const PersistentSettingsReader &reader)
{
    const QStringList keys = reader.restoreValue(QLatin1String("valueKeys")).toStringList();
    for (const QString &key : keys) {
        QVariant value = reader.restoreValue(QLatin1String("value-") + key);
        m_values.insert(key, value);
    }
}

void SessionManagerPrivate::restoreEditors(const PersistentSettingsReader &reader)
{
    const QVariant editorsettings = reader.restoreValue(QLatin1String("EditorSettings"));
    if (editorsettings.isValid()) {
        EditorManager::restoreState(QByteArray::fromBase64(editorsettings.toByteArray()));
        sessionLoadingProgress();
    }
}

/*!
    Returns the last session that was opened by the user.
*/
QString SessionManager::lastSession()
{
    return ICore::settings()->value(Constants::LASTSESSION_KEY).toString();
}

/*!
    Returns the session that was active when Qt Creator was last closed, if any.
*/
QString SessionManager::startupSession()
{
    return ICore::settings()->value(Constants::STARTUPSESSION_KEY).toString();
}

void SessionManager::markSessionFileDirty()
{
    sb_d->m_virginSession = false;
}

void SessionManagerPrivate::sessionLoadingProgress()
{
    m_future.setProgressValue(m_future.progressValue() + 1);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

} // namespace ProjectExplorer
