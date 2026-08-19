// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "actionmanager_test.h"

#include "actionmanager.h"
#include "command.h"

#include <utils/algorithm.h>

#include <QAction>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

// ActionManager::command() warns when it finds nothing, which is the expected
// outcome here.
static Command *registeredCommand(Id id)
{
    return findOrDefault(ActionManager::commands(), [id](Command *cmd) { return cmd->id() == id; });
}

class ActionManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCommandGoesWithItsAction()
    {
        const Id id("Core.Tests.LoneAction");
        QVERIFY(!registeredCommand(id));
        {
            QAction action("Lone");
            QVERIFY(ActionManager::registerAction(&action, id));
            QVERIFY(registeredCommand(id));
        }
        // Nobody called unregisterAction, and the action is gone: so is the
        // command, instead of lingering in menus and shortcut settings with
        // nothing left to trigger.
        QVERIFY(!registeredCommand(id));
    }

    void testCommandOutlivesOneOfItsActions()
    {
        const Id id("Core.Tests.SharedAction");
        QAction global("Global");
        Command *cmd = ActionManager::registerAction(&global, id);
        QVERIFY(cmd);
        {
            QAction contextual("Contextual");
            ActionManager::registerAction(&contextual, id, Context("Core.Tests.Context"));
        }
        QCOMPARE(registeredCommand(id), cmd);
        QCOMPARE(cmd->action()->text(), QString("Global"));
    }
};

QObject *createActionManagerTest()
{
    return new ActionManagerTest;
}

} // namespace Core::Internal

#include "actionmanager_test.moc"
