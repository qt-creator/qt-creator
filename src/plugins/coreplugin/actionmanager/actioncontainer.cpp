/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "actioncontainer_p.h"
#include "actionmanager_p.h"
#include "actionmanager.h"

#include "command_p.h"

#include "coreconstants.h"
#include "id.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QMenuBar>

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
    \fn QAction *ActionContainer::insertLocation(const Core::Id &group) const
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
    \fn void ActionContainer::addAction(Core::Command *action, const Core::Id &group)
    Add the \a action as a menu item to this action container. The action is added as the
    last item of the specified \a group.
    \sa appendGroup()
    \sa addMenu()
*/

/*!
    \fn void ActionContainer::addMenu(Core::ActionContainer *menu, const Core::Id &group)
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

ActionContainerPrivate::ActionContainerPrivate(Id id)
    : m_onAllDisabledBehavior(Disable), m_id(id), m_updateRequested(false)
{
    appendGroup(Constants::G_DEFAULT_ONE);
    appendGroup(Constants::G_DEFAULT_TWO);
    appendGroup(Constants::G_DEFAULT_THREE);
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

void ActionContainerPrivate::appendGroup(const Id &groupId)
{
    m_groups.append(Group(groupId));
}

void ActionContainerPrivate::insertGroup(const Id &before, const Id &groupId)
{
    QList<Group>::iterator it = m_groups.begin();
    while (it != m_groups.end()) {
        if (it->id == before) {
            m_groups.insert(it, Group(groupId));
            break;
        }
        ++it;
    }
}

QList<Group>::const_iterator ActionContainerPrivate::findGroup(const Id &groupId) const
{
    QList<Group>::const_iterator it = m_groups.constBegin();
    while (it != m_groups.constEnd()) {
        if (it->id == groupId)
            break;
        ++it;
    }
    return it;
}


QAction *ActionContainerPrivate::insertLocation(const Id &groupId) const
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

void ActionContainerPrivate::addAction(Command *command, const Id &groupId)
{
    if (!canAddAction(command))
        return;

    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), qDebug() << "Can't find group"
               << groupId.name() << "in container" << id().name(); return);
    QAction *beforeAction = insertLocation(groupIt);
    m_groups[groupIt-m_groups.constBegin()].items.append(command);

    connect(command, SIGNAL(activeStateChanged()), this, SLOT(scheduleUpdate()));
    connect(command, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
    insertAction(beforeAction, command->action());
    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *menu, const Id &groupId)
{
    ActionContainerPrivate *containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    if (!containerPrivate->canBeAddedToMenu())
        return;

    MenuActionContainer *container = static_cast<MenuActionContainer *>(containerPrivate);
    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    QAction *beforeAction = insertLocation(groupIt);
    m_groups[groupIt-m_groups.constBegin()].items.append(menu);

    connect(menu, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
    insertMenu(beforeAction, container->menu());
    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *before, ActionContainer *menu, const Id &groupId)
{
    ActionContainerPrivate *containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    if (!containerPrivate->canBeAddedToMenu())
        return;

    MenuActionContainer *container = static_cast<MenuActionContainer *>(containerPrivate);
    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    QAction *beforeAction = before->menu()->menuAction();
    m_groups[groupIt-m_groups.constBegin()].items.append(menu);

    connect(menu, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
    insertMenu(beforeAction, container->menu());
    scheduleUpdate();
}

/*!
 * \fn Command *ActionContainer::addSeparator(const Context &context, const Id &group, QAction **outSeparator)
 *
 * Adds a separator to the end of the given \a group to the action container, which is enabled
 * for a given \a context. The created separator action is returned through \a outSeparator.
 *
 * Returns the created Command for the separator.
 */
/*! \a context \a group \a outSeparator
 * \internal
 */
Command *ActionContainerPrivate::addSeparator(const Context &context, const Id &group, QAction **outSeparator)
{
    static int separatorIdCount = 0;
    QAction *separator = new QAction(this);
    separator->setSeparator(true);
    Id sepId = id().withSuffix(".Separator.").withSuffix(++separatorIdCount);
    Command *cmd = ActionManager::registerAction(separator, sepId, context);
    addAction(cmd, group);
    if (outSeparator)
        *outSeparator = separator;
    return cmd;
}

void ActionContainerPrivate::clear()
{
    QMutableListIterator<Group> it(m_groups);
    while (it.hasNext()) {
        Group &group = it.next();
        foreach (QObject *item, group.items) {
            if (Command *command = qobject_cast<Command *>(item)) {
                removeAction(command->action());
                disconnect(command, SIGNAL(activeStateChanged()), this, SLOT(scheduleUpdate()));
                disconnect(command, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
            } else if (ActionContainer *container = qobject_cast<ActionContainer *>(item)) {
                container->clear();
                disconnect(container, SIGNAL(destroyed()), this, SLOT(itemDestroyed()));
                removeMenu(container->menu());
            }
        }
        group.items.clear();
    }
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

Id ActionContainerPrivate::id() const
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
    return action && action->action();
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

MenuActionContainer::MenuActionContainer(Id id)
    : ActionContainerPrivate(id), m_menu(0)
{
    setOnAllDisabledBehavior(Disable);
}

void MenuActionContainer::setMenu(QMenu *menu)
{
    m_menu = menu;
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

void MenuActionContainer::removeAction(QAction *action)
{
    m_menu->removeAction(action);
}

void MenuActionContainer::removeMenu(QMenu *menu)
{
    m_menu->removeAction(menu->menuAction());
}

#ifdef Q_OS_MAC
static bool menuInMenuBar(const QMenu *menu)
{
    foreach (const QWidget *widget, menu->menuAction()->associatedWidgets()) {
        if (qobject_cast<const QMenuBar *>(widget))
            return true;
    }
    return false;
}
#endif

bool MenuActionContainer::updateInternal()
{
    if (onAllDisabledBehavior() == Show)
        return true;

#ifdef Q_OS_MAC
    // work around QTBUG-25544 which makes menus in the menu bar stay at their enabled state at startup
    // (so menus that are disabled at startup would stay disabled)
    if (menuInMenuBar(m_menu))
        return true;
#endif

    bool hasitems = false;
    QList<QAction *> actions = m_menu->actions();

    QListIterator<Group> it(m_groups);
    while (it.hasNext()) {
        const Group &group = it.next();
        foreach (QObject *item, group.items) {
            if (ActionContainerPrivate *container = qobject_cast<ActionContainerPrivate*>(item)) {
                actions.removeAll(container->menu()->menuAction());
                if (container == this) {
                    QByteArray warning = Q_FUNC_INFO + QByteArray(" container '");
                    if (this->menu())
                        warning += this->menu()->title().toLocal8Bit();
                    warning += "' contains itself as subcontainer";
                    qWarning("%s", warning.constData());
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

MenuBarActionContainer::MenuBarActionContainer(Id id)
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

void MenuBarActionContainer::removeAction(QAction *action)
{
    m_menuBar->removeAction(action);
}

void MenuBarActionContainer::removeMenu(QMenu *menu)
{
    m_menuBar->removeAction(menu->menuAction());
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

