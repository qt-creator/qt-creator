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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <coreplugin/actionmanager/actionmanagerinterface.h>

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

class ActionContainer;
class MainWindow;
class Command;

class ActionManager : public Core::ActionManagerInterface
{
    Q_OBJECT

public:
    ActionManager(MainWindow *mainWnd, UniqueIDManager *uidmgr);
    ~ActionManager();

    void setContext(const QList<int> &context);
    static ActionManager* instance();

    void saveSettings(QSettings *settings);
    QList<int> defaultGroups() const;

    QList<Command *> commands() const;
    QList<ActionContainer *> containers() const;

    bool hasContext(int context) const;

    ICommand *command(int uid) const;
    IActionContainer *actionContainer(int uid) const;

    void registerGlobalGroup(int groupId, int containerId);

    void initialize();

    //ActionManager Interface
    IActionContainer *createMenu(const QString &id);
    IActionContainer *createMenuBar(const QString &id);

    ICommand *registerAction(QAction *action, const QString &id,
        const QList<int> &context);
    ICommand *registerAction(QAction *action, const QString &id);
    ICommand *registerShortcut(QShortcut *shortcut, const QString &id,
        const QList<int> &context);

    void addAction(Core::ICommand *action, const QString &globalGroup);
    void addMenu(Core::IActionContainer *menu, const QString &globalGroup);

    Core::ICommand *command(const QString &id) const;
    Core::IActionContainer *actionContainer(const QString &id) const;

private:
    bool hasContext(QList<int> context) const;
    ICommand *registerOverridableAction(QAction *action, const QString &id,
        bool checkUnique);

    static ActionManager* m_instance;
    QList<int> m_defaultGroups;

    typedef QHash<int, Command *> IdCmdMap;
    IdCmdMap m_idCmdMap;

    typedef QHash<int, ActionContainer *> IdContainerMap;
    IdContainerMap m_idContainerMap;

    typedef QMap<int, int> GlobalGroupMap;
    GlobalGroupMap m_globalgroups;

    QList<int> m_context;

    MainWindow *m_mainWnd;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONMANAGER_H
