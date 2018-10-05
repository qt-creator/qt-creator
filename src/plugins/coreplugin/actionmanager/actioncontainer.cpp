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

#include "actioncontainer_p.h"
#include "actionmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/id.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTimer>
#include <QAction>
#include <QMenuBar>

Q_DECLARE_METATYPE(Core::Internal::MenuActionContainer*)

using namespace Utils;

namespace Core {
namespace Internal {

/*!
    \class ActionContainer
    \mainclass

    \brief The ActionContainer class represents a menu or menu bar in Qt Creator.

    You don't create instances of this class directly, but instead use the
    \l{ActionManager::createMenu()}, \l{ActionManager::createMenuBar()} and
    \l{ActionManager::createTouchBar()} functions.
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
    \fn QAction *ActionContainer::insertLocation(Id group) const
    Returns an action representing the \a group,
    that could be used with \c{QWidget::insertAction}.
*/

/*!
    \fn void ActionContainer::appendGroup(Id group)
    Adds a group with the given \a identifier to the action container. Using groups
    you can segment your action container into logical parts and add actions and
    menus directly to these parts.
    \sa addAction()
    \sa addMenu()
*/

/*!
    \fn void ActionContainer::addAction(Command *action, Id group = Id())
    Add the \a action as a menu item to this action container. The action is added as the
    last item of the specified \a group.
    \sa appendGroup()
    \sa addMenu()
*/

/*!
    \fn void ActionContainer::addMenu(ActionContainer *menu, Id group = Id())
    Add the \a menu as a submenu to this action container. The menu is added as the
    last item of the specified \a group.
    \sa appendGroup()
    \sa addAction()
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

void ActionContainerPrivate::appendGroup(Id groupId)
{
    m_groups.append(Group(groupId));
}

void ActionContainerPrivate::insertGroup(Id before, Id groupId)
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

QList<Group>::const_iterator ActionContainerPrivate::findGroup(Id groupId) const
{
    QList<Group>::const_iterator it = m_groups.constBegin();
    while (it != m_groups.constEnd()) {
        if (it->id == groupId)
            break;
        ++it;
    }
    return it;
}


QAction *ActionContainerPrivate::insertLocation(Id groupId) const
{
    QList<Group>::const_iterator it = findGroup(groupId);
    QTC_ASSERT(it != m_groups.constEnd(), return nullptr);
    return insertLocation(it);
}

QAction *ActionContainerPrivate::actionForItem(QObject *item) const
{
    if (auto cmd = qobject_cast<Command *>(item)) {
        return cmd->action();
    } else if (auto container = qobject_cast<ActionContainerPrivate *>(item)) {
        if (container->containerAction())
            return container->containerAction();
    }
    QTC_ASSERT(false, return nullptr);
}

QAction *ActionContainerPrivate::insertLocation(QList<Group>::const_iterator group) const
{
    if (group == m_groups.constEnd())
        return nullptr;
    ++group;
    while (group != m_groups.constEnd()) {
        if (!group->items.isEmpty()) {
            QObject *item = group->items.first();
            QAction *action = actionForItem(item);
            if (action)
                return action;
        }
        ++group;
    }
    return nullptr;
}

void ActionContainerPrivate::addAction(Command *command, Id groupId)
{
    if (!canAddAction(command))
        return;

    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), qDebug() << "Can't find group"
               << groupId.name() << "in container" << id().name(); return);
    m_groups[groupIt - m_groups.constBegin()].items.append(command);
    connect(command, &Command::activeStateChanged, this, &ActionContainerPrivate::scheduleUpdate);
    connect(command, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    QAction *beforeAction = insertLocation(groupIt);
    insertAction(beforeAction, command);

    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *menu, Id groupId)
{
    auto containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    QTC_ASSERT(containerPrivate->canBeAddedToContainer(this), return);

    const Id actualGroupId = groupId.isValid() ? groupId : Id(Constants::G_DEFAULT_TWO);
    QList<Group>::const_iterator groupIt = findGroup(actualGroupId);
    QTC_ASSERT(groupIt != m_groups.constEnd(), return);
    m_groups[groupIt - m_groups.constBegin()].items.append(menu);
    connect(menu, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    QAction *beforeAction = insertLocation(groupIt);
    insertMenu(beforeAction, menu);

    scheduleUpdate();
}

void ActionContainerPrivate::addMenu(ActionContainer *before, ActionContainer *menu)
{
    auto containerPrivate = static_cast<ActionContainerPrivate *>(menu);
    QTC_ASSERT(containerPrivate->canBeAddedToContainer(this), return);

    QMutableListIterator<Group> it(m_groups);
    while (it.hasNext()) {
        Group &group = it.next();
        const int insertionPoint = group.items.indexOf(before);
        if (insertionPoint >= 0) {
            group.items.insert(insertionPoint, menu);
            break;
        }
    }
    connect(menu, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);

    auto beforePrivate = static_cast<ActionContainerPrivate *>(before);
    QAction *beforeAction = beforePrivate->containerAction();
    if (beforeAction)
        insertMenu(beforeAction, menu);

    scheduleUpdate();
}

/*!
 * Adds a separator to the end of the given \a group to the action container, which is enabled
 * for a given \a context. The created separator action is returned through \a outSeparator.
 *
 * Returns the created Command for the separator.
 */
/*! \a context \a group \a outSeparator
 * \internal
 */
Command *ActionContainerPrivate::addSeparator(const Context &context, Id group, QAction **outSeparator)
{
    static int separatorIdCount = 0;
    auto separator = new QAction(this);
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
            if (auto command = qobject_cast<Command *>(item)) {
                removeAction(command);
                disconnect(command, &Command::activeStateChanged,
                           this, &ActionContainerPrivate::scheduleUpdate);
                disconnect(command, &QObject::destroyed, this, &ActionContainerPrivate::itemDestroyed);
            } else if (auto container = qobject_cast<ActionContainer *>(item)) {
                container->clear();
                disconnect(container, &QObject::destroyed,
                           this, &ActionContainerPrivate::itemDestroyed);
                removeMenu(container);
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
    return nullptr;
}

QMenuBar *ActionContainerPrivate::menuBar() const
{
    return nullptr;
}

TouchBar *ActionContainerPrivate::touchBar() const
{
    return nullptr;
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
    QTimer::singleShot(0, this, &ActionContainerPrivate::update);
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
    : ActionContainerPrivate(id),
      m_menu(new QMenu)
{
    m_menu->setObjectName(id.toString());
    m_menu->menuAction()->setMenuRole(QAction::NoRole);
    setOnAllDisabledBehavior(Disable);
}

MenuActionContainer::~MenuActionContainer()
{
    delete m_menu;
}

QMenu *MenuActionContainer::menu() const
{
    return m_menu;
}

QAction *MenuActionContainer::containerAction() const
{
    return m_menu->menuAction();
}

void MenuActionContainer::insertAction(QAction *before, Command *command)
{
    m_menu->insertAction(before, command->action());
}

void MenuActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    menu->setParent(m_menu, menu->windowFlags()); // work around issues with Qt Wayland (QTBUG-68636)
    m_menu->insertMenu(before, menu);
}

void MenuActionContainer::removeAction(Command *command)
{
    m_menu->removeAction(command->action());
}

void MenuActionContainer::removeMenu(ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    m_menu->removeAction(menu->menuAction());
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
            if (auto container = qobject_cast<ActionContainerPrivate*>(item)) {
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
            } else if (auto command = qobject_cast<Command *>(item)) {
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

bool MenuActionContainer::canBeAddedToContainer(ActionContainerPrivate *container) const
{
    return qobject_cast<MenuActionContainer *>(container)
           || qobject_cast<MenuBarActionContainer *>(container);
}

// ---------- MenuBarActionContainer ------------

/*!
    \class Core::Internal::MenuBarActionContainer
    \internal
*/

MenuBarActionContainer::MenuBarActionContainer(Id id)
    : ActionContainerPrivate(id), m_menuBar(nullptr)
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

QAction *MenuBarActionContainer::containerAction() const
{
    return nullptr;
}

void MenuBarActionContainer::insertAction(QAction *before, Command *command)
{
    m_menuBar->insertAction(before, command->action());
}

void MenuBarActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
    menu->setParent(m_menuBar, menu->windowFlags()); // work around issues with Qt Wayland (QTBUG-68636)
    m_menuBar->insertMenu(before, menu);
}

void MenuBarActionContainer::removeAction(Command *command)
{
    m_menuBar->removeAction(command->action());
}

void MenuBarActionContainer::removeMenu(ActionContainer *container)
{
    QMenu *menu = container->menu();
    QTC_ASSERT(menu, return);
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

bool MenuBarActionContainer::canBeAddedToContainer(ActionContainerPrivate *) const
{
    return false;
}

// ---------- TouchBarActionContainer ------------

const char ID_PREFIX[] = "io.qt.qtcreator.";

TouchBarActionContainer::TouchBarActionContainer(Id id, const QIcon &icon, const QString &text)
    : ActionContainerPrivate(id),
      m_touchBar(std::make_unique<TouchBar>(id.withPrefix(ID_PREFIX).name(), icon, text))
{
}

TouchBarActionContainer::~TouchBarActionContainer() = default;

TouchBar *TouchBarActionContainer::touchBar() const
{
    return m_touchBar.get();
}

QAction *TouchBarActionContainer::containerAction() const
{
    return m_touchBar->touchBarAction();
}

QAction *TouchBarActionContainer::actionForItem(QObject *item) const
{
    if (Command *command = qobject_cast<Command *>(item))
        return command->touchBarAction();
    return ActionContainerPrivate::actionForItem(item);
}

void TouchBarActionContainer::insertAction(QAction *before, Command *command)
{
    m_touchBar->insertAction(before,
                             command->id().withPrefix(ID_PREFIX).name(),
                             command->touchBarAction());
}

void TouchBarActionContainer::insertMenu(QAction *before, ActionContainer *container)
{
    TouchBar *touchBar = container->touchBar();
    QTC_ASSERT(touchBar, return);
    m_touchBar->insertTouchBar(before, touchBar);
}

void TouchBarActionContainer::removeAction(Command *command)
{
    m_touchBar->removeAction(command->touchBarAction());
}

void TouchBarActionContainer::removeMenu(ActionContainer *container)
{
    TouchBar *touchBar = container->touchBar();
    QTC_ASSERT(touchBar, return);
    m_touchBar->removeTouchBar(touchBar);
}

bool TouchBarActionContainer::canBeAddedToContainer(ActionContainerPrivate *container) const
{
    return qobject_cast<TouchBarActionContainer *>(container);
}

bool TouchBarActionContainer::updateInternal()
{
    return false;
}

} // namespace Internal

Command *ActionContainer::addSeparator(Id group)
{
    static const Context context(Constants::C_GLOBAL);
    return addSeparator(context, group);
}

} // namespace Core
