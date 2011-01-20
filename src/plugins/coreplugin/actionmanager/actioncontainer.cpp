/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "actioncontainer_p.h"
#include "actionmanager_p.h"

#include "command_p.h"

#include "coreconstants.h"
#include "uniqueidmanager.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTimer>
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
    \l{ActionContainer::setOnAllDisabledBehavior()}{OnAllDisabledBehavior}. The default is
    ActionContainer::Disable for menus, and ActionContainer::Show for menu bars.
*/

/*!
    \enum ActionContainer::OnAllDisabledBehavior
    Defines what happens when the represented menu is empty or contains only disabled/invisible items.
    \value Disable
        The menu will be visible but disabled.
    \value Hide
        The menu will not be visible until the state of the subitems change.
    \value Show
        The menu will still be visible and active.
*/

/*!
    \fn ActionContainer::setOnAllDisabledBehavior(OnAllDisabledBehavior behavior)
    Defines the \a behavior of the menu represented by this action container for the case
    whenever it only contains disabled items and submenus.
    The default is ActionContainer::Disable for menus, and ActionContainer::Show for menu bars.
    \sa ActionContainer::OnAllDisabledBehavior
    \sa ActionContainer::onAllDisabledBehavior()
*/

/*!
    \fn ActionContainer::onAllDisabledBehavior() const
    Returns the \a behavior of the menu represented by this action container for the case
    whenever it only contains disabled items and submenus.
    The default is ActionContainer::Disable for menus, and ActionContainer::Show for menu bars.
    \sa ActionContainer::OnAllDisabledBehavior
    \sa ActionContainer::setOnAllDisabledBehavior()
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
    \fn ActionContainer::~ActionContainer()
    \internal
*/

// ---------- ActionContainerPrivate ------------

/*!
    \class Core::Internal::ActionContainerPrivate
    \internal
*/

ActionContainerPrivate::ActionContainerPrivate(int id)
    : m_onAllDisabledBehavior(Disable), m_id(id), m_updateRequested(false)
{
    appendGroup(QLatin1String(Constants::G_DEFAULT_ONE));
    appendGroup(QLatin1String(Constants::G_DEFAULT_TWO));
    appendGroup(QLatin1String(Constants::G_DEFAULT_THREE));
    scheduleUpdate();
}

void ActionContainerPrivate::setOnAllDisabledBehavior(OnAllDisabledBehavior behavior)
{
    m_onAllDisabledBehavior = behavior;
}

ActionContainer::OnAllDisabledBehavior ActionContainerPrivate::onAllDisabledBehavior() const
{
    return m_onAllDisabledBehavior;
}

void ActionContainerPrivate::appendGroup(const QString &groupId)
{
    m_groups.append(Group(groupId));
}

QList<Group>::const_iterator ActionContainerPrivate::findGroup(const QString &groupId) const
{
    QList<Group>::const_iterator it = m_groups.constBegin();
    while (it != m_groups.constEnd()) {
        if (it->id == groupId)
            break;
        ++it;
    }
    return it;
}


QAction *ActionContainerPrivate::insertLocation(const QString &groupId) const
{
    QList<Group>::const_iterator it = findGroup(groupId);
    QTC_ASSERT(it != m_groups.constEnd(), return 0);
    return insertLocation(it);
}

QAction *ActionContainerPrivate::insertLocation(QList<Group>::const_iterator group) const
{
    if (group == m_groups.constEnd())
        return 0;
    ++group;
    while (group != m_groups.constEnd()) {
        if (!group->items.isEmpty()) {
            QObject *item = group->items.first();
            if (Command *cmd = qobject_cast<Command *>(item)) {
                return cmd->action();
            } else if (ActionContainer *container = qobject_cast<ActionContainer *>(item)) {
                if (container->menu())
                    return container->menu()->menuAction();
            }
            QTC_ASSERT(false, return 0);
        }
        ++group;
    }
    return 0;
}

void ActionContainerPrivate::addAction(Command *command, const QString &groupId)
{
    if (!canAddAction(command))
        return;

    QString actualGroupId;
    if (groupId.isEmpty())
        actualGroupId = QLatin1String(Constants::G_DEFAULT_TWO);
    else
        actualGroupId = groupId;

    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    QAction *beforeAction = insertLocation(groupIt);
    m_groups[groupIt-m_groups.constBegin()].items.append(command);

    connect(command, SIGNAL(activeStateChanged()), this, SLOT(scheduleUpdate()));
    connect(command, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
    insertAction(beforeAction, command->action());
    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *menu, const QString &groupId)
{
    ActionContainerPrivate *containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    if (!containerPrivate->canBeAddedToMenu())
        return;
    MenuActionContainer *container = static_cast<MenuActionContainer *>(containerPrivate);

    QString actualGroupId;
    if (groupId.isEmpty())
        actualGroupId = QLatin1String(Constants::G_DEFAULT_TWO);
    else
        actualGroupId = groupId;

    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    QAction *beforeAction = insertLocation(groupIt);
    m_groups[groupIt-m_groups.constBegin()].items.append(menu);

    insertMenu(beforeAction, container->menu());
    scheduleUpdate();
}

void ActionContainerPrivate::itemDestroyed()
{
    QObject *obj = sender();
    QMutableListIterator<Group> it(m_groups);
    while (it.hasNext()) {
        Group &group = it.next();
        if (group.items.removeAll(obj) > 0)
            break;
    }
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

void ActionContainerPrivate::scheduleUpdate()
{
    if (m_updateRequested)
        return;
    m_updateRequested = true;
    QTimer::singleShot(0, this, SLOT(update()));
}

void ActionContainerPrivate::update()
{
    updateInternal();
    m_updateRequested = false;
}

// ---------- MenuActionContainer ------------

/*!
    \class Core::Internal::MenuActionContainer
    \internal
*/

MenuActionContainer::MenuActionContainer(int id)
    : ActionContainerPrivate(id), m_menu(0)
{
    setOnAllDisabledBehavior(Disable);
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

bool MenuActionContainer::updateInternal()
{
    if (onAllDisabledBehavior() == Show)
        return true;

    bool hasitems = false;
    QList<QAction *> actions = m_menu->actions();

    QListIterator<Group> it(m_groups);
    while (it.hasNext()) {
        const Group &group = it.next();
        foreach (QObject *item, group.items) {
            if (ActionContainerPrivate *container = qobject_cast<ActionContainerPrivate*>(item)) {
                actions.removeAll(container->menu()->menuAction());
                if (container == this) {
                    qWarning() << Q_FUNC_INFO << "container" << (this->menu() ? this->menu()->title() : "") <<  "contains itself as subcontainer";
                    continue;
                }
                if (container->updateInternal()) {
                    hasitems = true;
                    break;
                }
            } else if (Command *command = qobject_cast<Command *>(item)) {
                actions.removeAll(command->action());
                if (command->isActive()) {
                    hasitems = true;
                    break;
                }
            } else {
                QTC_ASSERT(false, continue);
            }
        }
        if (hasitems)
            break;
    }
    if (!hasitems) {
        // look if there were actions added that we don't control and check if they are enabled
        foreach (const QAction *action, actions) {
            if (!action->isSeparator() && action->isEnabled()) {
                hasitems = true;
                break;
            }
        }
    }

    if (onAllDisabledBehavior() == Hide)
        m_menu->menuAction()->setVisible(hasitems);
    else if (onAllDisabledBehavior() == Disable)
        m_menu->menuAction()->setEnabled(hasitems);

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
    setOnAllDisabledBehavior(Show);
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

bool MenuBarActionContainer::updateInternal()
{
    if (onAllDisabledBehavior() == Show)
        return true;

    bool hasitems = false;
    QList<QAction *> actions = m_menuBar->actions();
    for (int i=0; i<actions.size(); ++i) {
        if (actions.at(i)->isVisible()) {
            hasitems = true;
            break;
        }
    }

    if (onAllDisabledBehavior() == Hide)
        m_menuBar->setVisible(hasitems);
    else if (onAllDisabledBehavior() == Disable)
        m_menuBar->setEnabled(hasitems);

    return hasitems;
}

bool MenuBarActionContainer::canBeAddedToMenu() const
{
    return false;
}

