// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "session.h"

#include "sessiondialog.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "coreplugin.h"
#include "editormanager/editormanager.h"
#include "icore.h"
#include "modemanager.h"
#include "progressmanager/progressmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/macroexpander.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/store.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>

#include <nanotrace/nanotrace.h>

#include <QAction>
#include <QActionGroup>
#include <QFutureInterface>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>

using namespace ExtensionSystem;
using namespace Utils;

namespace Core {

namespace PE {
struct Tr
{
    Q_DECLARE_TR_FUNCTIONS(QtC::ProjectExplorer)
};
} // namespace PE

const char DEFAULT_SESSION[] = "default";
const char LAST_ACTIVE_TIMES_KEY[] = "LastActiveTimes";
const char STARTUPSESSION_KEY[] = "ProjectExplorer/SessionToRestore";
const char LASTSESSION_KEY[] = "ProjectExplorer/StartupSession";
const char AUTO_RESTORE_SESSION_SETTINGS_KEY[] = "ProjectExplorer/Settings/AutoRestoreLastSession";
static bool kIsAutoRestoreLastSessionDefault = false;
const char M_SESSION[] = "ProjectExplorer.Menu.Session";

/*!
     \class Core::SessionManager
     \inmodule QtCreator

     \brief The SessionManager class manages sessions.

     TODO the interface of this class is not really great.
     The implementation suffers from that all the functions from the
     public interface just wrap around functions which do the actual work.
     This could be improved.
*/

class SessionManagerPrivate
{
public:
    void restoreStartupSession();

    void restoreValues(const PersistentSettingsReader &reader);
    void restoreSessionValues(const PersistentSettingsReader &reader);
    void restoreEditors();

    void saveSettings();
    void restoreSettings();
    bool isAutoRestoreLastSession();
    void setAutoRestoreLastSession(bool restore);

    void updateSessionMenu();

    static QString windowTitleAddition(const FilePath &filePath);
    static QString sessionTitle(const FilePath &filePath);

    QString m_sessionName = "default";
    bool m_isAutoRestoreLastSession = false;
    bool m_virginSession = true;
    bool m_loadingSession = false;

    mutable QStringList m_sessions;
    mutable QHash<QString, QDateTime> m_sessionDateTimes;
    QHash<QString, QDateTime> m_lastActiveTimes;

    QMap<Utils::Key, QVariant> m_values;
    QMap<Utils::Key, QVariant> m_sessionValues;
    QFutureInterface<void> m_future;
    PersistentSettingsWriter *m_writer = nullptr;

    QMenu *m_sessionMenu;
    QAction *m_sessionManagerAction;
};

static SessionManager *m_instance = nullptr;
static SessionManagerPrivate *d = nullptr;

SessionManager::SessionManager()
{
    m_instance = this;
    d = new SessionManagerPrivate;

    connect(PluginManager::instance(), &PluginManager::initializationDone, this, [] {
        d->restoreStartupSession();
    });

    connect(ModeManager::instance(), &ModeManager::currentModeChanged, [](Id mode) {
        if (mode != Core::Constants::MODE_WELCOME)
            setValue("ActiveMode", mode.toString());
    });

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, [] {
        if (!SessionManager::isLoadingSession())
            SessionManager::saveSession();
        d->saveSettings();
    });

    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &SessionManager::markSessionFileDirty);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &SessionManager::markSessionFileDirty);
    connect(EditorManager::instance(), &EditorManager::autoSaved, this, [] {
        if (!PluginManager::isShuttingDown() && !SessionManager::isLoadingSession())
            SessionManager::saveSession();
    });

    // session menu
    ActionContainer *mfile = ActionManager::actionContainer(Core::Constants::M_FILE);
    ActionContainer *msession = ActionManager::createMenu(M_SESSION);
    msession->menu()->setTitle(PE::Tr::tr("S&essions"));
    msession->setOnAllDisabledBehavior(ActionContainer::Show);
    mfile->addMenu(msession, Core::Constants::G_FILE_SESSION);
    d->m_sessionMenu = msession->menu();
    connect(mfile->menu(), &QMenu::aboutToShow, this, [] { d->updateSessionMenu(); });

    // session manager action
    d->m_sessionManagerAction = new QAction(PE::Tr::tr("&Manage..."), this);
    d->m_sessionMenu->addAction(d->m_sessionManagerAction);
    d->m_sessionMenu->addSeparator();
    Command *cmd = ActionManager::registerAction(d->m_sessionManagerAction,
                                                 "ProjectExplorer.ManageSessions");
    cmd->setDefaultKeySequence(QKeySequence());
    connect(d->m_sessionManagerAction,
            &QAction::triggered,
            SessionManager::instance(),
            &SessionManager::showSessionManager);

    MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerFileVariables("Session",
                                    PE::Tr::tr("File where current session is saved."),
                                    [] {
                                        return SessionManager::sessionNameToFileName(
                                            SessionManager::activeSession());
                                    });
    expander->registerVariable("Session:Name", PE::Tr::tr("Name of current session."), [] {
        return SessionManager::activeSession();
    });

    d->restoreSettings();
}

SessionManager::~SessionManager()
{
    emit m_instance->aboutToUnloadSession(d->m_sessionName);
    delete d->m_writer;
    delete d;
    d = nullptr;
}

SessionManager *SessionManager::instance()
{
   return m_instance;
}

bool SessionManager::isDefaultVirgin()
{
    return isDefaultSession(d->m_sessionName) && d->m_virginSession;
}

bool SessionManager::isDefaultSession(const QString &session)
{
    return session == QLatin1String(DEFAULT_SESSION);
}

bool SessionManager::isLoadingSession()
{
    return d->m_loadingSession;
}

/*!
    Lets other plugins store persistent values specified by \a name and \a value
    within the session file.
*/

void SessionManager::setValue(const Key &name, const QVariant &value)
{
    if (d->m_values.value(name) == value)
        return;
    d->m_values.insert(name, value);
}

QVariant SessionManager::value(const Key &name)
{
    auto it = d->m_values.constFind(name);
    return (it == d->m_values.constEnd()) ? QVariant() : *it;
}

void SessionManager::setSessionValue(const Key &name, const QVariant &value)
{
    d->m_sessionValues.insert(name, value);
}

QVariant SessionManager::sessionValue(const Key &name, const QVariant &defaultValue)
{
    auto it = d->m_sessionValues.constFind(name);
    return (it == d->m_sessionValues.constEnd()) ? defaultValue : *it;
}

QString SessionManager::activeSession()
{
    return d->m_sessionName;
}

QStringList SessionManager::sessions()
{
    if (d->m_sessions.isEmpty()) {
        // We are not initialized yet, so do that now
        const FilePaths sessionFiles =
                ICore::userResourcePath().dirEntries({{"*qws"}}, QDir::Time | QDir::Reversed);
        const QVariantMap lastActiveTimes = ICore::settings()->value(LAST_ACTIVE_TIMES_KEY).toMap();
        for (const FilePath &file : sessionFiles) {
            const QString &name = file.completeBaseName();
            d->m_sessionDateTimes.insert(name, file.lastModified());
            const auto lastActiveTime = lastActiveTimes.find(name);
            d->m_lastActiveTimes.insert(name, lastActiveTime != lastActiveTimes.end()
                    ? lastActiveTime->toDateTime()
                    : file.lastModified());
            if (name != QLatin1String(DEFAULT_SESSION))
                d->m_sessions << name;
        }
        d->m_sessions.prepend(QLatin1String(DEFAULT_SESSION));
    }
    return d->m_sessions;
}

QDateTime SessionManager::sessionDateTime(const QString &session)
{
    return d->m_sessionDateTimes.value(session);
}

QDateTime SessionManager::lastActiveTime(const QString &session)
{
    return d->m_lastActiveTimes.value(session);
}

FilePath SessionManager::sessionNameToFileName(const QString &session)
{
    return ICore::userResourcePath(session + ".qws");
}

/*!
    Creates \a session, but does not actually create the file.

    Returns whether the creation was successful.

*/

bool SessionManager::createSession(const QString &session)
{
    if (sessions().contains(session))
        return false;
    Q_ASSERT(d->m_sessions.size() > 0);
    d->m_sessions.insert(1, session);
    d->m_lastActiveTimes.insert(session, QDateTime::currentDateTime());
    emit instance()->sessionCreated(session);
    return true;
}

bool SessionManager::renameSession(const QString &original, const QString &newName)
{
    if (!cloneSession(original, newName))
        return false;
    if (original == activeSession())
        loadSession(newName);
    emit instance()->sessionRenamed(original, newName);
    return deleteSession(original);
}

void SessionManager::showSessionManager()
{
    saveSession();
    Internal::SessionDialog sessionDialog(ICore::dialogParent());
    sessionDialog.setAutoLoadSession(d->isAutoRestoreLastSession());
    sessionDialog.exec();
    d->setAutoRestoreLastSession(sessionDialog.autoLoadSession());
}

/*!
    Shows a dialog asking the user to confirm the deletion of the specified
    \a sessions.

    Returns whether the user confirmed the deletion.
*/
bool SessionManager::confirmSessionDelete(const QStringList &sessions)
{
    const QString title = sessions.size() == 1 ? PE::Tr::tr("Delete Session")
                                               : PE::Tr::tr("Delete Sessions");
    const QString question
        = sessions.size() == 1
              ? PE::Tr::tr("Delete session %1?").arg(sessions.first())
              : PE::Tr::tr("Delete these sessions?\n    %1").arg(sessions.join("\n    "));
    return QMessageBox::question(ICore::dialogParent(),
                                 title,
                                 question,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

/*!
     Deletes \a session name from session list and the file from disk.

     Returns whether the deletion was successful.
*/
bool SessionManager::deleteSession(const QString &session)
{
    if (!d->m_sessions.contains(session))
        return false;
    d->m_sessions.removeOne(session);
    d->m_lastActiveTimes.remove(session);
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
    if (!d->m_sessions.contains(original))
        return false;

    FilePath sessionFile = sessionNameToFileName(original);
    // If the file does not exist, we can still clone
    if (!sessionFile.exists() || sessionFile.copyFile(sessionNameToFileName(clone))) {
        d->m_sessions.insert(1, clone);
        d->m_sessionDateTimes.insert(clone, sessionNameToFileName(clone).lastModified());
        emit instance()->sessionCreated(clone);

        return true;
    }
    return false;
}

static QString determineSessionToRestoreAtStartup()
{
    // TODO (session) move argument to core
    // Process command line arguments first:
    const bool lastSessionArg = PluginManager::specForPlugin(Internal::CorePlugin::instance())
                                    ->arguments()
                                    .contains("-lastsession");
    if (lastSessionArg && !SessionManager::startupSession().isEmpty())
        return SessionManager::startupSession();
    const QStringList arguments = PluginManager::arguments();
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
    if (d->m_isAutoRestoreLastSession)
        return SessionManager::startupSession();
    return {};
}

void SessionManagerPrivate::restoreStartupSession()
{
    NANOTRACE_SCOPE("Core", "SessionManagerPrivate::restoreStartupSession");
    QString sessionToRestoreAtStartup = determineSessionToRestoreAtStartup();
    if (!sessionToRestoreAtStartup.isEmpty())
        ModeManager::activateMode(Core::Constants::MODE_EDIT);

    // We have command line arguments, try to find a session in them
    QStringList arguments = PluginManager::arguments();
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
    SessionManager::loadSession(!sessionToRestoreAtStartup.isEmpty() ? sessionToRestoreAtStartup
                                                                     : QString(),
                                true);

    ICore::openFiles(Utils::transform(arguments, &FilePath::fromUserInput),
                     ICore::OpenFilesFlags(ICore::CanContainLineAndColumnNumbers
                                           | ICore::SwitchMode));
    emit m_instance->startupSessionRestored();
}

void SessionManagerPrivate::saveSettings()
{
    QtcSettings *s = ICore::settings();
    QVariantMap times;
    for (auto it = d->m_lastActiveTimes.cbegin(); it != d->m_lastActiveTimes.cend(); ++it)
        times.insert(it.key(), it.value());
    s->setValue(LAST_ACTIVE_TIMES_KEY, times);
    if (SessionManager::isDefaultVirgin()) {
        s->remove(STARTUPSESSION_KEY);
    } else {
        s->setValue(STARTUPSESSION_KEY, SessionManager::activeSession());
        s->setValue(LASTSESSION_KEY, SessionManager::activeSession());
    }
    s->setValueWithDefault(AUTO_RESTORE_SESSION_SETTINGS_KEY,
                           d->m_isAutoRestoreLastSession,
                           kIsAutoRestoreLastSessionDefault);
}

void SessionManagerPrivate::restoreSettings()
{
    d->m_isAutoRestoreLastSession = ICore::settings()
                                           ->value(AUTO_RESTORE_SESSION_SETTINGS_KEY,
                                                   kIsAutoRestoreLastSessionDefault)
                                           .toBool();
}

bool SessionManagerPrivate::isAutoRestoreLastSession()
{
    return d->m_isAutoRestoreLastSession;
}

void SessionManagerPrivate::setAutoRestoreLastSession(bool restore)
{
    d->m_isAutoRestoreLastSession = restore;
}

void SessionManagerPrivate::updateSessionMenu()
{
    // Delete group of previous actions (the actions are owned by the group and are deleted with it)
    auto oldGroup = m_sessionMenu->findChild<QActionGroup *>();
    if (oldGroup)
        delete oldGroup;
    m_sessionMenu->clear();

    m_sessionMenu->addAction(m_sessionManagerAction);
    m_sessionMenu->addSeparator();
    auto *ag = new QActionGroup(m_sessionMenu);
    const QString activeSession = SessionManager::activeSession();
    const bool isDefaultVirgin = SessionManager::isDefaultVirgin();

    QStringList sessions = SessionManager::sessions();
    std::sort(std::next(sessions.begin()), sessions.end(), [](const QString &s1, const QString &s2) {
        return SessionManager::lastActiveTime(s1) > SessionManager::lastActiveTime(s2);
    });
    for (int i = 0; i < sessions.size(); ++i) {
        const QString &session = sessions[i];

        const QString actionText
            = ActionManager::withNumberAccelerator(Utils::quoteAmpersands(session), i + 1);
        QAction *act = ag->addAction(actionText);
        act->setCheckable(true);
        if (session == activeSession && !isDefaultVirgin)
            act->setChecked(true);
        QObject::connect(act, &QAction::triggered, SessionManager::instance(), [session] {
            SessionManager::loadSession(session);
        });
    }
    m_sessionMenu->addActions(ag->actions());
    m_sessionMenu->setEnabled(true);
}

void SessionManagerPrivate::restoreValues(const PersistentSettingsReader &reader)
{
    const KeyList keys = keysFromStrings(reader.restoreValue("valueKeys").toStringList());
    for (const Key &key : keys) {
        QVariant value = reader.restoreValue("value-" + key);
        m_values.insert(key, value);
    }
}

void SessionManagerPrivate::restoreSessionValues(const PersistentSettingsReader &reader)
{
    const Store values = reader.restoreValues();
    // restore toplevel items that are not restored by restoreValues
    const auto end = values.constEnd();
    for (auto it = values.constBegin(); it != end; ++it) {
        if (it.key() == "valueKeys" || it.key().view().startsWith("value-"))
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
    Returns the session that was active when \QC was last closed, if any.
*/
QString SessionManager::startupSession()
{
    return ICore::settings()->value(STARTUPSESSION_KEY).toString();
}

void SessionManager::markSessionFileDirty()
{
    d->m_virginSession = false;
}

void SessionManager::sessionLoadingProgress()
{
    d->m_future.setProgressValue(d->m_future.progressValue() + 1);
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}

void SessionManager::addSessionLoadingSteps(int steps)
{
    d->m_future.setProgressRange(0, d->m_future.progressMaximum() + steps);
}

/*
 * ========== Notes on storing and loading the default session ==========
 * The default session comes in two flavors: implicit and explicit. The implicit one,
 * also referred to as "default virgin" in the code base, is the one that is active
 * at start-up, if no session has been explicitly loaded due to command-line arguments
 * or the "restore last session" setting in the session manager.
 * The implicit default session silently turns into the explicit default session
 * by loading a project or a file or changing settings in the Dependencies panel. The explicit
 * default session can also be loaded by the user via the Welcome Screen.
 * This mechanism somewhat complicates the handling of session-specific settings such as
 * the ones in the task pane: Users expect that changes they make there become persistent, even
 * when they are in the implicit default session. However, we can't just blindly store
 * the implicit default session, because then we'd overwrite the project list of the explicit
 * default session. Therefore, we use the following logic:
 *     - Upon start-up, if no session is to be explicitly loaded, we restore the parts of the
 *       explicit default session that are not related to projects, editors etc; the
 *       "general settings" of the session, so to speak.
 *     - When storing the implicit default session, we overwrite only these "general settings"
 *       of the explicit default session and keep the others as they are.
 *     - When switching from the implicit to the explicit default session, we keep the
 *       "general settings" and load everything else from the session file.
 * This guarantees that user changes are properly transferred and nothing gets lost from
 * either the implicit or the explicit default session.
 *
 */
bool SessionManager::loadSession(const QString &session, bool initial)
{
    const bool loadImplicitDefault = session.isEmpty();
    const bool switchFromImplicitToExplicitDefault = session == DEFAULT_SESSION
                                                     && d->m_sessionName == DEFAULT_SESSION
                                                     && !initial;

    // Do nothing if we have that session already loaded,
    // exception if the session is the default virgin session
    // we still want to be able to load the default session
    if (session == d->m_sessionName && !SessionManager::isDefaultVirgin())
        return true;

    if (!loadImplicitDefault && !SessionManager::sessions().contains(session))
        return false;

    // Try loading the file
    FilePath fileName = SessionManager::sessionNameToFileName(loadImplicitDefault ? DEFAULT_SESSION
                                                                                  : session);
    PersistentSettingsReader reader;
    if (fileName.exists()) {
        if (!reader.load(fileName)) {
            QMessageBox::warning(ICore::dialogParent(),
                                 PE::Tr::tr("Error while restoring session"),
                                 PE::Tr::tr("Could not restore session %1")
                                     .arg(fileName.toUserOutput()));

            return false;
        }

        if (loadImplicitDefault) {
            d->restoreValues(reader);
            emit SessionManager::instance()->sessionLoaded(DEFAULT_SESSION);
            return true;
        }
    } else if (loadImplicitDefault) {
        emit SessionManager::instance()->sessionLoaded(DEFAULT_SESSION);
        return true;
    }

    d->m_loadingSession = true;

    // Allow everyone to set something in the session and before saving
    emit SessionManager::instance()->aboutToUnloadSession(d->m_sessionName);

    if (!saveSession()) {
        d->m_loadingSession = false;
        return false;
    }

    // Clean up
    if (!EditorManager::closeAllEditors()) {
        d->m_loadingSession = false;
        return false;
    }

    if (!switchFromImplicitToExplicitDefault)
        d->m_values.clear();
    d->m_sessionValues.clear();

    d->m_sessionName = session;
    delete d->m_writer;
    d->m_writer = nullptr;
    EditorManager::updateWindowTitles();

    d->m_virginSession = false;

    ProgressManager::addTask(d->m_future.future(),
                             PE::Tr::tr("Loading Session"),
                             "ProjectExplorer.SessionFile.Load");

    d->m_future.setProgressRange(0, 1 /*initialization*/ + 1 /*editors*/);
    d->m_future.setProgressValue(0);

    if (fileName.exists()) {
        if (!switchFromImplicitToExplicitDefault)
            d->restoreValues(reader);
        d->restoreSessionValues(reader);
    }

    QColor c = QColor(SessionManager::sessionValue("Color").toString());
    if (c.isValid())
        StyleHelper::setBaseColor(c);

    SessionManager::sessionLoadingProgress();

    d->restoreEditors();

    // let other code restore the session
    emit SessionManager::instance()->aboutToLoadSession(session);

    d->m_future.reportFinished();
    d->m_future = QFutureInterface<void>();

    d->m_lastActiveTimes.insert(session, QDateTime::currentDateTime());

    emit SessionManager::instance()->sessionLoaded(session);

    d->m_loadingSession = false;
    return true;
}

bool SessionManager::saveSession()
{
    emit SessionManager::instance()->aboutToSaveSession();

    const FilePath filePath = SessionManager::sessionNameToFileName(d->m_sessionName);
    Store data;

    // See the explanation at loadSession() for how we handle the implicit default session.
    if (SessionManager::isDefaultVirgin()) {
        if (filePath.exists()) {
            PersistentSettingsReader reader;
            if (!reader.load(filePath)) {
                QMessageBox::warning(ICore::dialogParent(),
                                     PE::Tr::tr("Error while saving session"),
                                     PE::Tr::tr("Could not save session %1")
                                         .arg(filePath.toUserOutput()));
                return false;
            }
            data = reader.restoreValues();
        }
    } else {
        const QColor c = StyleHelper::requestedBaseColor();
        if (c.isValid()) {
            QString tmp = QString::fromLatin1("#%1%2%3")
                              .arg(c.red(), 2, 16, QLatin1Char('0'))
                              .arg(c.green(), 2, 16, QLatin1Char('0'))
                              .arg(c.blue(), 2, 16, QLatin1Char('0'));
            setSessionValue("Color", tmp);
        }
        setSessionValue("EditorSettings", EditorManager::saveState().toBase64());

        const auto end = d->m_sessionValues.constEnd();
        for (auto it = d->m_sessionValues.constBegin(); it != end; ++it)
            data.insert(it.key(), it.value());
    }

    const auto end = d->m_values.constEnd();
    KeyList keys;
    for (auto it = d->m_values.constBegin(); it != end; ++it) {
        data.insert("value-" + it.key(), it.value());
        keys << it.key();
    }
    data.insert("valueKeys", stringsFromKeys(keys));

    if (!d->m_writer || d->m_writer->fileName() != filePath) {
        delete d->m_writer;
        d->m_writer = new PersistentSettingsWriter(filePath, "QtCreatorSession");
    }
    const bool result = d->m_writer->save(data, ICore::dialogParent());
    if (result) {
        if (!SessionManager::isDefaultVirgin())
            d->m_sessionDateTimes.insert(SessionManager::activeSession(),
                                            QDateTime::currentDateTime());
    } else {
        QMessageBox::warning(ICore::dialogParent(),
                             PE::Tr::tr("Error while saving session"),
                             PE::Tr::tr("Could not save session to file \"%1\"")
                                 .arg(d->m_writer->fileName().toUserOutput()));
    }

    return result;
}

} // namespace Core
