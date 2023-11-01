// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythoneditor.h"

#include "pyside.h"
#include "pythonconstants.h"
#include "pythonhighlighter.h"
#include "pythonindenter.h"
#include "pythonlanguageclient.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/commandbutton.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>

#include <utils/stylehelper.h>

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QMenu>

using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace Python::Internal {

static QAction *createAction(QObject *parent, ReplType type)
{
    QAction *action = new QAction(parent);
    switch (type) {
    case ReplType::Unmodified:
        action->setText(Tr::tr("REPL"));
        action->setToolTip(Tr::tr("Open interactive Python."));
        break;
    case ReplType::Import:
        action->setText(Tr::tr("REPL Import File"));
        action->setToolTip(Tr::tr("Open interactive Python and import file."));
        break;
    case ReplType::ImportToplevel:
        action->setText(Tr::tr("REPL Import *"));
        action->setToolTip(Tr::tr("Open interactive Python and import * from file."));
        break;
    }

    QObject::connect(action, &QAction::triggered, parent, [type, parent] {
        Core::IDocument *doc = Core::EditorManager::currentDocument();
        openPythonRepl(parent, doc ? doc->filePath() : FilePath(), type);
    });
    return action;
}

static void registerReplAction(QObject *parent)
{
    Core::ActionManager::registerAction(createAction(parent, ReplType::Unmodified),
                                        Constants::PYTHON_OPEN_REPL);
    Core::ActionManager::registerAction(createAction(parent, ReplType::Import),
                                        Constants::PYTHON_OPEN_REPL_IMPORT);
    Core::ActionManager::registerAction(createAction(parent, ReplType::ImportToplevel),
                                        Constants::PYTHON_OPEN_REPL_IMPORT_TOPLEVEL);
}

class PythonDocument : public TextDocument
{
    Q_OBJECT
public:
    PythonDocument() : TextDocument(Constants::C_PYTHONEDITOR_ID)
    {
        connect(PythonSettings::instance(),
                &PythonSettings::pylsEnabledChanged,
                this,
                [this](const bool enabled) {
                    if (!enabled)
                        return;
                    const FilePath &python = detectPython(filePath());
                    if (python.exists())
                        PyLSConfigureAssistant::openDocumentWithPython(python, this);
                });
        connect(this, &PythonDocument::openFinishedSuccessfully,
                this, &PythonDocument::checkForPyls);
    }

    void checkForPyls()
    {
        const FilePath &python = detectPython(filePath());
        if (!python.exists())
            return;

        PyLSConfigureAssistant::openDocumentWithPython(python, this);
        PySideInstaller::checkPySideInstallation(python, this);
    }
};

class PythonEditorWidget : public TextEditorWidget
{
public:
    PythonEditorWidget(QWidget *parent = nullptr);

protected:
    void finalizeInitialization() override;
    void setUserDefinedPython(const Interpreter &interpreter);
    void updateInterpretersSelector();

private:
    QToolButton *m_interpreters = nullptr;
    QList<QMetaObject::Connection> m_projectConnections;
};

PythonEditorWidget::PythonEditorWidget(QWidget *parent) : TextEditorWidget(parent)
{
    auto replButton = new QToolButton(this);
    replButton->setProperty(StyleHelper::C_NO_ARROW, true);
    replButton->setText(Tr::tr("REPL"));
    replButton->setPopupMode(QToolButton::InstantPopup);
    replButton->setToolTip(Tr::tr("Open interactive Python. Either importing nothing, "
                                  "importing the current file, "
                                  "or importing everything (*) from the current file."));
    auto menu = new QMenu(replButton);
    replButton->setMenu(menu);
    menu->addAction(Core::ActionManager::command(Constants::PYTHON_OPEN_REPL)->action());
    menu->addSeparator();
    menu->addAction(Core::ActionManager::command(Constants::PYTHON_OPEN_REPL_IMPORT)->action());
    menu->addAction(
        Core::ActionManager::command(Constants::PYTHON_OPEN_REPL_IMPORT_TOPLEVEL)->action());
    insertExtraToolBarWidget(TextEditorWidget::Left, replButton);
}

void PythonEditorWidget::finalizeInitialization()
{
    connect(textDocument(), &TextDocument::filePathChanged,
            this, &PythonEditorWidget::updateInterpretersSelector);
    connect(PythonSettings::instance(), &PythonSettings::interpretersChanged,
            this, &PythonEditorWidget::updateInterpretersSelector);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &PythonEditorWidget::updateInterpretersSelector);
}

void PythonEditorWidget::setUserDefinedPython(const Interpreter &interpreter)
{
    const auto pythonDocument = qobject_cast<PythonDocument *>(textDocument());
    QTC_ASSERT(pythonDocument, return);
    FilePath documentPath = pythonDocument->filePath();
    QTC_ASSERT(!documentPath.isEmpty(), return);
    if (Project *project = ProjectManager::projectForFile(documentPath)) {
        if (Target *target = project->activeTarget()) {
            if (RunConfiguration *rc = target->activeRunConfiguration()) {
                if (auto interpretersAspect= rc->aspect<InterpreterAspect>()) {
                    interpretersAspect->setCurrentInterpreter(interpreter);
                    return;
                }
            }
        }
    }
    definePythonForDocument(textDocument()->filePath(), interpreter.command);
    updateInterpretersSelector();
    pythonDocument->checkForPyls();
}

void PythonEditorWidget::updateInterpretersSelector()
{
    if (!m_interpreters) {
        m_interpreters = new QToolButton(this);
        insertExtraToolBarWidget(TextEditorWidget::Left, m_interpreters);
        m_interpreters->setMenu(new QMenu(m_interpreters));
        m_interpreters->setPopupMode(QToolButton::InstantPopup);
        m_interpreters->setToolButtonStyle(Qt::ToolButtonTextOnly);
        m_interpreters->setProperty(StyleHelper::C_NO_ARROW, true);
    }

    QMenu *menu = m_interpreters->menu();
    QTC_ASSERT(menu, return);
    menu->clear();
    for (const QMetaObject::Connection &connection : m_projectConnections)
        disconnect(connection);
    m_projectConnections.clear();
    const FilePath documentPath = textDocument()->filePath();
    if (Project *project = ProjectManager::projectForFile(documentPath)) {
        m_projectConnections << connect(project,
                                        &Project::activeTargetChanged,
                                        this,
                                        &PythonEditorWidget::updateInterpretersSelector);
        if (Target *target = project->activeTarget()) {
            m_projectConnections << connect(target,
                                            &Target::activeRunConfigurationChanged,
                                            this,
                                            &PythonEditorWidget::updateInterpretersSelector);
            if (RunConfiguration *rc = target->activeRunConfiguration()) {
                if (auto interpreterAspect = rc->aspect<InterpreterAspect>()) {
                    m_projectConnections << connect(interpreterAspect,
                                                    &InterpreterAspect::changed,
                                                    this,
                                                    &PythonEditorWidget::updateInterpretersSelector);
                }
            }
        }
    }

    auto setButtonText = [this](QString text) {
        constexpr int maxTextLength = 25;
        if (text.size() > maxTextLength)
            text = text.left(maxTextLength - 3) + "...";
        m_interpreters->setText(text);
    };

    const FilePath currentInterpreterPath = detectPython(textDocument()->filePath());
    const QList<Interpreter> configuredInterpreters = PythonSettings::interpreters();
    auto interpretersGroup = new QActionGroup(menu);
    interpretersGroup->setExclusive(true);
    std::optional<Interpreter> currentInterpreter;
    for (const Interpreter &interpreter : configuredInterpreters) {
        QAction *action = interpretersGroup->addAction(interpreter.name);
        connect(action, &QAction::triggered, this, [this, interpreter]() {
            setUserDefinedPython(interpreter);
        });
        action->setCheckable(true);
        if (!currentInterpreter && interpreter.command == currentInterpreterPath) {
            currentInterpreter = interpreter;
            action->setChecked(true);
            setButtonText(interpreter.name);
            m_interpreters->setToolTip(interpreter.command.toUserOutput());
        }
    }
    menu->addActions(interpretersGroup->actions());
    if (!currentInterpreter) {
        if (currentInterpreterPath.exists())
            setButtonText(currentInterpreterPath.toUserOutput());
        else
            setButtonText(Tr::tr("No Python Selected"));
    }
    if (!interpretersGroup->actions().isEmpty()) {
        menu->addSeparator();
        auto venvAction = menu->addAction(Tr::tr("Create Virtual Environment"));
        connect(venvAction,
                &QAction::triggered,
                this,
                [self = QPointer<PythonEditorWidget>(this), currentInterpreter]() {
                    if (!currentInterpreter)
                        return;
                    auto callback = [self](const std::optional<Interpreter> &venvInterpreter) {
                        if (self && venvInterpreter)
                            self->setUserDefinedPython(*venvInterpreter);
                    };
                    PythonSettings::createVirtualEnvironmentInteractive(self->textDocument()
                                                                            ->filePath()
                                                                            .parentDir(),
                                                                        *currentInterpreter,
                                                                        callback);
                });
    }
    auto settingsAction = menu->addAction(Tr::tr("Manage Python Interpreters"));
    connect(settingsAction, &QAction::triggered, this, []() {
        Core::ICore::showOptionsDialog(Constants::C_PYTHONOPTIONS_PAGE_ID);
    });
}

PythonEditorFactory::PythonEditorFactory()
{
    registerReplAction(&m_guard);

    setId(Constants::C_PYTHONEDITOR_ID);
    setDisplayName(::Core::Tr::tr(Constants::C_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::C_PY_MIMETYPE);

    setEditorActionHandlers(TextEditorActionHandler::Format
                            | TextEditorActionHandler::UnCommentSelection
                            | TextEditorActionHandler::UnCollapseAll
                            | TextEditorActionHandler::FollowSymbolUnderCursor);

    setDocumentCreator([]() { return new PythonDocument; });
    setEditorWidgetCreator([]() { return new PythonEditorWidget; });
    setIndenterCreator([](QTextDocument *doc) { return new PythonIndenter(doc); });
    setSyntaxHighlighterCreator([] { return new PythonHighlighter; });
    setCommentDefinition(CommentDefinition::HashStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
}

} // Python::Internal

#include "pythoneditor.moc"
