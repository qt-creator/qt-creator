// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "actionmanager_p.h"

#include "actioncontainer.h"
#include "command.h"

#include <utils/touchbar/touchbar.h>

namespace Core {
namespace Internal {

struct Group
{
    Group(Utils::Id id) : id(id) {}
    Utils::Id id;
    QList<QObject *> items; // Command * or ActionContainer *
};

class ActionContainerPrivate : public ActionContainer
{
    Q_OBJECT

public:
    ActionContainerPrivate(Utils::Id id, ActionManagerPrivate *actionManagerPrivate);
    ~ActionContainerPrivate() override = default;

    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) final;
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const final;

    QAction *insertLocation(Utils::Id groupId) const final;
    void appendGroup(Utils::Id id) final;
    void insertGroup(Utils::Id before, Utils::Id groupId) final;
    void addAction(Command *action, Utils::Id group = {}) final;
    void addMenu(ActionContainer *menu, Utils::Id group = {}) final;
    void addMenu(ActionContainer *before, ActionContainer *menu) final;
    Command *addSeparator(const Context &context, Utils::Id group = {}, QAction **outSeparator = nullptr) final;
    void clear() final;

    Utils::Id id() const final;

    QMenu *menu() const override;
    QMenuBar *menuBar() const override;
    Utils::TouchBar *touchBar() const override;

    virtual QAction *containerAction() const = 0;
    virtual QAction *actionForItem(QObject *item) const;

    virtual void insertAction(QAction *before, Command *command) = 0;
    virtual void insertMenu(QAction *before, ActionContainer *container) = 0;

    virtual void removeAction(Command *command) = 0;
    virtual void removeMenu(ActionContainer *container) = 0;

    virtual bool update() = 0;

protected:
    static bool canAddAction(Command *action);
    bool canAddMenu(ActionContainer *menu) const;
    virtual bool canBeAddedToContainer(ActionContainerPrivate *container) const = 0;

    // groupId --> list of Command* and ActionContainer*
    QList<Group> m_groups;

private:
    void scheduleUpdate();
    void itemDestroyed(QObject *sender);

    QList<Group>::const_iterator findGroup(Utils::Id groupId) const;
    QAction *insertLocation(QList<Group>::const_iterator group) const;

    OnAllDisabledBehavior m_onAllDisabledBehavior;
    Utils::Id m_id;
    ActionManagerPrivate *m_actionManagerPrivate = nullptr;
    bool m_updateRequested;
};

class MenuActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    explicit MenuActionContainer(Utils::Id id, ActionManagerPrivate *actionManagerPrivate);
    ~MenuActionContainer() override;

    QMenu *menu() const override;

    QAction *containerAction() const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

protected:
    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool update() override;

private:
    QPointer<QMenu> m_menu;
};

class MenuBarActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    explicit MenuBarActionContainer(Utils::Id id, ActionManagerPrivate *actionManagerPrivate);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const override;

    QAction *containerAction() const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

protected:
    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool update() override;

private:
    QMenuBar *m_menuBar;
};

class TouchBarActionContainer : public ActionContainerPrivate
{
    Q_OBJECT

public:
    TouchBarActionContainer(Utils::Id id,
                            ActionManagerPrivate *actionManagerPrivate,
                            const QIcon &icon,
                            const QString &text);
    ~TouchBarActionContainer() override;

    Utils::TouchBar *touchBar() const override;

    QAction *containerAction() const override;
    QAction *actionForItem(QObject *item) const override;

    void insertAction(QAction *before, Command *command) override;
    void insertMenu(QAction *before, ActionContainer *container) override;

    void removeAction(Command *command) override;
    void removeMenu(ActionContainer *container) override;

    bool canBeAddedToContainer(ActionContainerPrivate *container) const override;
    bool update() override;

private:
    std::unique_ptr<Utils::TouchBar> m_touchBar;
};

} // namespace Internal
} // namespace Core
