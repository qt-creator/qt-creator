/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ACTIONMANAGERPRIVATE_H
#define ACTIONMANAGERPRIVATE_H

#include <coreplugin/actionmanager/actionmanager.h>
#include "command_p.h"

#include <coreplugin/icontext.h>

#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Core {

class UniqueIDManager;

namespace Internal {

class ActionContainerPrivate;
class MainWindow;
class CommandPrivate;

class ActionManagerPrivate : public Core::ActionManager
{
    Q_OBJECT

public:
    explicit ActionManagerPrivate(MainWindow *mainWnd);
    ~ActionManagerPrivate();

    void setContext(const Context &context);
    static ActionManagerPrivate *instance();

    void saveSettings(QSettings *settings);

    QList<Command *> commands() const;

    bool hasContext(int context) const;

    Command *command(int uid) const;
    ActionContainer *actionContainer(int uid) const;

    void initialize();

    //ActionManager Interface
    ActionContainer *createMenu(const Id &id);
    ActionContainer *createMenuBar(const Id &id);

    Command *registerAction(QAction *action, const Id &id,
        const Context &context, bool scriptable=false);
    Command *registerShortcut(QShortcut *shortcut, const Id &id,
        const Context &context, bool scriptable=false);

    Core::Command *command(const Id &id) const;
    Core::ActionContainer *actionContainer(const Id &id) const;
    void unregisterAction(QAction *action, const Id &id);
    void unregisterShortcut(const Id &id);

private slots:
    void containerDestroyed();
private:
    bool hasContext(const Context &context) const;
    Action *overridableAction(const Id &id);

    static ActionManagerPrivate *m_instance;

    typedef QHash<int, CommandPrivate *> IdCmdMap;
    IdCmdMap m_idCmdMap;

    typedef QHash<int, ActionContainerPrivate *> IdContainerMap;
    IdContainerMap m_idContainerMap;

//    typedef QMap<int, int> GlobalGroupMap;
//    GlobalGroupMap m_globalgroups;
//
    Context m_context;

    MainWindow *m_mainWnd;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONMANAGERPRIVATE_H
