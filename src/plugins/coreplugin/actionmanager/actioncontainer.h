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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef ACTIONCONTAINER_H
#define ACTIONCONTAINER_H

#include "actionmanager.h"

#include <coreplugin/actionmanager/iactioncontainer.h>
#include <coreplugin/actionmanager/icommand.h>

namespace Core {
namespace Internal {

class ActionContainer : public Core::IActionContainer
{
public:
    enum ContainerState {
        CS_None         = 0x000000,
        CS_Initialized  = 0x010000,
        CS_PreLocation  = 0x020000,
        CS_UserDefined  = 0x040000
    };

    ActionContainer(ContainerType type, int id);
    virtual ~ActionContainer() {}

    void setEmptyAction(EmptyAction ea);
    bool hasEmptyAction(EmptyAction ea) const;

    void setState(ContainerState state);
    bool hasState(ContainerState state) const;

    QAction *insertLocation(const QString &group) const;
    void appendGroup(const QString &group, bool global = false);
    void addAction(ICommand *action, const QString &group = QString());
    void addMenu(IActionContainer *menu, const QString &group = QString());

    int id() const;
    ContainerType type() const;

    QMenu *menu() const;
    QToolBar *toolBar() const;
    QMenuBar *menuBar() const;

    virtual void insertAction(QAction *before, QAction *action) = 0;
    virtual void insertMenu(QAction *before, QMenu *menu) = 0;

    QList<ICommand *> commands() const { return m_commands; }
    QList<IActionContainer *> subContainers() const { return m_subContainers; }
protected:
    bool canAddAction(ICommand *action) const;
    bool canAddMenu(IActionContainer *menu) const;

    void addAction(ICommand *action, int pos, bool setpos);
    void addMenu(IActionContainer *menu, int pos, bool setpos);

private:
    QAction *beforeAction(int pos, int *prevKey) const;
    int calcPosition(int pos, int prevKey) const;

    QList<int> m_groups;
    int m_data;
    ContainerType m_type;
    int m_id;
    QMap<int, int> m_posmap;
    QList<IActionContainer *> m_subContainers;
    QList<ICommand *> m_commands;
};

class MenuActionContainer : public ActionContainer
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

private:
    QMenu *m_menu;
    CommandLocation m_location;
};

class ToolBarActionContainer : public ActionContainer
{
public:
    ToolBarActionContainer(int id);

    void setToolBar(QToolBar *toolBar);
    QToolBar *toolBar() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);
    bool update();

private:
    QToolBar *m_toolBar;
};

class MenuBarActionContainer : public ActionContainer
{
public:
    MenuBarActionContainer(int id);

    void setMenuBar(QMenuBar *menuBar);
    QMenuBar *menuBar() const;

    void insertAction(QAction *before, QAction *action);
    void insertMenu(QAction *before, QMenu *menu);
    bool update();

private:
    QMenuBar *m_menuBar;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONCONTAINER_H
