// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythoneditor.h"

#include "pyside.h"
#include "pythonbuildconfiguration.h"
#include "pythonconstants.h"
#include "pythonhighlighter.h"
#include "pythonindenter.h"
#include "pythonkitaspect.h"
#include "pythonlanguageclient.h"
#include "pythonsettings.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>

#include <texteditor/texteditor.h>

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

class PythonEditorWidget : public TextEditorWidget
{
public:
    PythonEditorWidget(QWidget *parent = nullptr);

protected:
    void finalizeInitialization() override;
    void updateInterpretersSelector();

private:
    QToolButton *m_interpreters = nullptr;
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
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &PythonEditorWidget::updateInterpretersSelector);
    connect(KitManager::instance(), &KitManager::kitsChanged,
            this, &PythonEditorWidget::updateInterpretersSelector);
    auto pythonDocument = qobject_cast<PythonDocument *>(textDocument());
    if (QTC_GUARD(pythonDocument)) {
        connect(pythonDocument, &PythonDocument::pythonUpdated,
                this, &PythonEditorWidget::updateInterpretersSelector);
    }
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

    auto setButtonText = [this](QString text) {
        constexpr int maxTextLength = 25;
        if (text.size() > maxTextLength)
            text = text.left(maxTextLength - 3) + "...";
        m_interpreters->setText(text);
    };

    const FilePath documentPath = textDocument()->filePath();
    Project *project = Utils::findOrDefault(ProjectManager::projects(),
                                            [documentPath](Project *project) {
                                                return project->mimeType()
                                                           == Constants::C_PY_PROJECT_MIME_TYPE
                                                       && project->isKnownFile(documentPath);
                                            });

    if (project) {
        auto interpretersGroup = new QActionGroup(menu);
        interpretersGroup->setExclusive(true);
        for (Target *target : project->targets()) {
            QTC_ASSERT(target, continue);
            for (auto buildConfiguration : target->buildConfigurations()) {
                QTC_ASSERT(buildConfiguration, continue);
                const QString name = buildConfiguration->displayName();
                QAction *action = interpretersGroup->addAction(buildConfiguration->displayName());
                action->setCheckable(true);
                if (target == project->activeTarget()
                    && target->activeBuildConfiguration() == buildConfiguration) {
                    action->setChecked(true);
                    setButtonText(name);
                    if (auto pbc = qobject_cast<PythonBuildConfiguration *>(buildConfiguration))
                        m_interpreters->setToolTip(pbc->python().toUserOutput());
                }
                connect(action,
                        &QAction::triggered,
                        project,
                        [project, target, buildConfiguration]() {
                            target->setActiveBuildConfiguration(buildConfiguration,
                                                                SetActive::NoCascade);
                            if (target != project->activeTarget())
                                project->setActiveTarget(target, SetActive::NoCascade);
                        });
            }
        }

        menu->addActions(interpretersGroup->actions());

        QMenu *addMenu = menu->addMenu("Add new Interpreter");
        for (auto kit : KitManager::kits()) {
            if (std::optional<Interpreter> python = PythonKitAspect::python(kit)) {
                if (auto buildConficurationFactory
                    = ProjectExplorer::BuildConfigurationFactory::find(kit,
                                                                       project->projectFilePath())) {
                    const QString name = kit->displayName();
                    QMenu *interpreterAddMenu = addMenu->addMenu(name);
                    const QList<BuildInfo> buildInfos
                        = buildConficurationFactory->allAvailableSetups(kit,
                                                                        project->projectFilePath());
                    for (const BuildInfo &buildInfo : buildInfos) {
                        QAction *action = interpreterAddMenu->addAction(buildInfo.displayName);
                        connect(action, &QAction::triggered, project, [project, buildInfo]() {
                            if (BuildConfiguration *buildConfig = project->setup(buildInfo)) {
                                buildConfig->target()
                                    ->setActiveBuildConfiguration(buildConfig, SetActive::NoCascade);
                                project->setActiveTarget(buildConfig->target(),
                                                         SetActive::NoCascade);
                            }
                        });
                    }
                }
            }
        }

        menu->addSeparator();
    } else {
        auto setUserDefinedPython = [this](const FilePath &interpreter){
            const auto pythonDocument = qobject_cast<PythonDocument *>(textDocument());
            QTC_ASSERT(pythonDocument, return);
            const FilePath documentPath = pythonDocument->filePath();
            QTC_ASSERT(!documentPath.isEmpty(), return);
            definePythonForDocument(documentPath, interpreter);
            updateInterpretersSelector();
            pythonDocument->updateCurrentPython();
        };
        const FilePath currentInterpreterPath = detectPython(documentPath);
        const QList<Interpreter> configuredInterpreters = PythonSettings::interpreters();
        auto interpretersGroup = new QActionGroup(menu);
        interpretersGroup->setExclusive(true);
        std::optional<Interpreter> currentInterpreter;
        for (const Interpreter &interpreter : configuredInterpreters) {
            QAction *action = interpretersGroup->addAction(interpreter.name);
            connect(action, &QAction::triggered, this, [interpreter, setUserDefinedPython]() {
                setUserDefinedPython(interpreter.command);
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
                    [self = QPointer<PythonEditorWidget>(this),
                     currentInterpreter,
                     setUserDefinedPython]() {
                        if (!currentInterpreter)
                            return;
                        auto callback = [self, setUserDefinedPython](
                                            const std::optional<FilePath> &venvInterpreter) {
                            if (self && venvInterpreter)
                                setUserDefinedPython(*venvInterpreter);
                        };
                        PythonSettings::createVirtualEnvironmentInteractive(self->textDocument()
                                                                                ->filePath()
                                                                                .parentDir(),
                                                                            *currentInterpreter,
                                                                            callback);
                    });
        }
    }
    auto settingsAction = menu->addAction(Tr::tr("Manage Python Interpreters"));
    connect(settingsAction, &QAction::triggered, this, []() {
        Core::ICore::showOptionsDialog(Constants::C_PYTHONOPTIONS_PAGE_ID);
    });
}

PythonDocument::PythonDocument()
    : TextDocument(Constants::C_PYTHONEDITOR_ID)
{
    connect(PythonSettings::instance(),
            &PythonSettings::pylsEnabledChanged,
            this,
            [this](const bool enabled) {
                if (!enabled)
                    return;
                const FilePath &python = detectPython(filePath());
                if (python.exists())
                    openDocumentWithPython(python, this);
            });
    connect(this,
            &PythonDocument::openFinishedSuccessfully,
            this,
            &PythonDocument::updateCurrentPython);
}

void PythonDocument::updateCurrentPython()
{
    updatePython(detectPython(filePath()));
}

void PythonDocument::updatePython(const FilePath &python)
{
    openDocumentWithPython(python, this);
    PySideInstaller::checkPySideInstallation(python, this);
    emit pythonUpdated(python);
}

class PythonEditorFactory final : public TextEditorFactory
{
public:
    PythonEditorFactory()
    {
        setId(Constants::C_PYTHONEDITOR_ID);
        setDisplayName(::Core::Tr::tr(Constants::C_EDITOR_DISPLAY_NAME));
        addMimeType(Constants::C_PY_MIMETYPE);

        setOptionalActionMask(OptionalActions::Format
                                | OptionalActions::UnCommentSelection
                                | OptionalActions::UnCollapseAll
                                | OptionalActions::FollowSymbolUnderCursor);

        setDocumentCreator([]() { return new PythonDocument; });
        setEditorWidgetCreator([]() { return new PythonEditorWidget; });
        setIndenterCreator(&createPythonIndenter);
        setSyntaxHighlighterCreator(&createPythonHighlighter);
        setCommentDefinition(CommentDefinition::HashStyle);
        setParenthesesMatchingEnabled(true);
        setCodeFoldingSupported(true);
    }
};

void setupPythonEditorFactory(QObject *guard)
{
    static PythonEditorFactory thePythonEditorFactory;

    registerReplAction(guard);
}

} // Python::Internal
