// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsplugin.h"

#include "clangtool.h"
#include "clangtoolsconstants.h"
#include "clangtoolsprojectsettingswidget.h"
#include "clangtoolstr.h"
#include "documentclangtoolrunner.h"
#include "documentquickfixfactory.h"
#include "settingswidget.h"

#ifdef WITH_TESTS
#include "readexporteddiagnosticstest.h"
#include "clangtoolspreconfiguredsessiontests.h"
#include "clangtoolsunittests.h"
#endif

#include <utils/icon.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>

#include <texteditor/texteditor.h>

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <QAction>
#include <QDebug>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QToolBar>

using namespace Core;
using namespace ProjectExplorer;

namespace ClangTools::Internal {

static ProjectPanelFactory *m_projectPanelFactoryInstance = nullptr;

ProjectPanelFactory *projectPanelFactory()
{
    return m_projectPanelFactoryInstance;
}

class ClangToolsPluginPrivate
{
public:
    ClangToolsPluginPrivate()
        : quickFixFactory(
            [this](const Utils::FilePath &filePath) { return runnerForFilePath(filePath); })
    {}

    DocumentClangToolRunner *runnerForFilePath(const Utils::FilePath &filePath)
    {
        for (DocumentClangToolRunner *runner : std::as_const(documentRunners)) {
            if (runner->filePath() == filePath)
                return runner;
        }
        return nullptr;
    }

    ClangTidyTool clangTidyTool;
    ClazyTool clazyTool;
    ClangToolsOptionsPage optionsPage;
    QMap<Core::IDocument *, DocumentClangToolRunner *> documentRunners;
    DocumentQuickFixFactory quickFixFactory;
};

ClangToolsPlugin::~ClangToolsPlugin()
{
    delete d;
}

void ClangToolsPlugin::initialize()
{
    TaskHub::addCategory({taskCategory(),
                          Tr::tr("Clang Tools"),
                          Tr::tr("Issues that Clang-Tidy and Clazy found when analyzing code.")});

    // Import tidy/clazy diagnostic configs from CppEditor now
    // instead of at opening time of the settings page
    ClangToolsSettings::instance();

    d = new ClangToolsPluginPrivate;

    registerAnalyzeActions();

    auto panelFactory = m_projectPanelFactoryInstance = new ProjectPanelFactory;
    panelFactory->setPriority(100);
    panelFactory->setId(Constants::PROJECT_PANEL_ID);
    panelFactory->setDisplayName(Tr::tr("Clang Tools"));
    panelFactory->setCreateWidgetFunction(
        [](Project *project) { return new ClangToolsProjectSettingsWidget(project); });
    ProjectPanelFactory::registerFactory(panelFactory);

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::currentEditorChanged,
            this,
            &ClangToolsPlugin::onCurrentEditorChanged);

#ifdef WITH_TESTS
    addTest<PreconfiguredSessionTests>();
    addTest<ClangToolsUnitTests>();
    addTest<ReadExportedDiagnosticsTest>();
#endif
}

void ClangToolsPlugin::onCurrentEditorChanged()
{
    for (Core::IEditor *editor : Core::EditorManager::visibleEditors()) {
        IDocument *document = editor->document();
        if (d->documentRunners.contains(document))
            continue;
        auto runner = new DocumentClangToolRunner(document);
        connect(runner, &DocumentClangToolRunner::destroyed, this, [this, document] {
            d->documentRunners.remove(document);
        });
        d->documentRunners[document] = runner;
    }
}

void ClangToolsPlugin::registerAnalyzeActions()
{
    const char * const menuGroupId = "ClangToolsCppGroup";
    ActionContainer * const mtoolscpp
        = ActionManager::actionContainer(CppEditor::Constants::M_TOOLS_CPP);
    if (mtoolscpp) {
        mtoolscpp->insertGroup(CppEditor::Constants::G_GLOBAL, menuGroupId);
        mtoolscpp->addSeparator(menuGroupId);
    }
    Core::ActionContainer * const mcontext = Core::ActionManager::actionContainer(
        CppEditor::Constants::M_CONTEXT);
    if (mcontext) {
        mcontext->insertGroup(CppEditor::Constants::G_GLOBAL, menuGroupId);
        mcontext->addSeparator(menuGroupId);
    }

    for (const auto &toolInfo : {std::make_tuple(ClangTidyTool::instance(),
                                                 Constants::RUN_CLANGTIDY_ON_PROJECT,
                                                 Constants::RUN_CLANGTIDY_ON_CURRENT_FILE),
                                 std::make_tuple(ClazyTool::instance(),
                                                 Constants::RUN_CLAZY_ON_PROJECT,
                                                 Constants::RUN_CLAZY_ON_CURRENT_FILE)}) {
        ClangTool * const tool = std::get<0>(toolInfo);
        ActionManager::registerAction(tool->startAction(), std::get<1>(toolInfo));
        Command *cmd = ActionManager::registerAction(tool->startOnCurrentFileAction(),
                                                     std::get<2>(toolInfo));
        if (mtoolscpp)
            mtoolscpp->addAction(cmd, menuGroupId);
        if (mcontext)
            mcontext->addAction(cmd, menuGroupId);
    }

    // add button to tool bar of C++ source files
    connect(EditorManager::instance(), &EditorManager::editorOpened, this, [](IEditor *editor) {
        if (editor->document()->filePath().isEmpty()
                || !Utils::mimeTypeForName(editor->document()->mimeType()).inherits("text/x-c++src"))
            return;
        auto *textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
        if (!textEditor)
            return;
        TextEditor::TextEditorWidget *widget = textEditor->editorWidget();
        if (!widget)
            return;
        const QIcon icon = Utils::Icon({{":/debugger/images/debugger_singleinstructionmode.png",
                                         Utils::Theme::IconsBaseColor}}).icon();
        const auto button = new QToolButton;
        button->setPopupMode(QToolButton::InstantPopup);
        button->setIcon(icon);
        button->setToolTip(Tr::tr("Analyze File..."));
        button->setProperty(Utils::StyleHelper::C_NO_ARROW, true);
        widget->toolBar()->addWidget(button);
        const auto toolsMenu = new QMenu(widget);
        button->setMenu(toolsMenu);
        for (const auto &toolInfo : {std::make_pair(ClangTidyTool::instance(),
                                                    Constants::RUN_CLANGTIDY_ON_CURRENT_FILE),
                                     std::make_pair(ClazyTool::instance(),
                                                    Constants::RUN_CLAZY_ON_CURRENT_FILE)}) {
            ClangTool * const tool = toolInfo.first;
            Command * const cmd = ActionManager::command(toolInfo.second);
            QAction *const action = toolsMenu->addAction(tool->name(), [editor, tool] {
                tool->startTool(editor->document()->filePath());
            });
            cmd->augmentActionWithShortcutToolTip(action);
        }
    });
}

} // ClangTools::Internal
