/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "actionmanager.h"
#include "mainwindow.h"
#include "actioncontainer.h"
#include "command.h"
#include "uniqueidmanager.h"

#include <coreplugin/coreconstants.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMenu>
#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>

namespace {
    enum { warnAboutFindFailures = 0 };
}

/*!
    \class Core::ActionManagerInterface
    \mainclass
    \ingroup qwb
    \inheaderfile actionmanagerinterface.h

    \brief All actions should be registered in the ActionManager, since this enables the user to
    e.g. change their shortcuts at a central place.

    The ActionManagerInterface is the central bookkeeper of actions and their shortcuts and layout.
    You get the only implementation of this class from the core interface (ICore::actionManager()).

    The main reasons for the need of this class is to provide a central place where the user
    can specify all his keyboard shortcuts, and to provide a solution for actions that should
    behave differently in different contexts (like the copy/replace/undo/redo actions).

    All actions that are registered with the same string id (but different context lists)
    are considered to be overloads of the same command. The action that is visible to the user
    is the one returned by ICommand::action(). (If you provide yourself a user visible
    representation of your action be sure to always use ICommand::action() for this.)
    If this action is invoked by the user, the signal is forwarded to the registered action that
    is valid for the current context.

    You use this class also to add items to registered
    action containers like the applications menu bar. For this you register your action via the
    registerAction methods, get the action container for a specific id (like specified in
    Core::Constants) with a call of
    actionContainer(const QString&) and add your command to this container.

    Guidelines:
    \list
    \o Always register your actions and shortcuts!
    \o When registering an action with cmd=registerAction(action, id, contexts) be sure to connect
       your own action connect(action, SIGNAL...) but make cmd->action() visible to the user, i.e.
       widget->addAction(cmd->action()).
    \o Use this class to add actions to the applications menus
    \endlist

    \sa Core::ICore, Core::ICommand
    \sa Core::IActionContainer
*/

/*!
    \fn virtual IActionContainer *ActionManagerInterface::createMenu(const QString &id) = 0
    ...
*/

/*!
    \fn virtual IActionContainer *ActionManagerInterface::createMenuBar(const QString &id) = 0
    ...
*/

/*!
    \fn virtual ICommand *ActionManagerInterface::registerAction(QAction *action, const QString &id, const QList<int> &context) = 0
    ...
*/

/*!
    \fn virtual ICommand *ActionManagerInterface::registerShortcut(QShortcut *shortcut, const QString &id, const QList<int> &context) = 0
    ...
*/

/*!
    \fn virtual ICommand *ActionManagerInterface::registerAction(QAction *action, const QString &id) = 0
    ...
*/

/*!
    \fn virtual void ActionManagerInterface::addAction(ICommand *action, const QString &globalGroup) = 0
    ...
*/

/*!
    \fn virtual void ActionManagerInterface::addMenu(IActionContainer *menu, const QString &globalGroup) = 0
    ...
*/

/*!
    \fn virtual ICommand *ActionManagerInterface::command(const QString &id) const = 0
    ...
*/

/*!
    \fn virtual IActionContainer *ActionManagerInterface::actionContainer(const QString &id) const = 0
    ...
*/

/*!
    \fn virtual ActionManagerInterface::~ActionManagerInterface()
    ...
*/

using namespace Core;
using namespace Core::Internal;

ActionManager* ActionManager::m_instance = 0;

/*!
    \class ActionManager
    \ingroup qwb
    \inheaderfile actionmanager.h

    \sa ActionContainer
*/

/*!
    ...
*/
ActionManager::ActionManager(MainWindow *mainWnd, UniqueIDManager *uidmgr) :
    ActionManagerInterface(mainWnd),
    m_mainWnd(mainWnd)
{
    m_defaultGroups << uidmgr->uniqueIdentifier(Constants::G_DEFAULT_ONE);
    m_defaultGroups << uidmgr->uniqueIdentifier(Constants::G_DEFAULT_TWO);
    m_defaultGroups << uidmgr->uniqueIdentifier(Constants::G_DEFAULT_THREE);
    m_instance = this;

}

/*!
    ...
*/
ActionManager::~ActionManager()
{
    qDeleteAll(m_idCmdMap.values());
    qDeleteAll(m_idContainerMap.values());
}

/*!
    ...
*/
ActionManager* ActionManager::instance()
{
    return m_instance;
}

/*!
    ...
*/
QList<int> ActionManager::defaultGroups() const
{
    return m_defaultGroups;
}

/*!
    ...
*/
QList<Command *> ActionManager::commands() const
{
    return m_idCmdMap.values();
}

/*!
    ...
*/
QList<ActionContainer *> ActionManager::containers() const
{
    return m_idContainerMap.values();
}

/*!
    ...
*/
void ActionManager::registerGlobalGroup(int groupId, int containerId)
{
    if (m_globalgroups.contains(groupId)) {
        qWarning() << "registerGlobalGroup: Global group "
            << m_mainWnd->uniqueIDManager()->stringForUniqueIdentifier(groupId)
            << " already registered";
    } else {
        m_globalgroups.insert(groupId, containerId);
    }
}

/*!
    ...
*/
bool ActionManager::hasContext(int context) const
{
    return m_context.contains(context);
}

/*!
    ...
*/
void ActionManager::setContext(const QList<int> &context)
{
    // here are possibilities for speed optimization if necessary:
    // let commands (de-)register themselves for contexts
    // and only update commands that are either in old or new contexts
    m_context = context;
    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator it = m_idCmdMap.constBegin(); it != cmdcend; ++it)
        it.value()->setCurrentContext(m_context);

    const IdContainerMap::const_iterator acend = m_idContainerMap.constEnd();
    for ( IdContainerMap::const_iterator it = m_idContainerMap.constBegin(); it != acend; ++it)
        it.value()->update();
}

/*!
    \internal
*/
bool ActionManager::hasContext(QList<int> context) const
{
    for (int i=0; i<m_context.count(); ++i) {
        if (context.contains(m_context.at(i)))
            return true;
    }
    return false;
}

/*!
    ...
*/
IActionContainer *ActionManager::createMenu(const QString &id)
{
    const int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(uid);
    if (it !=  m_idContainerMap.constEnd())
        return it.value();

    QMenu *m = new QMenu(m_mainWnd);
    m->setObjectName(id);

    MenuActionContainer *mc = new MenuActionContainer(uid);
    mc->setMenu(m);

    m_idContainerMap.insert(uid, mc);

    return mc;
}

/*!
    ...
*/
IActionContainer *ActionManager::createMenuBar(const QString &id)
{
    const int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(uid);
    if (it !=  m_idContainerMap.constEnd())
        return it.value();

    QMenuBar *mb = new QMenuBar; // No parent (System menu bar on Mac OS X)
    mb->setObjectName(id);

    MenuBarActionContainer *mbc = new MenuBarActionContainer(uid);
    mbc->setMenuBar(mb);

    m_idContainerMap.insert(uid, mbc);

    return mbc;
}

/*!
    ...
*/
ICommand *ActionManager::registerAction(QAction *action, const QString &id, const QList<int> &context)
{
    OverrideableAction *a = 0;
    ICommand *c = registerOverridableAction(action, id, false);
    a = static_cast<OverrideableAction *>(c);
    if (a)
        a->addOverrideAction(action, context);
    return a;
}

/*!
    ...
*/
ICommand *ActionManager::registerAction(QAction *action, const QString &id)
{
    return registerOverridableAction(action, id, true);
}

/*!
    \internal
*/
ICommand *ActionManager::registerOverridableAction(QAction *action, const QString &id, bool checkUnique)
{
    OverrideableAction *a = 0;
    const int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    if (Command *c = m_idCmdMap.value(uid, 0)) {
        if (c->type() != ICommand::CT_OverridableAction) {
            qWarning() << "registerAction: id" << id << "is registered with a different command type.";
            return c;
        }
        a = static_cast<OverrideableAction *>(c);
    }
    if (!a) {
        a = new OverrideableAction(uid);
        m_idCmdMap.insert(uid, a);
    }

    if (!a->action()) {
        QAction *baseAction = new QAction(m_mainWnd);
        baseAction->setObjectName(id);
        baseAction->setCheckable(action->isCheckable());
        baseAction->setIcon(action->icon());
        baseAction->setIconText(action->iconText());
        baseAction->setText(action->text());
        baseAction->setToolTip(action->toolTip());
        baseAction->setStatusTip(action->statusTip());
        baseAction->setWhatsThis(action->whatsThis());
        baseAction->setChecked(action->isChecked());
        baseAction->setSeparator(action->isSeparator());
        baseAction->setShortcutContext(Qt::ApplicationShortcut);
        baseAction->setEnabled(false);
        baseAction->setObjectName(id);
        baseAction->setParent(m_mainWnd);
#ifdef Q_OS_MAC
        baseAction->setIconVisibleInMenu(false);
#endif
        a->setAction(baseAction);
        m_mainWnd->addAction(baseAction);
        a->setKeySequence(a->keySequence());
        a->setDefaultKeySequence(QKeySequence());
    } else  if (checkUnique) {
        qWarning() << "registerOverridableAction: id" << id << "is already registered.";
    }

    return a;
}

/*!
    ...
*/
ICommand *ActionManager::registerShortcut(QShortcut *shortcut, const QString &id, const QList<int> &context)
{
    Shortcut *sc = 0;
    int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    if (Command *c = m_idCmdMap.value(uid, 0)) {
        if (c->type() != ICommand::CT_Shortcut) {
            qWarning() << "registerShortcut: id" << id << "is registered with a different command type.";
            return c;
        }
        sc = static_cast<Shortcut *>(c);
    } else {
        sc = new Shortcut(uid);
        m_idCmdMap.insert(uid, sc);
    }

    if (sc->shortcut()) {
        qWarning() << "registerShortcut: action already registered (id" << id << ".";
        return sc;
    }

    if (!hasContext(context))
        shortcut->setEnabled(false);
    shortcut->setObjectName(id);
    shortcut->setParent(m_mainWnd);
    sc->setShortcut(shortcut);

    if (context.isEmpty())
        sc->setContext(QList<int>() << 0);
    else
        sc->setContext(context);

    sc->setKeySequence(shortcut->key());
    sc->setDefaultKeySequence(QKeySequence());

    return sc;
}

/*!
    \fn void ActionManager::addAction(Core::ICommand *action, const QString &globalGroup)
*/
void ActionManager::addAction(ICommand *action, const QString &globalGroup)
{
    const int gid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(globalGroup);
    if (!m_globalgroups.contains(gid)) {
        qWarning() << "addAction: Unknown global group " << globalGroup;
        return;
    }

    const int cid = m_globalgroups.value(gid);
    if (IActionContainer *aci = actionContainer(cid)) {
        aci->addAction(action, globalGroup);
    } else {
        qWarning() << "addAction: Cannot find container." << cid << '/' << gid;
    }
}

/*!
    \fn void ActionManager::addMenu(Core::IActionContainer *menu, const QString &globalGroup)

*/
void ActionManager::addMenu(IActionContainer *menu, const QString &globalGroup)
{
    const int gid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(globalGroup);
    if (!m_globalgroups.contains(gid)) {
        qWarning() << "addAction: Unknown global group " << globalGroup;
        return;
    }

    const int cid = m_globalgroups.value(gid);
    if (IActionContainer *aci = actionContainer(cid)) {
        aci->addMenu(menu, globalGroup);
    } else {
        qWarning() << "addAction: Cannot find container." << cid << '/' << gid;
    }
}

/*!
    ...
*/
ICommand *ActionManager::command(const QString &id) const
{
    const int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    const IdCmdMap::const_iterator it = m_idCmdMap.constFind(uid);
    if (it == m_idCmdMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManager::command(): failed to find :" << id << '/' << uid;
        return 0;
    }
    return it.value();
}

/*!
    ...
*/
IActionContainer *ActionManager::actionContainer(const QString &id) const
{
    const int uid = m_mainWnd->uniqueIDManager()->uniqueIdentifier(id);
    const IdContainerMap::const_iterator it =  m_idContainerMap.constFind(uid);
    if ( it == m_idContainerMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManager::actionContainer(): failed to find :" << id << '/' << uid;
        return 0;
    }
    return it.value();
}

/*!
    ...
*/
ICommand *ActionManager::command(int uid) const
{
    const IdCmdMap::const_iterator it = m_idCmdMap.constFind(uid);
    if (it == m_idCmdMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManager::command(): failed to find :" <<  m_mainWnd->uniqueIDManager()->stringForUniqueIdentifier(uid) << '/' << uid;
        return 0;
    }
    return it.value();
}

/*!
    ...
*/
IActionContainer *ActionManager::actionContainer(int uid) const
{
    const IdContainerMap::const_iterator it = m_idContainerMap.constFind(uid);
    if (it == m_idContainerMap.constEnd()) {
        if (warnAboutFindFailures)
            qWarning() << "ActionManager::actionContainer(): failed to find :" <<  m_mainWnd->uniqueIDManager()->stringForUniqueIdentifier(uid) << uid;
        return 0;
    }
    return it.value();
}
static const char *settingsGroup = "KeyBindings";
static const char *idKey = "ID";
static const char *sequenceKey = "Keysequence";

/*!
    \internal
*/
void ActionManager::initialize()
{
    QSettings *settings = m_mainWnd->settings();
    const int shortcuts = settings->beginReadArray(QLatin1String(settingsGroup));
    for (int i=0; i<shortcuts; ++i) {
        settings->setArrayIndex(i);
        const QString sid = settings->value(QLatin1String(idKey)).toString();
        const QKeySequence key(settings->value(QLatin1String(sequenceKey)).toString());
        const int id = m_mainWnd->uniqueIDManager()->uniqueIdentifier(sid);

        ICommand *cmd = command(id);
        if (cmd)
            cmd->setKeySequence(key);
    }
    settings->endArray();
}

/*!
    ...
*/
void ActionManager::saveSettings(QSettings *settings)
{
    settings->beginWriteArray(QLatin1String(settingsGroup));
    int count = 0;

    const IdCmdMap::const_iterator cmdcend = m_idCmdMap.constEnd();
    for (IdCmdMap::const_iterator j = m_idCmdMap.constBegin(); j != cmdcend; ++j) {
        const int id = j.key();
        Command *cmd = j.value();
        QKeySequence key = cmd->keySequence();
        if (key != cmd->defaultKeySequence()) {
            const QString sid = m_mainWnd->uniqueIDManager()->stringForUniqueIdentifier(id);
            settings->setArrayIndex(count);
            settings->setValue(QLatin1String(idKey), sid);
            settings->setValue(QLatin1String(sequenceKey), key.toString());
            count++;
        }
    }

    settings->endArray();
}
