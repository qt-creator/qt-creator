// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "session.h"

#include "session_p.h"

#include "projectexplorer.h"
#include "projectexplorertr.h"
#include "projectmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

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
#include <QTimer>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {

const char DEFAULT_SESSION[] = "default";
const char LAST_ACTIVE_TIMES_KEY[] = "LastActiveTimes";
const char STARTUPSESSION_KEY[] = "ProjectExplorer/SessionToRestore";
const char LASTSESSION_KEY[] = "ProjectExplorer/StartupSession";
const char AUTO_RESTORE_SESSION_SETTINGS_KEY[] = "ProjectExplorer/Settings/AutoRestoreLastSession";
static bool kIsAutoRestoreLastSessionDefault = false;

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

    connect(ICore::instance(), &ICore::coreOpened, this, [] { sb_d->restoreStartupSession(); });

    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &SessionManager::saveActiveMode);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, [] {
        QtcSettings *s = ICore::settings();
        QVariantMap times;
        for (auto it = sb_d->m_lastActiveTimes.cbegin(); it != sb_d->m_lastActiveTimes.cend(); ++it)
            times.insert(it.key(), it.value());
        s->setValue(LAST_ACTIVE_TIMES_KEY, times);
        if (SessionManager::isDefaultVirgin()) {
            s->remove(STARTUPSESSION_KEY);
        } else {
            s->setValue(STARTUPSESSION_KEY, SessionManager::activeSession());
            s->setValue(LASTSESSION_KEY, SessionManager::activeSession());
        }
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

void SessionManager::setSessionValue(const QString &name, const QVariant &value)
{
    sb_d->m_sessionValues.insert(name, value);
}

QVariant SessionManager::sessionValue(const QString &name, const QVariant &defaultValue)
{
    auto it = sb_d->m_sessionValues.constFind(name);
    return (it == sb_d->m_sessionValues.constEnd()) ? defaultValue : *it;
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

static QString determineSessionToRestoreAtStartup()
{
    // TODO (session) move argument to core
    // Process command line arguments first:
    const bool lastSessionArg = ExtensionSystem::PluginManager::specForPlugin(
                                    ProjectExplorerPlugin::instance())
                                    ->arguments()
                                    .contains("-lastsession");
    if (lastSessionArg && !SessionManager::startupSession().isEmpty())
        return SessionManager::startupSession();
    const QStringList arguments = ExtensionSystem::PluginManager::arguments();
    QStringList sessions = SessionManager::sessions();
    // We have command line arguments, try to find a session in them
    // Default to no session loading
    for (const QString &arg : arguments) {
        if (sessions.contains(arg)) {
            // Session argument
            return arg;
        }
    }
    // Handle settings only after command line arguments:
    if (sb_d->m_isAutoRestoreLastSession)
        return SessionManager::startupSession();
    return {};
}

void SessionManagerPrivate::restoreStartupSession()
{
    m_isStartupSessionRestored = true;
    QString sessionToRestoreAtStartup = determineSessionToRestoreAtStartup();
    if (!sessionToRestoreAtStartup.isEmpty())
        ModeManager::activateMode(Core::Constants::MODE_EDIT);

    // We have command line arguments, try to find a session in them
    QStringList arguments = ExtensionSystem::PluginManager::arguments();
    if (!sessionToRestoreAtStartup.isEmpty() && !arguments.isEmpty())
        arguments.removeOne(sessionToRestoreAtStartup);

    // Massage the argument list.
    // Be smart about directories: If there is a session of that name, load it.
    //   Other than that, look for project files in it. The idea is to achieve
    //   'Do what I mean' functionality when starting Creator in a directory with
    //   the single command line argument '.' and avoid editor warnings about not
    //   being able to open directories.
    // In addition, convert "filename" "+45" or "filename" ":23" into
    //   "filename+45"   and "filename:23".
    if (!arguments.isEmpty()) {
        const QStringList sessions = SessionManager::sessions();
        for (int a = 0; a < arguments.size();) {
            const QString &arg = arguments.at(a);
            const QFileInfo fi(arg);
            if (fi.isDir()) {
                const QDir dir(fi.absoluteFilePath());
                // Does the directory name match a session?
                if (sessionToRestoreAtStartup.isEmpty() && sessions.contains(dir.dirName())) {
                    sessionToRestoreAtStartup = dir.dirName();
                    arguments.removeAt(a);
                    continue;
                }
            } // Done directories.
            // Converts "filename" "+45" or "filename" ":23" into "filename+45" and "filename:23"
            if (a && (arg.startsWith(QLatin1Char('+')) || arg.startsWith(QLatin1Char(':')))) {
                arguments[a - 1].append(arguments.takeAt(a));
                continue;
            }
            ++a;
        } // for arguments
    }     // !arguments.isEmpty()

    // Restore latest session or what was passed on the command line
    ProjectManager::loadSession(!sessionToRestoreAtStartup.isEmpty() ? sessionToRestoreAtStartup
                                                                     : QString(),
                                true);

    // delay opening projects from the command line even more
    QTimer::singleShot(0, m_instance, [arguments] {
        ICore::openFiles(Utils::transform(arguments, &FilePath::fromUserInput),
                         ICore::OpenFilesFlags(ICore::CanContainLineAndColumnNumbers
                                               | ICore::SwitchMode));
        emit m_instance->startupSessionRestored();
    });
}

bool SessionManagerPrivate::isStartupSessionRestored()
{
    return sb_d->m_isStartupSessionRestored;
}

void SessionManagerPrivate::saveSettings()
{
    ICore::settings()->setValueWithDefault(AUTO_RESTORE_SESSION_SETTINGS_KEY,
                                           sb_d->m_isAutoRestoreLastSession,
                                           kIsAutoRestoreLastSessionDefault);
}

void SessionManagerPrivate::restoreSettings()
{
    sb_d->m_isAutoRestoreLastSession = ICore::settings()
                                           ->value(AUTO_RESTORE_SESSION_SETTINGS_KEY,
                                                   kIsAutoRestoreLastSessionDefault)
                                           .toBool();
}

bool SessionManagerPrivate::isAutoRestoreLastSession()
{
    return sb_d->m_isAutoRestoreLastSession;
}

void SessionManagerPrivate::setAutoRestoreLastSession(bool restore)
{
    sb_d->m_isAutoRestoreLastSession = restore;
}

void SessionManagerPrivate::restoreValues(const PersistentSettingsReader &reader)
{
    const QStringList keys = reader.restoreValue(QLatin1String("valueKeys")).toStringList();
    for (const QString &key : keys) {
        QVariant value = reader.restoreValue(QLatin1String("value-") + key);
        m_values.insert(key, value);
    }
}

void SessionManagerPrivate::restoreSessionValues(const PersistentSettingsReader &reader)
{
    const QVariantMap values = reader.restoreValues();
    // restore toplevel items that are not restored by restoreValues
    const auto end = values.constEnd();
    for (auto it = values.constBegin(); it != end; ++it) {
        if (it.key() == "valueKeys" || it.key().startsWith("value-"))
            continue;
        m_sessionValues.insert(it.key(), it.value());
    }
}

void SessionManagerPrivate::restoreEditors()
{
    const QVariant editorsettings = m_sessionValues.value("EditorSettings");
    if (editorsettings.isValid()) {
        EditorManager::restoreState(QByteArray::fromBase64(editorsettings.toByteArray()));
        SessionManager::sessionLoadingProgress();
    }
}

/*!
    Returns the last session that was opened by the user.
*/
QString SessionManager::lastSession()
{
    return ICore::settings()->value(LASTSESSION_KEY).toString();
}

/*!
    Returns the session that was active when Qt Creator was last closed, if any.
*/
QString SessionManager::startupSession()
{
    return ICore::settings()->value(STARTUPSESSION_KEY).toString();
}

void SessionManager::markSessionFileDirty()
{
    sb_d->m_virginSession = false;
}

void SessionManager::sessionLoadingProgress()
{
    sb_d->m_future.setProgressValue(sb_d->m_future.progressValue() + 1);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SessionManager::addSessionLoadingSteps(int steps)
{
    sb_d->m_future.setProgressRange(0, sb_d->m_future.progressMaximum() + steps);
}

} // namespace ProjectExplorer
