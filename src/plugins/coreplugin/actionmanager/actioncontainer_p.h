/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ACTIONCONTAINER_P_H
#define ACTIONCONTAINER_P_H

#include "actionmanager_p.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

namespace Core {
namespace Internal {

struct Group
{
    Group(const Id &id) : id(id) {}
    Id id;
    QList<QObject *> items; // Command * or ActionContainer *
};

class ActionContainerPrivate : public Core::ActionContainer
{
    Q_OBJECT

public:
    ActionContainerPrivate(Id id);
    ~ActionContainerPrivate() {}

    void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior);
    ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const;

    QAction *insertLocation(const Id &groupId) const;
    void appendGroup(const Id &id);
    void insertGroup(const Id &before, const Id &groupId);
    void addAction(Command *action, const Id &group = Id());
    void addMenu(ActionContainer *menu, const Id &group = Id());
    void addMenu(ActionContainer *before, ActionContainer *menu, const Id &group = Id());
    Command *addSeparator(const Context &context, const Id &group = Id(), QAction **outSeparator = 0);
    virtual void clear();

    Id id() const;

    QMenu *menu() const;
    QMenuBar *menuBar() const;

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

private slots:
    void scheduleUpdate();
    void update();
    void itemDestroyed();

private:
    QList<Group>::const_iterator findGroup(const Id &groupId) const;
    QAction *insertLocation(QList<Group>::const_iterator group) const;

    OnAllDisabledBehavior m_onAllDisabledBehavior;
    Id m_id;
    bool m_updateRequested;
};

class MenuActionContainer : public ActionContainerPrivate
{
public:
    explicit MenuActionContainer(Id id);

    void setMenu(QMenu *menu);
    QMenu *menu() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);

    void removeAction(QAction *action);
    void removeMenu(QMenu *menu);

protected:
    bool canBeAddedToMenu() const;
    bool updateInternal();

private:
    QMenu *m_menu;
};

class MenuBarActionContainer : public ActionContainerPrivate
{
public:
    explicit MenuBarActionContainer(Id id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);

    void removeAction(QAction *action);
    void removeMenu(QMenu *menu);

protected:
    bool canBeAddedToMenu() const;
    bool updateInternal();

private:
    QMenuBar *m_menuBar;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONCONTAINER_P_H
