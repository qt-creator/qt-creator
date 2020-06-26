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

#include "actionmanager.h"
#include "actionmanager_p.h"
#include "actioncontainer_p.h"
#include "command_p.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/fadingindicator.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>

namespace {
    enum { warnAboutFindFailures = 0 };
}

static const char kKeyboardSettingsKey[] = "KeyboardShortcuts";
static const char kKeyboardSettingsKeyV2[] = "KeyboardShortcutsV2";

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

/*!
    \class Core::ActionManager
    \inheaderfile coreplugin/actionmanager/actionmanager.h
    \ingroup mainclasses
    \inmodule QtCreator

    \brief The ActionManager class is responsible for registration of menus and
    menu items and keyboard shortcuts.

    The action manager is the central bookkeeper of actions and their shortcuts
    and layout. It is a singleton containing mostly static functions. If you
    need access to the instance, for example for connecting to signals, call
    its ActionManager::instance() function.

    The action manager makes it possible to provide a central place where the
    users can specify all their keyboard shortcuts, and provides a solution for
    actions that should behave differently in different contexts (like the
    copy/replace/undo/redo actions).

    See \l{The Action Manager and Commands} for an overview of the interaction
    between Core::ActionManager, Core::Command, and Core::Context.

    Register a globally active action "My Action" by putting the following in
    your plugin's ExtensionSystem::IPlugin::initialize() function.

    \code
        QAction *myAction = new QAction(tr("My Action"), this);
        Command *cmd = ActionManager::registerAction(myAction, "myplugin.myaction", Context(C_GLOBAL));
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+u")));
        connect(myAction, &QAction::triggered, this, &MyPlugin::performMyAction);
    \endcode

    The \c connect is done to your own QAction instance. If you create for
    example a tool button that should represent the action, add the action from
    Command::action() to it.

    \code
        QToolButton *myButton = new QToolButton(someParentWidget);
        myButton->setDefaultAction(cmd->action());
    \endcode

    Also use the action manager to add items to registered action containers
    like the application's menu bar or menus in that menu bar. Register your
    action via the Core::ActionManager::registerAction() function, get the
    action container for a specific ID (as specified for example in the
    Core::Constants namespace) with Core::ActionManager::actionContainer(), and
    add your command to this container.

    Building on the example, adding "My Action" to the "Tools" menu would be
    done with

    \code
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addAction(cmd);
    \endcode

    \sa Core::ICore
    \sa Core::Command
    \sa Core::ActionContainer
    \sa Core::IContext
    \sa {The Action Manager and Commands}
*/

/*!
    \fn void Core::ActionManager::commandListChanged()

    Emitted when the command list has changed.
*/

/*!
    \fn void Core::ActionManager::commandAdded(Utils::Id id)

    Emitted when a command (with the \a id) is added.
*/

static ActionManager *m_instance = nullptr;
static ActionManagerPrivate *d;

/*!
    \internal
*/
ActionManager::ActionManager(QObject *parent)
    : QObject(parent)
{
    m_instance = this;
    d = new ActionManagerPrivate;
    if (Utils::HostOsInfo::isMacHost())
        QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
}

/*!
    \internal
*/
ActionManager::~ActionManager()
{
    delete d;
}

/*!
    Returns the pointer to the instance. Only use for connecting to signals.
*/
ActionManager *ActionManager::instance()
{
    return m_instance;
}

/*!
    Creates a new menu action container or returns an existing container with
    the specified \a id. The ActionManager owns the returned ActionContainer.
    Add your menu to some other menu or a menu bar via the actionContainer()
    and ActionContainer::addMenu() functions.

    \sa actionContainer()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createMenu(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it !=  d->m_idContainerMap.constEnd())
        return it.value();

    auto mc = new MenuActionContainer(id);

    d->m_idContainerMap.insert(id, mc);
    connect(mc, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);

    return mc;
}

/*!
    Creates a new menu bar action container or returns an existing container
    with the specified \a id. The ActionManager owns the returned
    ActionContainer.

    \sa createMenu()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createMenuBar(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it !=  d->m_idContainerMap.constEnd())
        return it.value();

    auto mb = new QMenuBar; // No parent (System menu bar on macOS)
    mb->setObjectName(id.toString());

    auto mbc = new MenuBarActionContainer(id);
    mbc->setMenuBar(mb);

    d->m_idContainerMap.insert(id, mbc);
    connect(mbc, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);

    return mbc;
}

/*!
    Creates a new (sub) touch bar action container or returns an existing
    container with the specified \a id. The ActionManager owns the returned
    ActionContainer.

    Note that it is only possible to create a single level of sub touch bars.
    The sub touch bar will be represented as a button with \a icon and \a text
    (either of which can be left empty), which opens the sub touch bar when
    touched.

    \sa actionContainer()
    \sa ActionContainer::addMenu()
*/
ActionContainer *ActionManager::createTouchBar(Id id, const QIcon &icon, const QString &text)
{
    QTC_CHECK(!icon.isNull() || !text.isEmpty());
    ActionContainer * const c = d->m_idContainerMap.value(id);
    if (c)
        return c;
    auto ac = new TouchBarActionContainer(id, icon, text);
    d->m_idContainerMap.insert(id, ac);
    connect(ac, &QObject::destroyed, d, &ActionManagerPrivate::containerDestroyed);
    return ac;
}

/*!
    Makes an \a action known to the system under the specified \a id.

    Returns a Command instance that represents the action in the application
    and is owned by the ActionManager. You can register several actions with
    the same \a id as long as the \a context is different. In this case
    triggering the action is forwarded to the registered QAction for the
    currently active context. If the optional \a context argument is not
    specified, the global context will be assumed. A \a scriptable action can
    be called from a script without the need for the user to interact with it.
*/
Command *ActionManager::registerAction(QAction *action, Id id, const Context &context, bool scriptable)
{
    Action *a = d->overridableAction(id);
    if (a) {
        a->addOverrideAction(action, context, scriptable);
        emit m_instance->commandListChanged();
        emit m_instance->commandAdded(id);
    }
    return a;
}

/*!
    Returns the Command instance that has been created with registerAction()
    for the specified \a id.

    \sa registerAction()
*/
Command *ActionManager::command(Id id)
{
    const ActionManagerPrivate::IdCmdMap::const_iterator it = d->m_idCmdMap.constFind(id);
    if (it == d->m_idCmdMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::command(): failed to find :"
                       << id.name();
        return nullptr;
    }
    return it.value();
}

/*!
    Returns the ActionContainter instance that has been created with
    createMenu(), createMenuBar(), createTouchBar() for the specified \a id.

    Use the ID \c{Core::Constants::MENU_BAR} to retrieve the main menu bar.

    Use the IDs \c{Core::Constants::M_FILE}, \c{Core::Constants::M_EDIT}, and
    similar constants to retrieve the various default menus.

    Use the ID \c{Core::Constants::TOUCH_BAR} to retrieve the main touch bar.

    \sa ActionManager::createMenu()
    \sa ActionManager::createMenuBar()
*/
ActionContainer *ActionManager::actionContainer(Id id)
{
    const ActionManagerPrivate::IdContainerMap::const_iterator it = d->m_idContainerMap.constFind(id);
    if (it == d->m_idContainerMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::actionContainer(): failed to find :"
                       << id.name();
        return nullptr;
    }
    return it.value();
}

/*!
    Returns all registered commands.
*/
QList<Command *> ActionManager::commands()
{
    // transform list of Action into list of Command
    QList<Command *> result;
    foreach (Command *cmd, d->m_idCmdMap)
        result << cmd;
    return result;
}

/*!
    Removes the knowledge about an \a action under the specified \a id.

    Usually you do not need to unregister actions. The only valid use case for unregistering
    actions, is for actions that represent user definable actions, like for the custom Locator
    filters. If the user removes such an action, it also has to be unregistered from the action manager,
    to make it disappear from shortcut settings etc.
*/
void ActionManager::unregisterAction(QAction *action, Id id)
{
    Action *a = d->m_idCmdMap.value(id, nullptr);
    if (!a) {
        qWarning() << "unregisterAction: id" << id.name()
                   << "is registered with a different command type.";
        return;
    }
    a->removeOverrideAction(action);
    if (a->isEmpty()) {
        // clean up
        d->saveSettings(a);
        ICore::mainWindow()->removeAction(a->action());
        // ActionContainers listen to the commands' destroyed signals
        delete a->action();
        d->m_idCmdMap.remove(id);
        delete a;
    }
    emit m_instance->commandListChanged();
}

/*!
    \internal
*/
void ActionManager::setPresentationModeEnabled(bool enabled)
{
    if (enabled == isPresentationModeEnabled())
        return;

    // Signal/slots to commands:
    foreach (Command *c, commands()) {
        if (c->action()) {
            if (enabled)
                connect(c->action(), &QAction::triggered, d, &ActionManagerPrivate::actionTriggered);
            else
                disconnect(c->action(), &QAction::triggered, d, &ActionManagerPrivate::actionTriggered);
        }
    }

    d->m_presentationModeEnabled = enabled;
}

/*!
    Returns whether presentation mode is enabled.

    The presentation mode is enabled when starting \QC with the command line
    argument \c{-presentationMode}. In presentation mode, \QC displays any
    pressed shortcut in an overlay box.
*/
bool ActionManager::isPresentationModeEnabled()
{
    return d->m_presentationModeEnabled;
}

/*!
    Decorates the specified \a text with a numbered accelerator key \a number,
    in the style of the \uicontrol {Recent Files} menu.
*/
QString ActionManager::withNumberAccelerator(const QString &text, const int number)
{
    if (Utils::HostOsInfo::isMacHost() || number > 9)
        return text;
    return QString("&%1 | %2").arg(number).arg(text);
}

/*!
    \internal
*/
void ActionManager::saveSettings()
{
    d->saveSettings();
}

/*!
    \internal
*/
void ActionManager::setContext(const Context &context)
{
    d->setContext(context);
}

/*!
    \class ActionManagerPrivate
    \inheaderfile actionmanager_p.h
    \internal
*/

ActionManagerPrivate::~ActionManagerPrivate()
{
    // first delete containers to avoid them reacting to command deletion
    foreach (ActionContainerPrivate *container, m_idContainerMap)
        disconnect(container, &QObject::destroyed, this, &ActionManagerPrivate::containerDestroyed);
    qDeleteAll(m_idContainerMap);
    qDeleteAll(m_idCmdMap);
}

void ActionManagerPrivate::setContext(const Context &context)
{
    // here are possibilities for speed optimization if necessary:
    // let commands (de-)register themselves for contexts
    // and only update commands that are either in old or new contexts
    m_context = context;
    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator it = m_idCmdMap.constBegin(); it != cmdcend; ++it)
        it.value()->setCurrentContext(m_context);
}

bool ActionManagerPrivate::hasContext(const Context &context) const
{
    for (int i = 0; i < m_context.size(); ++i) {
        if (context.contains(m_context.at(i)))
            return true;
    }
    return false;
}

void ActionManagerPrivate::containerDestroyed()
{
    auto container = static_cast<ActionContainerPrivate *>(sender());
    m_idContainerMap.remove(m_idContainerMap.key(container));
}

void ActionManagerPrivate::actionTriggered()
{
    auto action = qobject_cast<QAction *>(QObject::sender());
    if (action)
        showShortcutPopup(action->shortcut().toString());
}

void ActionManagerPrivate::showShortcutPopup(const QString &shortcut)
{
    if (shortcut.isEmpty() || !ActionManager::isPresentationModeEnabled())
        return;

    QWidget *window = QApplication::activeWindow();
    if (!window) {
        if (!QApplication::topLevelWidgets().isEmpty()) {
            window = QApplication::topLevelWidgets().first();
        } else {
            QTC_ASSERT(QApplication::desktop(), return);
            window = QApplication::desktop()->screen();
            QTC_ASSERT(window, return);
        }
    }

    Utils::FadingIndicator::showText(window, shortcut);
}

Action *ActionManagerPrivate::overridableAction(Id id)
{
    Action *a = m_idCmdMap.value(id, nullptr);
    if (!a) {
        a = new Action(id);
        m_idCmdMap.insert(id, a);
        readUserSettings(id, a);
        ICore::mainWindow()->addAction(a->action());
        a->action()->setObjectName(id.toString());
        a->action()->setShortcutContext(Qt::ApplicationShortcut);
        a->setCurrentContext(m_context);

        if (ActionManager::isPresentationModeEnabled())
            connect(a->action(), &QAction::triggered, this, &ActionManagerPrivate::actionTriggered);
    }

    return a;
}

void ActionManagerPrivate::readUserSettings(Id id, Action *cmd)
{
    // TODO Settings V2 were introduced in Qt Creator 4.13, remove old settings at some point
    QSettings *settings = ICore::settings();
    // transfer from old settings if not done before
    const QString group = settings->childGroups().contains(kKeyboardSettingsKeyV2)
                              ? QString(kKeyboardSettingsKeyV2)
                              : QString(kKeyboardSettingsKey);
    settings->beginGroup(group);
    if (settings->contains(id.toString())) {
        const QVariant v = settings->value(id.toString());
        if (QMetaType::Type(v.type()) == QMetaType::QStringList) {
            cmd->setKeySequences(Utils::transform<QList>(v.toStringList(), [](const QString &s) {
                return QKeySequence::fromString(s);
            }));
        } else {
            cmd->setKeySequences({QKeySequence::fromString(v.toString())});
        }
    }
    settings->endGroup();
}

void ActionManagerPrivate::saveSettings(Action *cmd)
{
    const QString id = cmd->id().toString();
    const QString settingsKey = QLatin1String(kKeyboardSettingsKeyV2) + '/' + id;
    const QString compatSettingsKey = QLatin1String(kKeyboardSettingsKey) + '/' + id;
    const QList<QKeySequence> keys = cmd->keySequences();
    const QList<QKeySequence> defaultKeys = cmd->defaultKeySequences();
    if (keys != defaultKeys) {
        if (keys.isEmpty()) {
            ICore::settings()->setValue(settingsKey, QString());
            ICore::settings()->setValue(compatSettingsKey, QString());
        } else if (keys.size() == 1) {
            ICore::settings()->setValue(settingsKey, keys.first().toString());
            ICore::settings()->setValue(compatSettingsKey, keys.first().toString());
        } else {
            ICore::settings()->setValue(settingsKey,
                                        Utils::transform<QStringList>(keys,
                                                                      [](const QKeySequence &k) {
                                                                          return k.toString();
                                                                      }));
        }
    } else {
        ICore::settings()->remove(settingsKey);
    }
}

void ActionManagerPrivate::saveSettings()
{
    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator j = m_idCmdMap.constBegin(); j != cmdcend; ++j) {
        saveSettings(j.value());
    }
}
