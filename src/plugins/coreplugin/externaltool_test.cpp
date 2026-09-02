// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltool_test.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/actioncontainer.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "externaltool.h"
#include "externaltoolmanager.h"
#include "icontext.h"
#include "icore.h"

#include <utils/filepath.h>
#include <utils/macroexpander.h>

#include <QAction>
#include <QMenu>
#include <QScopeGuard>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

class ExternalToolTest final : public QObject
{
    Q_OBJECT

public:
    enum Field { Executable, Arguments, Input, WorkingDirectory };

private slots:
    void testEmptyVariables_data();
    void testEmptyVariables();
    void testUnresolvedVariables_data();
    void testUnresolvedVariables();
    void testActionStateFollowsTheCommand();
};

// Every field of the command is scanned, so dropping one of them from the scan
// would let a tool that cannot run appear ready.
static void addCommandFieldRows()
{
    QTest::addColumn<int>("field");
    QTest::newRow("executable") << int(ExternalToolTest::Executable);
    QTest::newRow("arguments") << int(ExternalToolTest::Arguments);
    QTest::newRow("input") << int(ExternalToolTest::Input);
    QTest::newRow("workingDirectory") << int(ExternalToolTest::WorkingDirectory);
}

static void setupTool(ExternalTool *tool, int field, const QString &variable)
{
    const QString executable = field == ExternalToolTest::Executable ? variable : QString("tool");
    tool->setExecutables({FilePath::fromString(executable)});
    tool->setArguments(field == ExternalToolTest::Arguments ? variable : QString());
    tool->setInput(field == ExternalToolTest::Input ? variable : QString());
    tool->setWorkingDirectory(
        FilePath::fromString(field == ExternalToolTest::WorkingDirectory ? variable : QString()));
}

void ExternalToolTest::testEmptyVariables_data()
{
    addCommandFieldRows();
}

void ExternalToolTest::testEmptyVariables()
{
    QFETCH(int, field);

    MacroExpander *expander = Utils::globalMacroExpander();
    if (!expander->expand(QString("%{CurrentDocument:FilePath}")).isEmpty())
        QSKIP("A document is open, so the current-document variables are not empty.");
    QVERIFY(!expander->expand(QString("%{IDE:ResourcePath}")).isEmpty());

    ExternalTool contextless;
    setupTool(&contextless, field, "%{CurrentDocument:FilePath}");
    QCOMPARE(contextless.emptyVariables(), QStringList("CurrentDocument:FilePath"));
    QVERIFY(contextless.unresolvedVariables().isEmpty());

    ExternalTool available;
    setupTool(&available, field, "%{IDE:ResourcePath}");
    QVERIFY(available.emptyVariables().isEmpty());
    QVERIFY(available.unresolvedVariables().isEmpty());

    // What follows a prefix is user input, so an empty result is legitimate.
    ExternalTool prefixed;
    setupTool(&prefixed, field, "%{Env:QTC_EXTERNALTOOL_TEST_UNSET}");
    QVERIFY(prefixed.emptyVariables().isEmpty());
    QVERIFY(prefixed.unresolvedVariables().isEmpty());
}

void ExternalToolTest::testUnresolvedVariables_data()
{
    addCommandFieldRows();
}

// An unknown variable stays in the command as-is, which is what makes the tool
// refuse to run. It does not count as empty.
void ExternalToolTest::testUnresolvedVariables()
{
    QFETCH(int, field);

    ExternalTool tool;
    setupTool(&tool, field, "%{NoSuchVariable}");
    QCOMPARE(tool.unresolvedVariables(), QStringList("NoSuchVariable"));
    QVERIFY(tool.emptyVariables().isEmpty());
}

void ExternalToolTest::testActionStateFollowsTheCommand()
{
    const QMap<QString, QList<ExternalTool *>> original = ExternalToolManager::toolsByCategory();
    const QScopeGuard restore([original] { ExternalToolManager::setToolsByCategory(original); });

    // The manager takes ownership, and hands it back to the guard above.
    auto tool = new ExternalTool;
    tool->setId("Test.ExternalTool");
    tool->setDisplayName("Test External Tool");
    tool->setExecutables({FilePath::fromString("tool")});
    tool->setArguments("%{NoSuchVariable}");

    QMap<QString, QList<ExternalTool *>> tools = original;
    tools[QString()].append(tool);
    ExternalToolManager::setToolsByCategory(tools);

    Command *command = ActionManager::command(Id("Tools.External.").withSuffix(tool->id()));
    QVERIFY(command);
    QAction *action = command->actionForContext(Constants::C_GLOBAL);
    QVERIFY(action);
    QVERIFY(!action->isEnabled());

    // Nothing observes the command changing, so only showing the menu can pick it up.
    tool->setArguments({});
    QVERIFY(!action->isEnabled());

    ActionContainer *menu = ActionManager::actionContainer(Id(Constants::M_TOOLS_EXTERNAL));
    QVERIFY(QMetaObject::invokeMethod(menu->menu(), "aboutToShow"));
    QVERIFY(action->isEnabled());

    // Shortcuts and the locator never open the menu, so a context change has to
    // reach the action too.
    tool->setArguments("%{NoSuchVariable}");
    QVERIFY(action->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(ICore::instance(), "contextChanged",
                                      Q_ARG(Core::Context, Core::Context())));
    QVERIFY(!action->isEnabled());
}

QObject *createExternalToolTest()
{
    return new ExternalToolTest;
}

} // namespace Core::Internal

#include "externaltool_test.moc"
