/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef ACTIONCONTAINER_H
#define ACTIONCONTAINER_H

#include <QtCore/QObject>

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

    virtual int id() const = 0;

    virtual QMenu *menu() const = 0;
    virtual QMenuBar *menuBar() const = 0;

    virtual QAction *insertLocation(const QString &group) const = 0;
    virtual void appendGroup(const QString &group) = 0;
    virtual void addAction(Core::Command *action, const QString &group = QString()) = 0;
    virtual void addMenu(Core::ActionContainer *menu, const QString &group = QString()) = 0;

    // clears this menu and submenus from all actions and submenus
    // doesn't destroy the submenus and commands, just removes them from their parents
    virtual void clear() = 0;
    virtual ~ActionContainer() {}
};

} // namespace Core

#endif // ACTIONCONTAINER_H
