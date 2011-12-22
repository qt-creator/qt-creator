/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "actionmanager_p.h"
#include "mainwindow.h"
#include "actioncontainer_p.h"
#include "command_p.h"
#include "id.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QDesktopWidget>
#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QMenuBar>

namespace {
    enum { warnAboutFindFailures = 0 };
}

/*!
    \class Core::ActionManager
    \mainclass

    \brief The action manager is responsible for registration of menus and
    menu items and keyboard shortcuts.

    The ActionManager is the central bookkeeper of actions and their shortcuts and layout.
    You get the only implementation of this class from the core interface
    ICore::actionManager() method, e.g.
    \code
        Core::ICore::instance()->actionManager()
    \endcode

    The main reasons for the need of this class is to provide a central place where the user
    can specify all his keyboard shortcuts, and to provide a solution for actions that should
    behave differently in different contexts (like the copy/replace/undo/redo actions).

    \section1 Contexts

    All actions that are registered with the same string ID (but different context lists)
    are considered to be overloads of the same command, represented by an instance
    of the Command class.
    Exactly only one of the registered actions with the same ID is active at any time.
    Which action this is, is defined by the context list that the actions were registered
    with:

    If the current focus widget was registered via \l{ICore::addContextObject()},
    all the contexts returned by its IContext object are active. In addition all
    contexts set via \l{ICore::addAdditionalContext()} are active as well. If one
    of the actions was registered for one of these active contexts, it is the one
    active action, and receives \c triggered and \c toggled signals. Also the
    appearance of the visible action for this ID might be adapted to this
    active action (depending on the settings of the corresponding \l{Command} object).

    The action that is visible to the user is the one returned by Command::action().
    If you provide yourself a user visible representation of your action you need
    to use Command::action() for this.
    When this action is invoked by the user,
    the signal is forwarded to the registered action that is valid for the current context.

    \section1 Registering Actions

    To register a globally active action "My Action"
    put the following in your plugin's IPlugin::initialize method:
    \code
        Core::ActionManager *am = Core::ICore::instance()->actionManager();
        QAction *myAction = new QAction(tr("My Action"), this);
        Core::Command *cmd = am->registerAction(myAction,
                                                 "myplugin.myaction",
                                                 Core::Context(C_GLOBAL));
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Alt+u")));
        connect(myAction, SIGNAL(triggered()), this, SLOT(performMyAction()));
    \endcode

    So the \c connect is done to your own QAction instance. If you create e.g.
    a tool button that should represent the action you add the action
    from Command::action() to it:
    \code
        QToolButton *myButton = new QToolButton(someParentWidget);
        myButton->setDefaultAction(cmd->action());
    \endcode

    Also use the ActionManager to add items to registered
    action containers like the applications menu bar or menus in that menu bar.
    To do this, you register your action via the
    registerAction methods, get the action container for a specific ID (like specified in
    the Core::Constants namespace) with a call of
    actionContainer(const QString&) and add your command to this container.

    Following the example adding "My Action" to the "Tools" menu would be done by
    \code
        am->actionContainer(Core::M_TOOLS)->addAction(cmd);
    \endcode

    \section1 Important Guidelines:
    \list
    \o Always register your actions and shortcuts!
    \o Register your actions and shortcuts during your plugin's \l{ExtensionSystem::IPlugin::initialize()}
       or \l{ExtensionSystem::IPlugin::extensionsInitialized()} methods, otherwise the shortcuts won't appear
       in the keyboard settings dialog from the beginning.
    \o When registering an action with \c{cmd=registerAction(action, id, contexts)} be sure to connect
       your own action \c{connect(action, SIGNAL...)} but make \c{cmd->action()} visible to the user, i.e.
       \c{widget->addAction(cmd->action())}.
    \o Use this class to add actions to the applications menus
    \endlist

    \sa Core::ICore
    \sa Core::Command
    \sa Core::ActionContainer
    \sa Core::IContext
*/

/*!
    \fn ActionContainer *ActionManager::createMenu(const Id &id)
    \brief Creates a new menu with the given string \a id.

    Returns a new ActionContainer that you can use to get the QMenu instance
    or to add menu items to the menu. The ActionManager owns
    the returned ActionContainer.
    Add your menu to some other menu or a menu bar via the
    ActionManager::actionContainer and ActionContainer::addMenu methods.
*/

/*!
    \fn ActionContainer *ActionManager::createMenuBar(const Id &id)
    \brief Creates a new menu bar with the given string \a id.

    Returns a new ActionContainer that you can use to get the QMenuBar instance
    or to add menus to the menu bar. The ActionManager owns
    the returned ActionContainer.
*/

/*!
    \fn Command *ActionManager::registerAction(QAction *action, const Id &id, const Context &context, bool scriptable)
    \brief Makes an \a action known to the system under the specified string \a id.

    Returns a command object that represents the action in the application and is
    owned by the ActionManager. You can registered several actions with the
    same \a id as long as the \a context is different. In this case
    a trigger of the actual action is forwarded to the registered QAction
    for the currently active context.
    A scriptable action can be called from a script without the need for the user
    to interact with it.
*/

/*!
    \fn Command *ActionManager::registerShortcut(QShortcut *shortcut, const QString &id, const Context &context, bool scriptable)
    \brief Makes a \a shortcut known to the system under the specified string \a id.

    Returns a command object that represents the shortcut in the application and is
    owned by the ActionManager. You can registered several shortcuts with the
    same \a id as long as the \a context is different. In this case
    a trigger of the actual shortcut is forwarded to the registered QShortcut
    for the currently active context.
    A scriptable shortcut can be called from a script without the need for the user
    to interact with it.
*/

/*!
    \fn Command *ActionManager::command(const Id &id) const
    \brief Returns the Command object that is known to the system
    under the given string \a id.

    \sa ActionManager::registerAction()
*/

/*!
    \fn ActionContainer *ActionManager::actionContainer(const Id &id) const
    \brief Returns the IActionContainter object that is know to the system
    under the given string \a id.

    \sa ActionManager::createMenu()
    \sa ActionManager::createMenuBar()
*/

/*!
    \fn Command *ActionManager::unregisterAction(QAction *action, const Id &id)
    \brief Removes the knowledge about an \a action under the specified string \a id.

    Usually you do not need to unregister actions. The only valid use case for unregistering
    actions, is for actions that represent user definable actions, like for the custom Locator
    filters. If the user removes such an action, it also has to be unregistered from the action manager,
    to make it disappear from shortcut settings etc.
*/

/*!
    \fn ActionManager::ActionManager(QObject *parent)
    \internal
*/
/*!
    \fn ActionManager::~ActionManager()
    \internal
*/

using namespace Core;
using namespace Core::Internal;

ActionManagerPrivate* ActionManagerPrivate::m_instance = 0;

/*!
    \class ActionManagerPrivate
    \inheaderfile actionmanager_p.h
    \internal
*/

ActionManagerPrivate::ActionManagerPrivate(MainWindow *mainWnd)
  : ActionManager(mainWnd),
    m_mainWnd(mainWnd),
    m_presentationLabel(0)
{
    m_presentationLabelTimer.setInterval(1000);
    m_instance = this;
}

ActionManagerPrivate::~ActionManagerPrivate()
{
    // first delete containers to avoid them reacting to command deletion
    foreach (ActionContainerPrivate *container, m_idContainerMap)
        disconnect(container, SIGNAL(destroyed()), this, SLOT(containerDestroyed()));
    qDeleteAll(m_idContainerMap.values());
    qDeleteAll(m_idCmdMap.values());
}

ActionManagerPrivate *ActionManagerPrivate::instance()
{
    return m_instance;
}

QList<Command *> ActionManagerPrivate::commands() const
{
    // transform list of CommandPrivate into list of Command
    QList<Command *> result;
    foreach (Command *cmd, m_idCmdMap.values())
        result << cmd;
    return result;
}

bool ActionManagerPrivate::hasContext(int context) const
{
    return m_context.contains(context);
}

QDebug operator<<(QDebug in, const Context &context)
{
    in << "CONTEXT: ";
    foreach (int c, context)
        in << "   " << c << Id::fromUniqueIdentifier(c).toString();
    return in;
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

ActionContainer *ActionManagerPrivate::createMenu(const Id &id)
{
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(id);
    if (it !=  m_idContainerMap.constEnd())
        return it.value();

    QMenu *m = new QMenu(m_mainWnd);
    m->setObjectName(QLatin1String(id.name()));

    MenuActionContainer *mc = new MenuActionContainer(id);
    mc->setMenu(m);

    m_idContainerMap.insert(id, mc);
    connect(mc, SIGNAL(destroyed()), this, SLOT(containerDestroyed()));

    return mc;
}

ActionContainer *ActionManagerPrivate::createMenuBar(const Id &id)
{
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(id);
    if (it !=  m_idContainerMap.constEnd())
        return it.value();

    QMenuBar *mb = new QMenuBar; // No parent (System menu bar on Mac OS X)
    mb->setObjectName(id.toString());

    MenuBarActionContainer *mbc = new MenuBarActionContainer(id);
    mbc->setMenuBar(mb);

    m_idContainerMap.insert(id, mbc);
    connect(mbc, SIGNAL(destroyed()), this, SLOT(containerDestroyed()));

    return mbc;
}

void ActionManagerPrivate::containerDestroyed()
{
    ActionContainerPrivate *container = static_cast<ActionContainerPrivate *>(sender());
    m_idContainerMap.remove(m_idContainerMap.key(container));
}

void ActionManagerPrivate::actionTriggered()
{
    QAction *action = qobject_cast<QAction *>(QObject::sender());
    if (action)
        showShortcutPopup(action->shortcut().toString());
}

void ActionManagerPrivate::shortcutTriggered()
{
    QShortcut *sc = qobject_cast<QShortcut *>(QObject::sender());
    if (sc)
        showShortcutPopup(sc->key().toString());
}

void ActionManagerPrivate::showShortcutPopup(const QString &shortcut)
{
    if (shortcut.isEmpty() || !isPresentationModeEnabled())
        return;

    m_presentationLabel->setText(shortcut);
    m_presentationLabel->adjustSize();

    QPoint p = m_mainWnd->mapToGlobal(m_mainWnd->rect().center() - m_presentationLabel->rect().center());
    m_presentationLabel->move(p);

    m_presentationLabel->show();
    m_presentationLabel->raise();
    m_presentationLabelTimer.start();
}

Command *ActionManagerPrivate::registerAction(QAction *action, const Id &id, const Context &context, bool scriptable)
{
    Action *a = overridableAction(id);
    if (a) {
        a->addOverrideAction(action, context, scriptable);
        emit commandListChanged();
        emit commandAdded(id.toString());
    }
    return a;
}

Action *ActionManagerPrivate::overridableAction(const Id &id)
{
    Action *a = 0;
    if (CommandPrivate *c = m_idCmdMap.value(id, 0)) {
        a = qobject_cast<Action *>(c);
        if (!a) {
            qWarning() << "registerAction: id" << id.name()
                       << "is registered with a different command type.";
            return 0;
        }
    } else {
        a = new Action(id);
        m_idCmdMap.insert(id, a);
        m_mainWnd->addAction(a->action());
        a->action()->setObjectName(id.toString());
        a->action()->setShortcutContext(Qt::ApplicationShortcut);
        a->setCurrentContext(m_context);

        if (isPresentationModeEnabled())
            connect(a->action(), SIGNAL(triggered()), this, SLOT(actionTriggered()));
    }

    return a;
}

void ActionManagerPrivate::unregisterAction(QAction *action, const Id &id)
{
    Action *a = 0;
    CommandPrivate *c = m_idCmdMap.value(id, 0);
    QTC_ASSERT(c, return);
    a = qobject_cast<Action *>(c);
    if (!a) {
        qWarning() << "unregisterAction: id" << id.name()
                   << "is registered with a different command type.";
        return;
    }
    a->removeOverrideAction(action);
    if (a->isEmpty()) {
        // clean up
        // ActionContainers listen to the commands' destroyed signals
        m_mainWnd->removeAction(a->action());
        delete a->action();
        m_idCmdMap.remove(id);
        delete a;
    }
    emit commandListChanged();
}

Command *ActionManagerPrivate::registerShortcut(QShortcut *shortcut, const Id &id, const Context &context, bool scriptable)
{
    Shortcut *sc = 0;
    if (CommandPrivate *c = m_idCmdMap.value(id, 0)) {
        sc = qobject_cast<Shortcut *>(c);
        if (!sc) {
            qWarning() << "registerShortcut: id" << id.name()
                       << "is registered with a different command type.";
            return c;
        }
    } else {
        sc = new Shortcut(id);
        m_idCmdMap.insert(id, sc);
    }

    if (sc->shortcut()) {
        qWarning() << "registerShortcut: action already registered, id" << id.name() << ".";
        return sc;
    }

    if (!hasContext(context))
        shortcut->setEnabled(false);
    shortcut->setObjectName(id.toString());
    shortcut->setParent(m_mainWnd);
    sc->setShortcut(shortcut);
    sc->setScriptable(scriptable);

    if (context.isEmpty())
        sc->setContext(Context(0));
    else
        sc->setContext(context);

    emit commandListChanged();
    emit commandAdded(id.toString());

    if (isPresentationModeEnabled())
        connect(sc->shortcut(), SIGNAL(activated()), this, SLOT(shortcutTriggered()));
    return sc;
}

Command *ActionManagerPrivate::command(const Id &id) const
{
    const IdCmdMap::const_iterator it = m_idCmdMap.constFind(id);
    if (it == m_idCmdMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::command(): failed to find :"
                       << id.name();
        return 0;
    }
    return it.value();
}

ActionContainer *ActionManagerPrivate::actionContainer(const Id &id) const
{
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(id);
    if (it == m_idContainerMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManagerPrivate::actionContainer(): failed to find :"
                       << id.name();
        return 0;
    }
    return it.value();
}

static const char settingsGroup[] = "KeyBindings";
static const char idKey[] = "ID";
static const char sequenceKey[] = "Keysequence";

void ActionManagerPrivate::initialize()
{
    QSettings *settings = Core::ICore::instance()->settings();
    const int shortcuts = settings->beginReadArray(QLatin1String(settingsGroup));
    for (int i = 0; i < shortcuts; ++i) {
        settings->setArrayIndex(i);
        const QKeySequence key(settings->value(QLatin1String(sequenceKey)).toString());
        const Id id = Id(settings->value(QLatin1String(idKey)).toString());

        Command *cmd = command(id);
        if (cmd)
            cmd->setKeySequence(key);
    }
    settings->endArray();
}

void ActionManagerPrivate::saveSettings(QSettings *settings)
{
    settings->beginWriteArray(QLatin1String(settingsGroup));
    int count = 0;

    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator j = m_idCmdMap.constBegin(); j != cmdcend; ++j) {
        const Id id = j.key();
        CommandPrivate *cmd = j.value();
        QKeySequence key = cmd->keySequence();
        if (key != cmd->defaultKeySequence()) {
            settings->setArrayIndex(count);
            settings->setValue(QLatin1String(idKey), id.toString());
            settings->setValue(QLatin1String(sequenceKey), key.toString());
            count++;
        }
    }

    settings->endArray();
}

void ActionManagerPrivate::unregisterShortcut(const Core::Id &id)
{
    Shortcut *sc = 0;
    CommandPrivate *c = m_idCmdMap.value(id, 0);
    QTC_ASSERT(c, return);
    sc = qobject_cast<Shortcut *>(c);
    if (!sc) {
        qWarning() << "unregisterShortcut: id" << id.name()
                   << "is registered with a different command type.";
        return;
    }
    delete sc->shortcut();
    m_idCmdMap.remove(id);
    delete sc;
    emit commandListChanged();
}

void ActionManagerPrivate::setPresentationModeEnabled(bool enabled)
{
    if (enabled == isPresentationModeEnabled())
        return;

    // Signal/slots to commands:
    foreach (Command *c, commands()) {
        if (c->action()) {
            if (enabled)
                connect(c->action(), SIGNAL(triggered()), this, SLOT(actionTriggered()));
            else
                disconnect(c->action(), SIGNAL(triggered()), this, SLOT(actionTriggered()));
        }
        if (c->shortcut()) {
            if (enabled)
                connect(c->shortcut(), SIGNAL(activated()), this, SLOT(shortcutTriggered()));
            else
                disconnect(c->shortcut(), SIGNAL(activated()), this, SLOT(shortcutTriggered()));
        }
    }

    // The label for the shortcuts:
    if (!m_presentationLabel) {
        m_presentationLabel = new QLabel(0, Qt::ToolTip | Qt::WindowStaysOnTopHint);
        QFont font = m_presentationLabel->font();
        font.setPixelSize(45);
        m_presentationLabel->setFont(font);
        m_presentationLabel->setAlignment(Qt::AlignCenter);
        m_presentationLabel->setMargin(5);

        connect(&m_presentationLabelTimer, SIGNAL(timeout()), m_presentationLabel, SLOT(hide()));
    } else {
        m_presentationLabelTimer.stop();
        delete m_presentationLabel;
        m_presentationLabel = 0;
    }
}

bool ActionManagerPrivate::isPresentationModeEnabled()
{
    return m_presentationLabel;
}
