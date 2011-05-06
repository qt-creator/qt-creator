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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include "coreplugin/core_global.h"
#include "coreplugin/uniqueidmanager.h"
#include <coreplugin/actionmanager/command.h>
#include "coreplugin/icontext.h"

#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
class QShortcut;
class QString;
QT_END_NAMESPACE

namespace Core {

class ActionContainer;

class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    ActionManager(QObject *parent = 0) : QObject(parent) {}
    virtual ~ActionManager() {}

    virtual ActionContainer *createMenu(const Id &id) = 0;
    virtual ActionContainer *createMenuBar(const Id &id) = 0;

    virtual Command *registerAction(QAction *action, const Id &id, const Context &context, bool scriptable = false) = 0;
    virtual Command *registerShortcut(QShortcut *shortcut, const Id &id, const Context &context, bool scriptable = false) = 0;

    virtual Command *command(const Id &id) const = 0;
    virtual ActionContainer *actionContainer(const Id &id) const = 0;

    virtual QList<Command *> commands() const = 0;

    virtual void unregisterAction(QAction *action, const Id &id) = 0;
    virtual void unregisterShortcut(const Id &id) = 0;

signals:
    void commandListChanged();
    void commandAdded(const QString &id);
};

} // namespace Core

#endif // ACTIONMANAGER_H
