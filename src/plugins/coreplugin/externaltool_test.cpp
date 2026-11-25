// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltool_test.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/actioncontainer.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "externaltool.h"
#include "externaltoolmanager.h"
#include "progressmanager/progressmanager.h"

#include <utils/aggregate.h>
#include <utils/aiassistance.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/plaintextedit/plaintextedit.h>
#include <utils/temporarydirectory.h>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QListWidget>
#include <QMenu>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QPointer>
#include <QTest>

#include <memory>

using namespace Utils;

namespace Core::Internal {

class ExternalToolTest final : public QObject
{
    Q_OBJECT

public:
    enum Field { Executable, Arguments, Input, WorkingDirectory, Environment };

private slots:
    void cleanup();
    void testEmptyVariables_data();
    void testEmptyVariables();
    void testUnresolvedVariables_data();
    void testUnresolvedVariables();
    void testFallbackExecutables_data();
    void testFallbackExecutables();
    void testActionStateFollowsTheCommand();
    void testReplaceSelectionNeedsAWritableEditor();
    void testSendToAiRoundTrip();
    void testSendToAiSubmitsOutput();
    void testSendToAiAfterFailure();
    void testSendToAiCanceledByProgress();
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
    QTest::newRow("environment") << int(ExternalToolTest::Environment);
}

static void setupTool(ExternalTool *tool, int field, const QString &variable)
{
    const QString executable = field == ExternalToolTest::Executable ? variable : QString("tool");
    tool->setExecutables({FilePath::fromString(executable)});
    tool->setArguments(field == ExternalToolTest::Arguments ? variable : QString());
    tool->setInput(field == ExternalToolTest::Input ? variable : QString());
    tool->setWorkingDirectory(
        FilePath::fromString(field == ExternalToolTest::WorkingDirectory ? variable : QString()));
    EnvironmentChanges environment;
    if (field == ExternalToolTest::Environment)
        environment.setItemsFromUser({{"QTC_EXTERNALTOOL_TEST_VAR", variable}});
    tool->setEnvironmentUserChanges(environment);
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
    QVERIFY(!contextless.canExecute());

    ExternalTool available;
    setupTool(&available, field, "%{IDE:ResourcePath}");
    QVERIFY(available.emptyVariables().isEmpty());
    QVERIFY(available.unresolvedVariables().isEmpty());
    QVERIFY(available.canExecute());

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

void ExternalToolTest::testFallbackExecutables_data()
{
    QTest::addColumn<QStringList>("executables");
    QTest::addColumn<QStringList>("empty");
    QTest::addColumn<QStringList>("unresolved");

    const QString empty = "%{CurrentDocument:FilePath}";
    const QString unresolved = "%{NoSuchVariable}";
    const QString available = "%{IDE:ResourcePath}";

    QTest::newRow("empty, available") << QStringList{empty, available} << QStringList()
                                      << QStringList();
    QTest::newRow("unresolved, available") << QStringList{unresolved, available}
                                           << QStringList() << QStringList();
    QTest::newRow("empty, plain") << QStringList{empty, "tool"} << QStringList()
                                  << QStringList();
    QTest::newRow("empty, empty")
        << QStringList{empty, "%{CurrentDocument:Path}"}
        << QStringList{"CurrentDocument:FilePath", "CurrentDocument:Path"} << QStringList();
    QTest::newRow("unresolved, unresolved")
        << QStringList{unresolved, "%{NoOtherVariable}"} << QStringList()
        << QStringList{"NoSuchVariable", "NoOtherVariable"};
    QTest::newRow("empty, unresolved") << QStringList{empty, unresolved}
                                       << QStringList("CurrentDocument:FilePath")
                                       << QStringList("NoSuchVariable");
}

// Only the first executable that is found is run, so the others not expanding
// does not keep the tool from running.
void ExternalToolTest::testFallbackExecutables()
{
    QFETCH(QStringList, executables);
    QFETCH(QStringList, empty);
    QFETCH(QStringList, unresolved);

    MacroExpander *expander = Utils::globalMacroExpander();
    if (!expander->expand(QString("%{CurrentDocument:FilePath}")).isEmpty())
        QSKIP("A document is open, so the current-document variables are not empty.");
    QVERIFY(!expander->expand(QString("%{IDE:ResourcePath}")).isEmpty());

    ExternalTool tool;
    tool.setExecutables(Utils::transform(executables, &FilePath::fromString));
    QCOMPARE(tool.emptyVariables(), empty);
    QCOMPARE(tool.unresolvedVariables(), unresolved);
    QCOMPARE(bool(tool.canExecute()), empty.isEmpty() && unresolved.isEmpty());
}

static QAction *menuAction(const ExternalTool *tool)
{
    ActionContainer *menu = ActionManager::actionContainer(Id(Constants::M_TOOLS_EXTERNAL));
    return Utils::findOrDefault(menu->menu()->actions(),
                                Utils::equal(&QAction::text, tool->displayName()));
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
    QAction *action = menuAction(tool);
    QVERIFY(action);
    QVERIFY(action != command->action());
    QVERIFY(!action->isEnabled());

    // A shortcut reaches the command without opening the menu, so the command must
    // stay enabled for when the tool can run, and refuse to run until then.
    QVERIFY(command->action()->isEnabled());
    QVERIFY(!tool->canExecute());

    // Nothing observes the command changing, so only showing the menu can pick it up.
    // The locator gets there through the same signal.
    tool->setArguments({});
    QVERIFY(!action->isEnabled());

    ActionContainer *menu = ActionManager::actionContainer(Id(Constants::M_TOOLS_EXTERNAL));
    QVERIFY(QMetaObject::invokeMethod(menu->menu(), "aboutToShow"));
    QVERIFY(action->isEnabled());
    QVERIFY(command->action()->isEnabled());
}

void ExternalToolTest::testReplaceSelectionNeedsAWritableEditor()
{
    const QMap<QString, QList<ExternalTool *>> original = ExternalToolManager::toolsByCategory();
    const QScopeGuard restore([original] { ExternalToolManager::setToolsByCategory(original); });

    // The manager takes ownership, and hands it back to the guard above.
    auto tool = new ExternalTool;
    tool->setId("Test.ReplaceSelectionTool");
    tool->setDisplayName("Test Replace Selection Tool");
    tool->setExecutables({FilePath::fromString("tool")});
    tool->setOutputHandling(ExternalTool::ReplaceSelection);

    QMap<QString, QList<ExternalTool *>> tools = original;
    tools[QString()].append(tool);
    ExternalToolManager::setToolsByCategory(tools);

    QAction *action = menuAction(tool);
    QVERIFY(action);

    // A run that left a document open cannot check the no-editor case.
    if (!EditorManager::currentEditor())
        QVERIFY(!action->isEnabled());

    QString title = "readonly.txt";
    IEditor *editor = EditorManager::openEditorWithContents(
        Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "text");
    QVERIFY(editor);
    const QScopeGuard closeEditor([editor] { EditorManager::closeEditors({editor}, false); });
    PlainTextEdit *edit = Aggregation::query<PlainTextEdit>(editor->widget());
    QVERIFY(edit);

    ActionContainer *menu = ActionManager::actionContainer(Id(Constants::M_TOOLS_EXTERNAL));

    edit->setReadOnly(true);
    QVERIFY(QMetaObject::invokeMethod(menu->menu(), "aboutToShow"));
    QVERIFY(!action->isEnabled());

    edit->setReadOnly(false);
    QVERIFY(QMetaObject::invokeMethod(menu->menu(), "aboutToShow"));
    QVERIFY(action->isEnabled());
}

// Sending the output to an AI and the prompt for it survive saving and loading.
void ExternalToolTest::testSendToAiRoundTrip()
{
    const QByteArray xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<externaltool id="Test.SendToAi">
    <description>Sends to AI</description>
    <displayname>Send To AI</displayname>
    <category>Test</category>
    <executable output="sendtoai" error="showinpane">
        <path>tool</path>
        <aiprompt>Review %{CurrentDocument:FilePath}</aiprompt>
    </executable>
</externaltool>
)";
    const Result<ExternalTool *> parsed = ExternalTool::createFromXml(xml);
    QVERIFY2(parsed, parsed ? "" : qPrintable(parsed.error()));
    const std::unique_ptr<ExternalTool> tool(*parsed);
    QCOMPARE(tool->outputHandling(), ExternalTool::SendToAi);
    QCOMPARE(tool->errorHandling(), ExternalTool::ShowInPane);
    QCOMPARE(tool->aiPrompt(), "Review %{CurrentDocument:FilePath}");

    const TemporaryDirectory dir("externaltool-ai");
    const FilePath file = dir.filePath("tool.xml");
    tool->setFilePath(file);
    const Result<> saved = tool->save();
    QVERIFY2(saved, saved ? "" : qPrintable(saved.error()));
    const Result<ExternalTool *> loaded = ExternalTool::createFromFile(file);
    QVERIFY2(loaded, loaded ? "" : qPrintable(loaded.error()));
    const std::unique_ptr<ExternalTool> reloaded(*loaded);
    reloaded->setFilePath(file);
    QVERIFY(*reloaded == *tool);
}

class RecordingAiProvider final : public AiProvider
{
public:
    RecordingAiProvider()
        : AiProvider("Test.RecordingAiProvider", "Recording")
    {
        AiTarget target;
        target.id = "record";
        target.displayName = "Record Request";
        setTargets({target});
    }

    void execute(const AiRequest &request, const AiTargetSelection &, AiResponse *response) final
    {
        requests.append(request);
        response->reportFinished(QString());
    }

    QList<AiRequest> requests;
};

static QDialog *visibleDialog()
{
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto dialog = qobject_cast<QDialog *>(widget); dialog && dialog->isVisible())
            return dialog;
    }
    return nullptr;
}

const char sendToAiProgressId[] = "Core.ExternalTool.SendToAi";

static void pickRecordingTarget()
{
    QDialog *dialog = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((dialog = visibleDialog()), 10000);
    auto list = dialog->findChild<QListWidget *>();
    QVERIFY(list);
    const QList<QListWidgetItem *> items = list->findItems("Record Request", Qt::MatchExactly);
    QCOMPARE(items.size(), 1);
    list->setCurrentItem(items.first());
    dialog->accept();
}

static std::unique_ptr<ExternalTool> sendToAiTool(const QString &arguments)
{
    auto tool = std::make_unique<ExternalTool>();
    tool->setId("Test.SendToAiTool");
    tool->setDisplayName("Test Send To AI Tool");
    tool->setExecutables({FilePath::fromString("/bin/sh")});
    tool->setArguments(arguments);
    tool->setOutputHandling(ExternalTool::SendToAi);
    tool->setErrorHandling(ExternalTool::SendToAi);
    return tool;
}

static QString contextText(const AiRequest &request, const QString &name)
{
    for (const AiContextItem &item : request.context) {
        if (item.name == name)
            return item.text;
    }
    return {};
}

// The channels that go to the AI are collected and sent along with the expanded
// prompt once the tool finished; the others are not.
void ExternalToolTest::testSendToAiSubmitsOutput()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The tool is a POSIX shell command.");

    RecordingAiProvider provider;
    auto tool = std::make_unique<ExternalTool>();
    tool->setId("Test.SendToAiTool");
    tool->setDisplayName("Test Send To AI Tool");
    tool->setExecutables({FilePath::fromString("/bin/sh")});
    tool->setArguments("-c \"echo to-stdout; echo to-stderr >&2\"");
    tool->setOutputHandling(ExternalTool::SendToAi);
    tool->setErrorHandling(ExternalTool::Ignore);
    tool->setAiPrompt("Check this in %{IDE:ResourcePath}");
    QSignalSpy started(ProgressManager::instance(), &ProgressManager::taskStarted);
    QSignalSpy ended(ProgressManager::instance(), &ProgressManager::allTasksFinished);
    tool->execute();
    // The run shows its progress until the AI has the request.
    QVERIFY(started.contains({QVariant::fromValue(Id(sendToAiProgressId))}));

    pickRecordingTarget();
    if (QTest::currentTestFailed())
        return;

    QTRY_COMPARE(provider.requests.size(), 1);
    QTRY_VERIFY(ended.contains({QVariant::fromValue(Id(sendToAiProgressId))}));
    const AiRequest &request = provider.requests.first();
    QCOMPARE(request.id, Id("Core.ExternalTool.Test.SendToAiTool"));
    QVERIFY2(!request.prompt.contains("%{"), qPrintable(request.prompt));
    QVERIFY2(request.prompt.startsWith("Check this in "), qPrintable(request.prompt));
    QVERIFY2(contextText(request, "Standard output").contains("to-stdout"),
             qPrintable(contextText(request, "Standard output")));
    QVERIFY(!contextText(request, "Standard output").contains("to-stderr"));
    QVERIFY(contextText(request, "Command").contains("/bin/sh"));
    QVERIFY(Utils::allOf(request.context, [](const AiContextItem &item) {
        return item.name != "Standard error";
    }));
}

// A test that fails early must not leave its run, or its dialog, to the next one.
void ExternalToolTest::cleanup()
{
    ProgressManager::cancelTasks(sendToAiProgressId);
    while (QDialog *dialog = visibleDialog())
        dialog->reject();
}

// The output of a tool that fails is what one wants explained the most.
void ExternalToolTest::testSendToAiAfterFailure()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The tool is a POSIX shell command.");

    RecordingAiProvider provider;
    const auto tool = sendToAiTool("-c \"echo something broke >&2; exit 3\"");
    tool->execute();
    pickRecordingTarget();
    if (QTest::currentTestFailed())
        return;

    QTRY_COMPARE(provider.requests.size(), 1);
    const AiRequest &request = provider.requests.first();
    QVERIFY2(contextText(request, "Standard error").contains("something broke"),
             qPrintable(contextText(request, "Standard error")));
    QVERIFY2(contextText(request, "Command").contains("3"),
             qPrintable(contextText(request, "Command")));
}

// Canceling the progress while the user picks an AI ends the run without a
// request.
void ExternalToolTest::testSendToAiCanceledByProgress()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("The tool is a POSIX shell command.");

    RecordingAiProvider provider;
    const auto tool = sendToAiTool("-c \"echo output\"");
    QSignalSpy ended(ProgressManager::instance(), &ProgressManager::allTasksFinished);
    tool->execute();
    QDialog *dialog = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT((dialog = visibleDialog()), 10000);
    QPointer<QDialog> dialogGuard(dialog);

    ProgressManager::cancelTasks(sendToAiProgressId);
    QTRY_VERIFY(ended.contains({QVariant::fromValue(Id(sendToAiProgressId))}));
    QTRY_VERIFY(dialogGuard.isNull() || !dialogGuard->isVisible());
    QVERIFY(provider.requests.isEmpty());
}

QObject *createExternalToolTest()
{
    return new ExternalToolTest;
}

} // namespace Core::Internal

#include "externaltool_test.moc"
