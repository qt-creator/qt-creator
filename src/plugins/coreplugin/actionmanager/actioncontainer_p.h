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

#pragma once

#include "actionmanager_p.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

#include <utils/touchbar/touchbar.h>

namespace Core {
namespace Internal {

struct Group
{
    Group(Id id) : id(id) {}
    Id id;
    QList<QObject *> items; // Command * or ActionContainer *
};

class ActionContainerPrivate : public ActionContainer
{
    Q_OBJECT

public:
    ActionContainerPrivate(Id id);
    ~ActionContainerPrivate() override = default;

    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) override;
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const override;

    QAction *insertLocation(Id groupId) const override;
    void appendGroup(Id id) override;
    void insertGroup(Id before, Id groupId) override;
    void addAction(Command *action, Id group = Id()) override;
    void addMenu(ActionContainer *menu, Id group = Id()) override;
    void addMenu(ActionContainer *before, ActionContainer *menu) override;
    Command *addSeparator(const Context &context, Id group = Id(), QAction **outSeparator = nullptr) override;
    void clear() override;

    Id id() const override;

    QMenu *menu() const override;
    QMenuBar *menuBar() const override;
    Utils::TouchBar *touchBar() const override;

    virtual QAction *containerAction() const = 0;
    virtual QAction *actionForItem(QObject *item) const;

    virtual void insertAction(QAction *before, Command *command) = 0;
    virtual void insertMenu(QAction *before, ActionContainer *container) = 0;

    virtual void removeAction(Command *command) = 0;
    virtual void removeMenu(ActionContainer *container) = 0;

    virtual bool updateInternal() = 0;

protected:
    bool canAddAction(Command *action) const;
    bool canAddMenu(ActionContainer *menu) const;
    virtual bool canBeAddedToContainer(ActionContainerPrivate *container) const = 0;

    // groupId --> list of Command* and ActionContainer*
    QList<Group> m_groups;

private:
    void scheduleUpdate();
    void update();
    void itemDestroyed();

    QList<Group>::const_iterator findGroup(Id groupId) const;
    QAction *insertLocation(QList<Group>::const_iterator group) const;

    OnAllDisabledBehavior m_onAllDisabledBehavior;
    Id m_id;
    bool m_updateRequested;
};

class MenuActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    explicit MenuActionContainer(Id id);
    ~MenuActionContainer() override;

    QMenu *menu() const override;

    QAction *containerAction() const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

protected:
    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool updateInternal() override;

private:
    QPointer<QMenu> m_menu;
};

class MenuBarActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    explicit MenuBarActionContainer(Id id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const override;

    QAction *containerAction() const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

protected:
    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool updateInternal() override;

private:
    QMenuBar *m_menuBar;
};

class TouchBarActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    TouchBarActionContainer(Id id, const QIcon &icon, const QString &text);
    ~TouchBarActionContainer() override;

    Utils::TouchBar *touchBar() const override;

    QAction *containerAction() const override;
    QAction *actionForItem(QObject *item) const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool updateInternal() override;

private:
    std::unique_ptr<Utils::TouchBar> m_touchBar;
};

} // namespace Internal
} // namespace Core
