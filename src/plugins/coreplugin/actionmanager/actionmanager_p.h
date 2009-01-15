/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef ACTIONMANAGERPRIVATE_H
#define ACTIONMANAGERPRIVATE_H

#include <coreplugin/actionmanager/actionmanager.h>

#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QMultiHash>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

struct CommandLocation
{
    int m_container;
    int m_position;
};

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
    ActionManagerPrivate(MainWindow *mainWnd, UniqueIDManager *uidmgr);
    ~ActionManagerPrivate();

    void setContext(const QList<int> &context);
    static ActionManagerPrivate* instance();

    void saveSettings(QSettings *settings);
    QList<int> defaultGroups() const;

    QList<CommandPrivate *> commands() const;
    QList<ActionContainerPrivate *> containers() const;

    bool hasContext(int context) const;

    Command *command(int uid) const;
    ActionContainer *actionContainer(int uid) const;

    void initialize();

    //ActionManager Interface
    ActionContainer *createMenu(const QString &id);
    ActionContainer *createMenuBar(const QString &id);

    Command *registerAction(QAction *action, const QString &id,
        const QList<int> &context);
    Command *registerShortcut(QShortcut *shortcut, const QString &id,
        const QList<int> &context);

    Core::Command *command(const QString &id) const;
    Core::ActionContainer *actionContainer(const QString &id) const;

private:
    bool hasContext(QList<int> context) const;
    Command *registerOverridableAction(QAction *action, const QString &id,
        bool checkUnique);

    static ActionManagerPrivate* m_instance;
    QList<int> m_defaultGroups;

    typedef QHash<int, CommandPrivate *> IdCmdMap;
    IdCmdMap m_idCmdMap;

    typedef QHash<int, ActionContainerPrivate *> IdContainerMap;
    IdContainerMap m_idContainerMap;

//    typedef QMap<int, int> GlobalGroupMap;
//    GlobalGroupMap m_globalgroups;
//
    QList<int> m_context;

    MainWindow *m_mainWnd;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONMANAGERPRIVATE_H
