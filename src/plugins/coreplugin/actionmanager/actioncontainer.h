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

#ifndef ACTIONCONTAINER_H
#define ACTIONCONTAINER_H

#include "coreplugin/icontext.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QMenuBar;
class QAction;
QT_END_NAMESPACE

namespace Core {

class Command;

class ActionContainer : public QObject
{
    Q_OBJECT

public:
    enum OnAllDisabledBehavior {
        Disable,
        Hide,
        Show
    };

    virtual void setOnAllDisabledBehavior(OnAllDisabledBehavior behavior) = 0;
    virtual ActionContainer::OnAllDisabledBehavior onAllDisabledBehavior() const = 0;

    virtual Id id() const = 0;

    virtual QMenu *menu() const = 0;
    virtual QMenuBar *menuBar() const = 0;

    virtual QAction *insertLocation(const Id &group) const = 0;
    virtual void appendGroup(const Id &group) = 0;
    virtual void insertGroup(const Id &before, const Id &group) = 0;
    virtual void addAction(Command *action, const Id &group = Id()) = 0;
    virtual void addMenu(ActionContainer *menu, const Id &group = Id()) = 0;
    virtual void addMenu(ActionContainer *before, ActionContainer *menu, const Id &group = Id()) = 0;
    virtual Command *addSeparator(const Context &context, const Id &group = Id(), QAction **outSeparator = 0) = 0;

    // This clears this menu and submenus from all actions and submenus.
    // It does not destroy the submenus and commands, just removes them from their parents.
    virtual void clear() = 0;
};

} // namespace Core

#endif // ACTIONCONTAINER_H
