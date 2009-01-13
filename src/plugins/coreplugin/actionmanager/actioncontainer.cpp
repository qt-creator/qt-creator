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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "actioncontainer.h"
#include "actionmanager_p.h"

#include "command.h"
#include "coreimpl.h"

#include "coreconstants.h"
#include "uniqueidmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QToolBar>
#include <QtGui/QMenuBar>

Q_DECLARE_METATYPE(Core::Internal::MenuActionContainer*)

using namespace Core;
using namespace Core::Internal;

/*!
    \class IActionContainer
    \mainclass
    \ingroup qwb
    \inheaderfile iactioncontainer.h

    \brief The class...

    The Action Container interface...
*/

/*!
    \enum IActionContainer::ContainerType
*/

/*!
    \enum IActionContainer::EmptyAction
*/

/*!
    \fn virtual IActionContainer::setEmptyAction(EmptyAction ea)
*/

/*!
    \fn virtual int IActionContainer::id() const
*/

/*!
    \fn virtual ContainerType IActionContainer::type() const
*/

/*!
    \fn virtual QMenu *IActionContainer::menu() const
*/

/*!
    \fn virtual QToolBar *IActionContainer::toolBar() const
*/

/*!
    \fn virtual QMenuBar *IActionContainer::menuBar() const
*/

/*!
    \fn virtual QAction *IActionContainer::insertLocation(const QString &group) const
*/

/*!
    \fn virtual void IActionContainer::appendGroup(const QString &group, bool global)
*/

/*!
    \fn virtual void IActionContainer::addAction(Core::ICommand *action, const QString &group)
*/

/*!
    \fn virtual void IActionContainer::addMenu(Core::IActionContainer *menu, const QString &group)
*/

/*!
    \fn virtual bool IActionContainer::update()
*/

/*!
    \fn virtual IActionContainer::~IActionContainer()
*/

// ---------- ActionContainer ------------

/*!
    \class ActionContainer
    \ingroup qwb
    \inheaderfile actioncontainer.h
*/

/*!
    \enum ActionContainer::ContainerState
*/

/*!
\fn ActionContainer::ActionContainer(ContainerType type, int id)
*/
ActionContainer::ActionContainer(ContainerType type, int id)
    : m_data(CS_None), m_type(type), m_id(id)
{

}

/*!
    \fn virtual ActionContainer::~ActionContainer()
*/

/*!
    ...
*/
void ActionContainer::setEmptyAction(EmptyAction ea)
{
    m_data = ((m_data & ~EA_Mask) | ea);
}

/*!
    ...
*/
bool ActionContainer::hasEmptyAction(EmptyAction ea) const
{
    return (m_data & EA_Mask) == ea;
}

/*!
    ...
*/
void ActionContainer::setState(ContainerState state)
{
    m_data |= state;
}

/*!
    ...
*/
bool ActionContainer::hasState(ContainerState state) const
{
    return (m_data & state);
}

/*!
    ...
*/
void ActionContainer::appendGroup(const QString &group, bool global)
{
    UniqueIDManager *idmanager = CoreImpl::instance()->uniqueIDManager();
    int gid = idmanager->uniqueIdentifier(group);
    m_groups << gid;
    if (global)
        ActionManagerPrivate::instance()->registerGlobalGroup(gid, m_id);
}

/*!
    ...
*/
QAction *ActionContainer::insertLocation(const QString &group) const
{
    UniqueIDManager *idmanager = CoreImpl::instance()->uniqueIDManager();
    int grpid = idmanager->uniqueIdentifier(group);
    int prevKey = 0;
    int pos = ((grpid << 16) | 0xFFFF);
    return beforeAction(pos, &prevKey);
}

/*!
\fn virtual void ActionContainer::insertAction(QAction *before, QAction *action) = 0
*/

/*!
\fn virtual void ActionContainer::insertMenu(QAction *before, QMenu *menu) = 0
*/

/*!
\fn QList<ICommand *> ActionContainer::commands() const
*/

/*!
\fn QList<IActionContainer *> ActionContainer::subContainers() const
*/

/*!
    ...
*/
void ActionContainer::addAction(ICommand *action, const QString &group)
{
    if (!canAddAction(action))
        return;

    ActionManagerPrivate *am = ActionManagerPrivate::instance();
    Action *a = static_cast<Action *>(action);
    if (a->stateFlags() & Command::CS_PreLocation) {
        QList<CommandLocation> locs = a->locations();
        for (int i=0; i<locs.size(); ++i) {
            if (IActionContainer *aci = am->actionContainer(locs.at(i).m_container)) {
                ActionContainer *ac = static_cast<ActionContainer *>(aci);
                ac->addAction(action, locs.at(i).m_position, false);
            }
        }
        a->setStateFlags(a->stateFlags() | Command::CS_Initialized);
    } else {
        UniqueIDManager *idmanager = CoreImpl::instance()->uniqueIDManager();
        int grpid = idmanager->uniqueIdentifier(Constants::G_DEFAULT_TWO);
        if (!group.isEmpty())
            grpid = idmanager->uniqueIdentifier(group);
        if (!m_groups.contains(grpid) && !am->defaultGroups().contains(grpid))
            qWarning() << "*** addAction(): Unknown group: " << group;
        int pos = ((grpid << 16) | 0xFFFF);
        addAction(action, pos, true);
    }
}

/*!
    ...
*/
void ActionContainer::addMenu(IActionContainer *menu, const QString &group)
{
    if (!canAddMenu(menu))
        return;

    ActionManagerPrivate *am = ActionManagerPrivate::instance();
    MenuActionContainer *mc = static_cast<MenuActionContainer *>(menu);
    if (mc->hasState(ActionContainer::CS_PreLocation)) {
        CommandLocation loc = mc->location();
        if (IActionContainer *aci = am->actionContainer(loc.m_container)) {
            ActionContainer *ac = static_cast<ActionContainer *>(aci);
            ac->addMenu(menu, loc.m_position, false);
        }
        mc->setState(ActionContainer::CS_Initialized);
    } else {
        UniqueIDManager *idmanager = CoreImpl::instance()->uniqueIDManager();
        int grpid = idmanager->uniqueIdentifier(Constants::G_DEFAULT_TWO);
        if (!group.isEmpty())
            grpid = idmanager->uniqueIdentifier(group);
        if (!m_groups.contains(grpid) && !am->defaultGroups().contains(grpid))
            qWarning() << "*** addMenu(): Unknown group: " << group;
        int pos = ((grpid << 16) | 0xFFFF);
        addMenu(menu, pos, true);
    }
}

/*!
    ...
*/
int ActionContainer::id() const
{
    return m_id;
}

/*!
    ...
*/
IActionContainer::ContainerType ActionContainer::type() const
{
    return m_type;
}

/*!
    ...
*/
QMenu *ActionContainer::menu() const
{
    return 0;
}

/*!
    ...
*/
QToolBar *ActionContainer::toolBar() const
{
    return 0;
}

/*!
    ...
*/
QMenuBar *ActionContainer::menuBar() const
{
    return 0;
}

/*!
    ...
*/
bool ActionContainer::canAddAction(ICommand *action) const
{
    if (action->type() != ICommand::CT_OverridableAction)
        return false;

    Command *cmd = static_cast<Command *>(action);
    if (cmd->stateFlags() & Command::CS_Initialized)
        return false;

    return true;
}

/*!
    ...
*/
bool ActionContainer::canAddMenu(IActionContainer *menu) const
{
    if (menu->type() != IActionContainer::CT_Menu)
        return false;

    ActionContainer *container = static_cast<ActionContainer *>(menu);
    if (container->hasState(ActionContainer::CS_Initialized))
        return false;

    return true;
}

/*!
    ...
*/
void ActionContainer::addAction(ICommand *action, int pos, bool setpos)
{
    Action *a = static_cast<Action *>(action);

    int prevKey = 0;
    QAction *ba = beforeAction(pos, &prevKey);

    if (setpos) {
        pos = calcPosition(pos, prevKey);
        CommandLocation loc;
        loc.m_container = m_id;
        loc.m_position = pos;
        QList<CommandLocation> locs = a->locations();
        locs.append(loc);
        a->setLocations(locs);
    }

    m_commands.append(action);
    m_posmap.insert(pos, action->id());
    insertAction(ba, a->action());
}

/*!
    ...
*/
void ActionContainer::addMenu(IActionContainer *menu, int pos, bool setpos)
{
    MenuActionContainer *mc = static_cast<MenuActionContainer *>(menu);

    int prevKey = 0;
    QAction *ba = beforeAction(pos, &prevKey);

    if (setpos) {
        pos = calcPosition(pos, prevKey);
        CommandLocation loc;
        loc.m_container = m_id;
        loc.m_position = pos;
        mc->setLocation(loc);
    }

    m_subContainers.append(menu);
    m_posmap.insert(pos, menu->id());
    insertMenu(ba, mc->menu());
}

/*!
    ...
    \internal
*/
QAction *ActionContainer::beforeAction(int pos, int *prevKey) const
{
    ActionManagerPrivate *am = ActionManagerPrivate::instance();

    int baId = -1;

    (*prevKey) = -1;

    QMap<int, int>::const_iterator i = m_posmap.constBegin();
    while (i != m_posmap.constEnd()) {
        if (i.key() > pos) {
            baId = i.value();
            break;
        }
        (*prevKey) = i.key();
        ++i;
    }

    if (baId == -1)
        return 0;

    if (ICommand *cmd = am->command(baId))
        return cmd->action();
    if (IActionContainer *container = am->actionContainer(baId))
        if (QMenu *menu = container->menu())
            return menu->menuAction();

    return 0;
}

/*!
    ...
    \internal
*/
int ActionContainer::calcPosition(int pos, int prevKey) const
{
    int grp = (pos & 0xFFFF0000);
    if (prevKey == -1)
        return grp;

    int prevgrp = (prevKey & 0xFFFF0000);

    if (grp != prevgrp)
        return grp;

    return grp + (prevKey & 0xFFFF) + 10;
}

// ---------- MenuActionContainer ------------

/*!
    \class MenuActionContainer
    \ingroup qwb
    \inheaderfile actioncontainer.h
*/

/*!
    ...
*/
MenuActionContainer::MenuActionContainer(int id)
    : ActionContainer(CT_Menu, id), m_menu(0)
{
    setEmptyAction(EA_Disable);
}

/*!
    ...
*/
void MenuActionContainer::setMenu(QMenu *menu)
{
    m_menu = menu;

    QVariant v;
    qVariantSetValue<MenuActionContainer*>(v, this);

    m_menu->menuAction()->setData(v);
}

/*!
    ...
*/
QMenu *MenuActionContainer::menu() const
{
    return m_menu;
}

/*!
    ...
*/
void MenuActionContainer::insertAction(QAction *before, QAction *action)
{
    m_menu->insertAction(before, action);
}

/*!
    ...
*/
void MenuActionContainer::insertMenu(QAction *before, QMenu *menu)
{
    m_menu->insertMenu(before, menu);
}

/*!
    ...
*/
void MenuActionContainer::setLocation(const CommandLocation &location)
{
    m_location = location;
}

/*!
    ...
*/
CommandLocation MenuActionContainer::location() const
{
    return m_location;
}

/*!
    ...
*/
bool MenuActionContainer::update()
{
    if (hasEmptyAction(EA_None))
        return true;

    bool hasitems = false;

    foreach (IActionContainer *container, subContainers()) {
        if (container == this) {
            qWarning() << Q_FUNC_INFO << "container" << (this->menu() ? this->menu()->title() : "") <<  "contains itself as subcontainer";
            continue;
        }
        if (container->update()) {
            hasitems = true;
            break;
        }
    }
    if (!hasitems) {
        foreach (ICommand *command, commands()) {
            if (command->isActive()) {
                hasitems = true;
                break;
            }
        }
    }

    if (hasEmptyAction(EA_Hide))
        m_menu->setVisible(hasitems);
    else if (hasEmptyAction(EA_Disable))
        m_menu->setEnabled(hasitems);

    return hasitems;
}

// ---------- ToolBarActionContainer ------------

/*!
    \class ToolBarActionContainer
    \ingroup qwb
    \inheaderfile actioncontainer.h
*/

/*!
    ...
*/
ToolBarActionContainer::ToolBarActionContainer(int id)
    : ActionContainer(CT_ToolBar, id), m_toolBar(0)
{
    setEmptyAction(EA_None);
}

/*!
    ...
*/
void ToolBarActionContainer::setToolBar(QToolBar *toolBar)
{
    m_toolBar = toolBar;
}

/*!
    ...
*/
QToolBar *ToolBarActionContainer::toolBar() const
{
    return m_toolBar;
}

/*!
    ...
*/
void ToolBarActionContainer::insertAction(QAction *before, QAction *action)
{
    m_toolBar->insertAction(before, action);
}

/*!
    ...
*/
void ToolBarActionContainer::insertMenu(QAction *, QMenu *)
{
    // not implemented
}

/*!
    ...
*/
bool ToolBarActionContainer::update()
{
    if (hasEmptyAction(EA_None))
        return true;

    bool hasitems = false;
    foreach (ICommand *command, commands()) {
        if (command->isActive()) {
            hasitems = true;
            break;
        }
    }

    if (hasEmptyAction(EA_Hide))
        m_toolBar->setVisible(hasitems);
    else if (hasEmptyAction(EA_Disable))
        m_toolBar->setEnabled(hasitems);

    return hasitems;
}

// ---------- MenuBarActionContainer ------------

/*!
    \class MenuBarActionContainer
    \ingroup qwb
    \inheaderfile actioncontainer.h
*/

/*!
    ...
*/
MenuBarActionContainer::MenuBarActionContainer(int id)
    : ActionContainer(CT_ToolBar, id), m_menuBar(0)
{
    setEmptyAction(EA_None);
}

/*!
    ...
*/
void MenuBarActionContainer::setMenuBar(QMenuBar *menuBar)
{
    m_menuBar = menuBar;
}

/*!
    ...
*/
QMenuBar *MenuBarActionContainer::menuBar() const
{
    return m_menuBar;
}

/*!
    ...
*/
void MenuBarActionContainer::insertAction(QAction *before, QAction *action)
{
    m_menuBar->insertAction(before, action);
}

/*!
    ...
*/
void MenuBarActionContainer::insertMenu(QAction *before, QMenu *menu)
{
    m_menuBar->insertMenu(before, menu);
}

/*!
    ...
*/
bool MenuBarActionContainer::update()
{
    if (hasEmptyAction(EA_None))
        return true;

    bool hasitems = false;
    QList<QAction *> actions = m_menuBar->actions();
    for (int i=0; i<actions.size(); ++i) {
        if (actions.at(i)->isVisible()) {
            hasitems = true;
            break;
        }
    }

    if (hasEmptyAction(EA_Hide))
        m_menuBar->setVisible(hasitems);
    else if (hasEmptyAction(EA_Disable))
        m_menuBar->setEnabled(hasitems);

    return hasitems;
}
