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
    ~ActionContainerPrivate() override {}

    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) override;
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const override;

    QAction *insertLocation(Id groupId) const override;
    void appendGroup(Id id) override;
    void insertGroup(Id before, Id groupId) override;
    void addAction(Command *action, Id group = Id()) override;
    void addMenu(ActionContainer *menu, Id group = Id()) override;
    void addMenu(ActionContainer *before, ActionContainer *menu, Id group = Id()) override;
    Command *addSeparator(const Context &context, Id group = Id(), QAction **outSeparator = nullptr) override;
    void clear() override;

    Id id() const override;

    QMenu *menu() const override;
    QMenuBar *menuBar() const override;

    virtual void insertAction(QAction *before, QAction *action) = 0;
    virtual void insertMenu(QAction *before, QMenu *menu) = 0;

    virtual void removeAction(QAction *action) = 0;
    virtual void removeMenu(QMenu *menu) = 0;

    virtual bool updateInternal() = 0;

protected:
    bool canAddAction(Command *action) const;
    bool canAddMenu(ActionContainer *menu) const;
    virtual bool canBeAddedToMenu() const = 0;

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
public:
    explicit MenuActionContainer(Id id);
    ~MenuActionContainer() override;

    QMenu *menu() const override;

    void insertAction(QAction *before, QAction *action) override;
    void insertMenu(QAction *before, QMenu *menu) override;

    void removeAction(QAction *action) override;
    void removeMenu(QMenu *menu) override;

protected:
    bool canBeAddedToMenu() const override;
    bool updateInternal() override;

private:
    QPointer<QMenu> m_menu;
};

class MenuBarActionContainer : public ActionContainerPrivate
{
public:
    explicit MenuBarActionContainer(Id id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const override;

    void insertAction(QAction *before, QAction *action) override;
    void insertMenu(QAction *before, QMenu *menu) override;

    void removeAction(QAction *action) override;
    void removeMenu(QMenu *menu) override;

protected:
    bool canBeAddedToMenu() const override;
    bool updateInternal() override;

private:
    QMenuBar *m_menuBar;
};

} // namespace Internal
} // namespace Core
