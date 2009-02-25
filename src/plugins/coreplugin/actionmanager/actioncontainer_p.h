/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef ACTIONCONTAINER_P_H
#define ACTIONCONTAINER_P_H

#include "actionmanager_p.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>

namespace Core {
namespace Internal {

class ActionContainerPrivate : public Core::ActionContainer
{
public:
    enum ContainerState {
        CS_None         = 0x000000,
        CS_Initialized  = 0x010000,
        CS_PreLocation  = 0x020000,
        CS_UserDefined  = 0x040000
    };

    ActionContainerPrivate(int id);
    virtual ~ActionContainerPrivate() {}

    void setEmptyAction(EmptyAction ea);
    bool hasEmptyAction(EmptyAction ea) const;

    void setState(ContainerState state);
    bool hasState(ContainerState state) const;

    QAction *insertLocation(const QString &group) const;
    void appendGroup(const QString &group);
    void addAction(Command *action, const QString &group = QString());
    void addMenu(ActionContainer *menu, const QString &group = QString());

    int id() const;

    QMenu *menu() const;
    QMenuBar *menuBar() const;

    virtual void insertAction(QAction *before, QAction *action) = 0;
    virtual void insertMenu(QAction *before, QMenu *menu) = 0;

    QList<Command *> commands() const { return m_commands; }
    QList<ActionContainer *> subContainers() const { return m_subContainers; }
protected:
    bool canAddAction(Command *action) const;
    bool canAddMenu(ActionContainer *menu) const;
    virtual bool canBeAddedToMenu() const = 0;

    void addAction(Command *action, int pos, bool setpos);
    void addMenu(ActionContainer *menu, int pos, bool setpos);

private:
    QAction *beforeAction(int pos, int *prevKey) const;
    int calcPosition(int pos, int prevKey) const;

    QList<int> m_groups;
    int m_data;
    int m_id;
    QMap<int, int> m_posmap;
    QList<ActionContainer *> m_subContainers;
    QList<Command *> m_commands;
};

class MenuActionContainer : public ActionContainerPrivate
{
public:
    MenuActionContainer(int id);

    void setMenu(QMenu *menu);
    QMenu *menu() const;

    void setLocation(const CommandLocation &location);
    CommandLocation location() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);
    bool update();

protected:
    bool canBeAddedToMenu() const;
private:
    QMenu *m_menu;
    CommandLocation m_location;
};

class MenuBarActionContainer : public ActionContainerPrivate
{
public:
    MenuBarActionContainer(int id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);
    bool update();

protected:
    bool canBeAddedToMenu() const;
private:
    QMenuBar *m_menuBar;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONCONTAINER_P_H
