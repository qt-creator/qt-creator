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

#include "../core_global.h"
#include "../icontext.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QMenu;
class QMenuBar;
class QAction;
QT_END_NAMESPACE

namespace Utils { class TouchBar; }

namespace Core {

class Command;

class CORE_EXPORT ActionContainer : public QObject
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

    virtual Utils::Id id() const = 0;

    virtual QMenu *menu() const = 0;
    virtual QMenuBar *menuBar() const = 0;
    virtual Utils::TouchBar *touchBar() const = 0;

    virtual QAction *insertLocation(Utils::Id group) const = 0;
    virtual void appendGroup(Utils::Id group) = 0;
    virtual void insertGroup(Utils::Id before, Utils::Id group) = 0;
    virtual void addAction(Command *action, Utils::Id group = {}) = 0;
    virtual void addMenu(ActionContainer *menu, Utils::Id group = {}) = 0;
    virtual void addMenu(ActionContainer *before, ActionContainer *menu) = 0;
    Command *addSeparator(Utils::Id group = {});
    virtual Command *addSeparator(const Context &context, Utils::Id group = {}, QAction **outSeparator = nullptr) = 0;

    // This clears this menu and submenus from all actions and submenus.
    // It does not destroy the submenus and commands, just removes them from their parents.
    virtual void clear() = 0;
};

} // namespace Core
