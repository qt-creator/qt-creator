/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "actioncontainer_p.h"
#include "actionmanager_p.h"

#include "command_p.h"

#include "coreconstants.h"
#include "uniqueidmanager.h"

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>

Q_DECLARE_METATYPE(Core::Internal::MenuActionContainer*)

using namespace Core;
using namespace Core::Internal;

/*!
    \class ActionContainer
    \mainclass

    \brief The ActionContainer class represents a menu or menu bar in Qt Creator.

    You don't create instances of this class directly, but instead use the
    \l{ActionManager::createMenu()}
    and \l{ActionManager::createMenuBar()} methods.
    Retrieve existing action containers for an ID with
    \l{ActionManager::actionContainer()}.

    Within a menu or menu bar you can group menus and items together by defining groups
    (the order of the groups is defined by the order of the \l{ActionContainer::appendGroup()} calls), and
    adding menus/actions to these groups. If no custom groups are defined, an action container
    has three default groups \c{Core::Constants::G_DEFAULT_ONE}, \c{Core::Constants::G_DEFAULT_TWO}
    and \c{Core::Constants::G_DEFAULT_THREE}.

    You can define if the menu represented by this action container should automatically disable
    or hide whenever it only contains disabled items and submenus by setting the corresponding
    \l{ActionContainer::setEmptyAction()}{EmptyAction}.
*/

/*!
    \enum ActionContainer::EmptyAction
    Defines what happens when the represented menu is empty or contains only disabled/invisible items.
    \omitvalue EA_Mask
    \value EA_None
        The menu will still be visible and active.
    \value EA_Disable
        The menu will be visible but disabled.
    \value EA_Hide
        The menu will not be visible until the state of the subitems change.
*/

/*!
    \fn ActionContainer::setEmptyAction(EmptyAction disableOrHide)
    Defines if the menu represented by this action container should automatically \a disableOrHide
    whenever it only contains disabled items and submenus.
    \sa ActionContainer::EmptyAction
*/

/*!
    \fn int ActionContainer::id() const
    \internal
*/

/*!
    \fn QMenu *ActionContainer::menu() const
    Returns the QMenu instance that is represented by this action container, or
    0 if this action container represents a menu bar.
*/

/*!
    \fn QMenuBar *ActionContainer::menuBar() const
    Returns the QMenuBar instance that is represented by this action container, or
    0 if this action container represents a menu.
*/

/*!
    \fn QAction *ActionContainer::insertLocation(const QString &group) const
    Returns an action representing the \a group,
    that could be used with \c{QWidget::insertAction}.
*/

/*!
    \fn void ActionContainer::appendGroup(const QString &identifier)
    Adds a group with the given \a identifier to the action container. Using groups
    you can segment your action container into logical parts and add actions and
    menus directly to these parts.
    \sa addAction()
    \sa addMenu()
*/

/*!
    \fn void ActionContainer::addAction(Core::Command *action, const QString &group)
    Add the \a action as a menu item to this action container. The action is added as the
    last item of the specified \a group.
    \sa appendGroup()
    \sa addMenu()
*/

/*!
    \fn void ActionContainer::addMenu(Core::ActionContainer *menu, const QString &group)
    Add the \a menu as a submenu to this action container. The menu is added as the
    last item of the specified \a group.
    \sa appendGroup()
    \sa addAction()
*/

/*!
    \fn bool ActionContainer::update()
    \internal
*/

/*!
    \fn ActionContainer::~ActionContainer()
    \internal
*/

// ---------- ActionContainerPrivate ------------

/*!
    \class Core::Internal::ActionContainerPrivate
    \internal
*/

ActionContainerPrivate::ActionContainerPrivate(int id)
    : m_data(0), m_id(id)
{

}

void ActionContainerPrivate::setEmptyAction(EmptyAction ea)
{
    m_data = ((m_data & ~EA_Mask) | ea);
}

bool ActionContainerPrivate::hasEmptyAction(EmptyAction ea) const
{
    return (m_data & EA_Mask) == ea;
}

void ActionContainerPrivate::appendGroup(const QString &group)
{
    int gid = UniqueIDManager::instance()->uniqueIdentifier(group);
    m_groups << gid;
}

QAction *ActionContainerPrivate::insertLocation(const QString &group) const
{
    int grpid = UniqueIDManager::instance()->uniqueIdentifier(group);
    int prevKey = 0;
    int pos = ((grpid << 16) | 0xFFFF);
    return beforeAction(pos, &prevKey);
}

void ActionContainerPrivate::addAction(Command *action, const QString &group)
{
    if (!canAddAction(action))
        return;

    ActionManagerPrivate *am = ActionManagerPrivate::instance();
    UniqueIDManager *idmanager = UniqueIDManager::instance();
    int grpid = idmanager->uniqueIdentifier(Constants::G_DEFAULT_TWO);
    if (!group.isEmpty())
        grpid = idmanager->uniqueIdentifier(group);
    if (!m_groups.contains(grpid) && !am->defaultGroups().contains(grpid))
        qWarning() << "*** addAction(): Unknown group: " << group;
    int pos = ((grpid << 16) | 0xFFFF);
    addAction(action, pos, true);
}

void ActionContainerPrivate::addMenu(ActionContainer *menu, const QString &group)
{
    ActionContainerPrivate *container = static_cast<ActionContainerPrivate *>(menu);
    if (!container->canBeAddedToMenu())
        return;

    ActionManagerPrivate *am = ActionManagerPrivate::instance();
    UniqueIDManager *idmanager = UniqueIDManager::instance();
    int grpid = idmanager->uniqueIdentifier(Constants::G_DEFAULT_TWO);
    if (!group.isEmpty())
        grpid = idmanager->uniqueIdentifier(group);
    if (!m_groups.contains(grpid) && !am->defaultGroups().contains(grpid))
        qWarning() << "*** addMenu(): Unknown group: " << group;
    int pos = ((grpid << 16) | 0xFFFF);
    addMenu(menu, pos, true);
}

int ActionContainerPrivate::id() const
{
    return m_id;
}

QMenu *ActionContainerPrivate::menu() const
{
    return 0;
}

QMenuBar *ActionContainerPrivate::menuBar() const
{
    return 0;
}

bool ActionContainerPrivate::canAddAction(Command *action) const
{
    return (action->action() != 0);
}

void ActionContainerPrivate::addAction(Command *action, int pos, bool setpos)
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

void ActionContainerPrivate::addMenu(ActionContainer *menu, int pos, bool setpos)
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

QAction *ActionContainerPrivate::beforeAction(int pos, int *prevKey) const
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

    if (Command *cmd = am->command(baId))
        return cmd->action();
    if (ActionContainer *container = am->actionContainer(baId))
        if (QMenu *menu = container->menu())
            return menu->menuAction();

    return 0;
}

int ActionContainerPrivate::calcPosition(int pos, int prevKey) const
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
    \class Core::Internal::MenuActionContainer
    \internal
*/

MenuActionContainer::MenuActionContainer(int id)
    : ActionContainerPrivate(id), m_menu(0)
{
    setEmptyAction(EA_Disable);
}

void MenuActionContainer::setMenu(QMenu *menu)
{
    m_menu = menu;

    QVariant v;
    qVariantSetValue<MenuActionContainer*>(v, this);

    m_menu->menuAction()->setData(v);
}

QMenu *MenuActionContainer::menu() const
{
    return m_menu;
}

void MenuActionContainer::insertAction(QAction *before, QAction *action)
{
    m_menu->insertAction(before, action);
}

void MenuActionContainer::insertMenu(QAction *before, QMenu *menu)
{
    m_menu->insertMenu(before, menu);
}

void MenuActionContainer::setLocation(const CommandLocation &location)
{
    m_location = location;
}

CommandLocation MenuActionContainer::location() const
{
    return m_location;
}

bool MenuActionContainer::update()
{
    if (hasEmptyAction(EA_None))
        return true;

    bool hasitems = false;

    foreach (ActionContainer *container, subContainers()) {
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
        foreach (Command *command, commands()) {
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

bool MenuActionContainer::canBeAddedToMenu() const
{
    return true;
}


// ---------- MenuBarActionContainer ------------

/*!
    \class Core::Internal::MenuBarActionContainer
    \internal
*/

MenuBarActionContainer::MenuBarActionContainer(int id)
    : ActionContainerPrivate(id), m_menuBar(0)
{
    setEmptyAction(EA_None);
}

void MenuBarActionContainer::setMenuBar(QMenuBar *menuBar)
{
    m_menuBar = menuBar;
}

QMenuBar *MenuBarActionContainer::menuBar() const
{
    return m_menuBar;
}

void MenuBarActionContainer::insertAction(QAction *before, QAction *action)
{
    m_menuBar->insertAction(before, action);
}

void MenuBarActionContainer::insertMenu(QAction *before, QMenu *menu)
{
    m_menuBar->insertMenu(before, menu);
}

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

bool MenuBarActionContainer::canBeAddedToMenu() const
{
    return false;
}

