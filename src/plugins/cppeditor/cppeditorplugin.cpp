// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppeditorplugin.h"

#include "cppautocompleter.h"
#include "cppcodemodelinspectordialog.h"
#include "cppcodemodelsettings.h"
#include "cppcodemodelsettingspage.h"
#include "cppcodestylesettingspage.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppfilesettingspage.h"
#include "cppincludehierarchy.h"
#include "cppmodelmanager.h"
#include "cppoutline.h"
#include "cppprojectfile.h"
#include "cppprojectupdater.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"
#include "cppquickfixprojectsettingswidget.h"
#include "cppquickfixsettingspage.h"
#include "cpptoolsreuse.h"
#include "cpptoolssettings.h"
#include "cpptypehierarchy.h"
#include "projectinfo.h"
#include "resourcepreviewhoverhandler.h"

#ifdef WITH_TESTS
#include "compileroptionsbuilder_test.h"
#include "cppcodegen_test.h"
#include "cppcompletion_test.h"
#include "cppdoxygen_test.h"
#include "cppheadersource_test.h"
#include "cpphighlighter.h"
#include "cppincludehierarchy_test.h"
#include "cppinsertvirtualmethods.h"
#include "cpplocalsymbols_test.h"
#include "cpplocatorfilter_test.h"
#include "cppmodelmanager_test.h"
#include "cpppointerdeclarationformatter_test.h"
#include "cppquickfix_test.h"
#include "cpprenaming_test.h"
#include "cppsourceprocessor_test.h"
#include "cppuseselections_test.h"
#include "fileandtokenactions_test.h"
#include "followsymbol_switchmethoddecldef_test.h"
#include "functionutils.h"
#include "includeutils.h"
#include "projectinfo_test.h"
#include "symbolsearcher_test.h"
#include "typehierarchybuilder_test.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/colorpreviewhoverhandler.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stringtable.h>
#include <utils/theme/theme.h>

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMenu>
#include <QStringList>

using namespace CPlusPlus;
using namespace Core;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {
namespace Internal {

enum { QUICKFIX_INTERVAL = 20 };
enum { debug = 0 };

static CppEditorWidget *currentCppEditorWidget()
{
    if (IEditor *currentEditor = EditorManager::currentEditor())
        return qobject_cast<CppEditorWidget*>(currentEditor->widget());
    return nullptr;
}

//////////////////////////// CppEditorFactory /////////////////////////////

class CppEditorFactory : public TextEditorFactory
{
public:
    CppEditorFactory()
    {
        setId(Constants::CPPEDITOR_ID);
        setDisplayName(::Core::Tr::tr("C++ Editor"));
        addMimeType(Constants::C_SOURCE_MIMETYPE);
        addMimeType(Constants::C_HEADER_MIMETYPE);
        addMimeType(Constants::CPP_SOURCE_MIMETYPE);
        addMimeType(Constants::CPP_HEADER_MIMETYPE);
        addMimeType(Constants::QDOC_MIMETYPE);
        addMimeType(Constants::MOC_MIMETYPE);

        setDocumentCreator([]() { return new CppEditorDocument; });
        setEditorWidgetCreator([]() { return new CppEditorWidget; });
        setEditorCreator([]() {
            const auto editor = new BaseTextEditor;
            editor->addContext(ProjectExplorer::Constants::CXX_LANGUAGE_ID);
            return editor;
        });
        setAutoCompleterCreator([]() { return new CppAutoCompleter; });
        setCommentDefinition(CommentDefinition::CppStyle);
        setCodeFoldingSupported(true);
        setParenthesesMatchingEnabled(true);

        setEditorActionHandlers(TextEditorActionHandler::Format
                                | TextEditorActionHandler::UnCommentSelection
                                | TextEditorActionHandler::UnCollapseAll
                                | TextEditorActionHandler::FollowSymbolUnderCursor
                                | TextEditorActionHandler::FollowTypeUnderCursor
                                | TextEditorActionHandler::RenameSymbol
                                | TextEditorActionHandler::FindUsage);
    }
};

///////////////////////////////// CppEditorPlugin //////////////////////////////////

class CppEditorPluginPrivate : public QObject
{
public:
    ~CppEditorPluginPrivate()
    {
        ExtensionSystem::PluginManager::removeObject(&m_cppProjectUpdaterFactory);
        delete m_clangdSettingsPage;
    }

    void onTaskStarted(Utils::Id type);
    void onAllTasksFinished(Utils::Id type);
    void inspectCppCodeModel();

    QAction *m_reparseExternallyChangedFiles = nullptr;
    QAction *m_findRefsCategorizedAction = nullptr;
    QAction *m_openTypeHierarchyAction = nullptr;
    QAction *m_openIncludeHierarchyAction = nullptr;

    CppQuickFixAssistProvider m_quickFixProvider;
    CppQuickFixSettingsPage m_quickFixSettingsPage;

    QPointer<CppCodeModelInspectorDialog> m_cppCodeModelInspectorDialog;

    CppOutlineWidgetFactory m_cppOutlineWidgetFactory;
    CppTypeHierarchyFactory m_cppTypeHierarchyFactory;
    CppIncludeHierarchyFactory m_cppIncludeHierarchyFactory;
    CppEditorFactory m_cppEditorFactory;

    CppModelManager modelManager;
    CppCodeModelSettings m_codeModelSettings;
    CppToolsSettings settings;
    CppFileSettings m_fileSettings;
    CppFileSettingsPage m_cppFileSettingsPage{&m_fileSettings};
    CppCodeModelSettingsPage m_cppCodeModelSettingsPage{&m_codeModelSettings};
    ClangdSettingsPage *m_clangdSettingsPage = nullptr;
    CppCodeStyleSettingsPage m_cppCodeStyleSettingsPage;
    CppProjectUpdaterFactory m_cppProjectUpdaterFactory;
};

static CppEditorPlugin *m_instance = nullptr;
static QHash<FilePath, FilePath> m_headerSourceMapping;

CppEditorPlugin::CppEditorPlugin()
{
    m_instance = this;
}

CppEditorPlugin::~CppEditorPlugin()
{
    destroyCppQuickFixes();
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

CppEditorPlugin *CppEditorPlugin::instance()
{
    return m_instance;
}

CppQuickFixAssistProvider *CppEditorPlugin::quickFixProvider() const
{
    return &d->m_quickFixProvider;
}

void CppEditorPlugin::initialize()
{
    d = new CppEditorPluginPrivate;
    d->m_codeModelSettings.fromSettings(ICore::settings());

    CppModelManager::registerJsExtension();
    ExtensionSystem::PluginManager::addObject(&d->m_cppProjectUpdaterFactory);

    setupMenus();
    registerVariables();
    createCppQuickFixes();
    registerTests();

    SnippetProvider::registerGroup(Constants::CPP_SNIPPETS_GROUP_ID, Tr::tr("C++", "SnippetProvider"),
                                   &decorateCppEditor);

    connect(ProgressManager::instance(), &ProgressManager::taskStarted,
            d, &CppEditorPluginPrivate::onTaskStarted);
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            d, &CppEditorPluginPrivate::onAllTasksFinished);
}

void CppEditorPlugin::extensionsInitialized()
{
    setupProjectPanels();

    d->m_fileSettings.fromSettings(ICore::settings());
    d->m_fileSettings.addMimeInitializer();

    // Add the hover handler factories here instead of in initialize()
    // so that the Clang Code Model has a chance to hook in.
    d->m_cppEditorFactory.addHoverHandler(CppModelManager::createHoverHandler());
    d->m_cppEditorFactory.addHoverHandler(new ColorPreviewHoverHandler);
    d->m_cppEditorFactory.addHoverHandler(new ResourcePreviewHoverHandler);

    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCppSource,
                                  ProjectExplorer::Constants::FILEOVERLAY_CPP),
        Constants::CPP_SOURCE_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCSource,
                                  ProjectExplorer::Constants::FILEOVERLAY_C),
        Constants::C_SOURCE_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCppHeader,
                                  ProjectExplorer::Constants::FILEOVERLAY_H),
        Constants::CPP_HEADER_MIMETYPE);
}

static void insertIntoMenus(const QList<ActionContainer *> &menus,
                            const std::function<void(ActionContainer *)> &func)
{
    for (ActionContainer * const menu : menus)
        func(menu);
}

static void addActionToMenus(const QList<ActionContainer *> &menus, Command *cmd, Id id)
{
    insertIntoMenus(menus, [cmd, id](ActionContainer *menu) { menu->addAction(cmd, id); });
}

void CppEditorPlugin::setupMenus()
{
    ActionContainer * const cppToolsMenu = ActionManager::createMenu(Constants::M_TOOLS_CPP);
    cppToolsMenu->menu()->setTitle(Tr::tr("&C++"));
    cppToolsMenu->menu()->setEnabled(true);
    ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(cppToolsMenu);
    ActionContainer * const contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);

    insertIntoMenus({cppToolsMenu, contextMenu}, [](ActionContainer *menu) {
        menu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_SYMBOL);
        menu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_SELECTION);
        menu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_FILE);
        menu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_GLOBAL);
        menu->addSeparator(Constants::G_SELECTION);
        menu->addSeparator(Constants::G_FILE);
        menu->addSeparator(Constants::G_GLOBAL);
    });

    addPerSymbolActions();
    addActionsForSelections();
    addPerFileActions();
    addGlobalActions();

    ActionContainer * const toolsDebug
        = ActionManager::actionContainer(Core::Constants::M_TOOLS_DEBUG);
    QAction * const inspectCppCodeModel = new QAction(Tr::tr("Inspect C++ Code Model..."), this);
    Command * const cmd = ActionManager::registerAction(inspectCppCodeModel,
                                                       Constants::INSPECT_CPP_CODEMODEL);
    cmd->setDefaultKeySequence({useMacShortcuts ? Tr::tr("Meta+Shift+F12")
                                                : Tr::tr("Ctrl+Shift+F12")});
    connect(inspectCppCodeModel, &QAction::triggered,
            d, &CppEditorPluginPrivate::inspectCppCodeModel);
    toolsDebug->addAction(cmd);
}

void CppEditorPlugin::addPerSymbolActions()
{
    const QList<ActionContainer *> menus{ActionManager::actionContainer(Constants::M_TOOLS_CPP),
                                         ActionManager::actionContainer(Constants::M_CONTEXT)};
    ActionContainer * const touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);
    const Context context(Constants::CPPEDITOR_ID);
    const auto addSymbolActionToMenus = [&menus](Command *cmd) {
        addActionToMenus(menus, cmd, Constants::G_SYMBOL);
    };

    Command *cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    cmd->setTouchBarText(Tr::tr("Follow", "text on macOS touch bar"));
    addSymbolActionToMenus(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);
    addSymbolActionToMenus(ActionManager::command(
        TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT));
    addSymbolActionToMenus(ActionManager::command(
        TextEditor::Constants::FOLLOW_SYMBOL_TO_TYPE));
    addSymbolActionToMenus(ActionManager::command(
        TextEditor::Constants::FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT));

    QAction * const switchDeclarationDefinition
        = new QAction(Tr::tr("Switch Between Function Declaration/Definition"), this);
    cmd = ActionManager::registerAction(switchDeclarationDefinition,
                                        Constants::SWITCH_DECLARATION_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Shift+F2")));
    cmd->setTouchBarText(Tr::tr("Decl/Def", "text on macOS touch bar"));
    connect(switchDeclarationDefinition, &QAction::triggered,
            this, &CppEditorPlugin::switchDeclarationDefinition);
    addSymbolActionToMenus(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);

    QAction * const openDeclarationDefinitionInNextSplit =
        new QAction(Tr::tr("Open Function Declaration/Definition in Next Split"), this);
    cmd = ActionManager::registerAction(openDeclarationDefinitionInNextSplit,
                                        Constants::OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT,
                                        context, true);
    cmd->setDefaultKeySequence(QKeySequence(HostOsInfo::isMacHost()
                                                ? Tr::tr("Meta+E, Shift+F2")
                                                : Tr::tr("Ctrl+E, Shift+F2")));
    connect(openDeclarationDefinitionInNextSplit, &QAction::triggered,
            this, &CppEditorPlugin::openDeclarationDefinitionInNextSplit);
    addSymbolActionToMenus(cmd);

    addSymbolActionToMenus(ActionManager::command(TextEditor::Constants::FIND_USAGES));

    d->m_findRefsCategorizedAction = new QAction(Tr::tr("Find References With Access Type"), this);
    cmd = ActionManager::registerAction(d->m_findRefsCategorizedAction,
                                        "CppEditor.FindRefsCategorized", context);
    connect(d->m_findRefsCategorizedAction, &QAction::triggered, this, [this] {
        if (const auto w = currentCppEditorWidget()) {
            codeModelSettings()->setCategorizeFindReferences(true);
            w->findUsages();
            codeModelSettings()->setCategorizeFindReferences(false);
        }
    });
    addSymbolActionToMenus(cmd);

    addSymbolActionToMenus(ActionManager::command(TextEditor::Constants::RENAME_SYMBOL));

    d->m_openTypeHierarchyAction = new QAction(Tr::tr("Open Type Hierarchy"), this);
    cmd = ActionManager::registerAction(d->m_openTypeHierarchyAction,
                                        Constants::OPEN_TYPE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts
                                                ? Tr::tr("Meta+Shift+T") : Tr::tr("Ctrl+Shift+T")));
    connect(d->m_openTypeHierarchyAction, &QAction::triggered,
            this, &CppEditorPlugin::openTypeHierarchy);
    addSymbolActionToMenus(cmd);

    addSymbolActionToMenus(ActionManager::command(TextEditor::Constants::OPEN_CALL_HIERARCHY));

    // Refactoring sub-menu
    Command *sep = menus.last()->addSeparator(Constants::G_SYMBOL);
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
}

void CppEditorPlugin::addActionsForSelections()
{
    const QList<ActionContainer *> menus{ActionManager::actionContainer(Constants::M_TOOLS_CPP),
                                         ActionManager::actionContainer(Constants::M_CONTEXT)};
    const auto addSelectionActionToMenus = [&menus](Command *cmd) {
        addActionToMenus(menus, cmd, Constants::G_SELECTION);
    };
    addSelectionActionToMenus(ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION));
    addSelectionActionToMenus(ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION));
}

void CppEditorPlugin::addPerFileActions()
{
    const QList<ActionContainer *> menus{ActionManager::actionContainer(Constants::M_TOOLS_CPP),
                                         ActionManager::actionContainer(Constants::M_CONTEXT)};
    ActionContainer * const touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);
    const auto addFileActionToMenus = [&menus](Command *cmd) {
        addActionToMenus(menus, cmd, Constants::G_FILE);
    };
    const Context context(Constants::CPPEDITOR_ID);

    QAction * const switchAction = new QAction(Tr::tr("Switch Header/Source"), this);
    Command *cmd = ActionManager::registerAction(switchAction, Constants::SWITCH_HEADER_SOURCE,
                                                 context, true);
    cmd->setTouchBarText(Tr::tr("Header/Source", "text on macOS touch bar"));
    addFileActionToMenus(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F4));
    connect(switchAction, &QAction::triggered,
            this, [] { CppModelManager::switchHeaderSource(false); });

    QAction * const switchInNextSplitAction
        = new QAction(Tr::tr("Open Corresponding Header/Source in Next Split"), this);
    cmd = ActionManager::registerAction(
        switchInNextSplitAction, Constants::OPEN_HEADER_SOURCE_IN_NEXT_SPLIT, context, true);
    cmd->setDefaultKeySequence(QKeySequence(HostOsInfo::isMacHost()
                                                ? Tr::tr("Meta+E, F4")
                                                : Tr::tr("Ctrl+E, F4")));
    addFileActionToMenus(cmd);
    connect(switchInNextSplitAction, &QAction::triggered,
            this, [] { CppModelManager::switchHeaderSource(true); });

    QAction * const openPreprocessorDialog
        = new QAction(Tr::tr("Additional Preprocessor Directives..."), this);
    cmd = ActionManager::registerAction(openPreprocessorDialog,
                                        Constants::OPEN_PREPROCESSOR_DIALOG, context);
    cmd->setDefaultKeySequence(QKeySequence());
    connect(openPreprocessorDialog, &QAction::triggered,
            this, &CppEditorPlugin::showPreProcessorDialog);
    addFileActionToMenus(cmd);

    QAction * const showPreprocessedAction = new QAction(Tr::tr("Show Preprocessed Source"), this);
    cmd = ActionManager::registerAction(showPreprocessedAction,
                                        Constants::SHOW_PREPROCESSED_FILE, context);
    addFileActionToMenus(cmd);
    connect(showPreprocessedAction, &QAction::triggered,
            this, [] { CppModelManager::showPreprocessedFile(false); });

    QAction * const showPreprocessedInSplitAction = new QAction
        (Tr::tr("Show Preprocessed Source in Next Split"), this);
    cmd = ActionManager::registerAction(showPreprocessedInSplitAction,
                                        Constants::SHOW_PREPROCESSED_FILE_SPLIT, context);
    addFileActionToMenus(cmd);
    connect(showPreprocessedInSplitAction, &QAction::triggered,
            this, [] { CppModelManager::showPreprocessedFile(true); });

    QAction * const foldCommentsAction = new QAction(Tr::tr("Fold All Comment Blocks"), this);
    cmd = ActionManager::registerAction(foldCommentsAction,
                                        "CppTools.FoldCommentBlocks", context);
    addFileActionToMenus(cmd);
    connect(foldCommentsAction, &QAction::triggered, this, [] { CppModelManager::foldComments(); });
    QAction * const unfoldCommentsAction = new QAction(Tr::tr("Unfold All Comment Blocks"), this);
    cmd = ActionManager::registerAction(unfoldCommentsAction,
                                        "CppTools.UnfoldCommentBlocks", context);
    addFileActionToMenus(cmd);
    connect(unfoldCommentsAction, &QAction::triggered,
            this, [] { CppModelManager::unfoldComments(); });

    d->m_openIncludeHierarchyAction = new QAction(Tr::tr("Open Include Hierarchy"), this);
    cmd = ActionManager::registerAction(d->m_openIncludeHierarchyAction,
                                        Constants::OPEN_INCLUDE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts
                                                ? Tr::tr("Meta+Shift+I") : Tr::tr("Ctrl+Shift+I")));
    connect(d->m_openIncludeHierarchyAction, &QAction::triggered,
            this, &CppEditorPlugin::openIncludeHierarchy);
    addFileActionToMenus(cmd);
}

void CppEditorPlugin::addGlobalActions()
{
    const QList<ActionContainer *> menus{ActionManager::actionContainer(Constants::M_TOOLS_CPP),
                                         ActionManager::actionContainer(Constants::M_CONTEXT)};
    const auto addGlobalActionToMenus = [&menus](Command *cmd) {
        addActionToMenus(menus, cmd, Constants::G_GLOBAL);
    };

    QAction * const findUnusedFunctionsAction = new QAction(Tr::tr("Find Unused Functions"), this);
    Command *cmd = ActionManager::registerAction(findUnusedFunctionsAction,
                                                 "CppTools.FindUnusedFunctions");
    addGlobalActionToMenus(cmd);
    connect(findUnusedFunctionsAction, &QAction::triggered, this, [] {
        CppModelManager::findUnusedFunctions({});
    });

    QAction * const findUnusedFunctionsInSubProjectAction
        = new QAction(Tr::tr("Find Unused C/C++ Functions"), this);
    cmd = ActionManager::registerAction(findUnusedFunctionsInSubProjectAction,
                                        "CppTools.FindUnusedFunctionsInSubProject");
    for (ActionContainer *const projectContextMenu :
         {ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT),
          ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT)}) {
        projectContextMenu->addSeparator(ProjectExplorer::Constants::G_PROJECT_TREE);
        projectContextMenu->addAction(cmd, ProjectExplorer::Constants::G_PROJECT_TREE);
    }
    connect(findUnusedFunctionsInSubProjectAction, &QAction::triggered, this, [] {
        if (const Node *const node = ProjectTree::currentNode(); node && node->asFolderNode())
            CppModelManager::findUnusedFunctions(node->directory());
    });

    d->m_reparseExternallyChangedFiles = new QAction(Tr::tr("Reparse Externally Changed Files"),
                                                     this);
    cmd = ActionManager::registerAction(d->m_reparseExternallyChangedFiles,
                                        Constants::UPDATE_CODEMODEL);
    connect(d->m_reparseExternallyChangedFiles, &QAction::triggered,
            CppModelManager::instance(), &CppModelManager::updateModifiedSourceFiles);
    addGlobalActionToMenus(cmd);
}

void CppEditorPlugin::setupProjectPanels()
{
    const auto quickFixSettingsPanelFactory = new ProjectPanelFactory;
    quickFixSettingsPanelFactory->setPriority(100);
    quickFixSettingsPanelFactory->setId(Constants::QUICK_FIX_PROJECT_PANEL_ID);
    quickFixSettingsPanelFactory->setDisplayName(Tr::tr(Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
    quickFixSettingsPanelFactory->setCreateWidgetFunction([](Project *project) {
        return new CppQuickFixProjectSettingsWidget(project);
    });
    ProjectPanelFactory::registerFactory(quickFixSettingsPanelFactory);

    const auto fileNamesPanelFactory = new ProjectPanelFactory;
    fileNamesPanelFactory->setPriority(99);
    fileNamesPanelFactory->setDisplayName(Tr::tr("C++ File Naming"));
    fileNamesPanelFactory->setCreateWidgetFunction([](Project *project) {
        return new CppFileSettingsForProjectWidget(project);
    });
    ProjectPanelFactory::registerFactory(fileNamesPanelFactory);

    if (CppModelManager::isClangCodeModelActive()) {
        d->m_clangdSettingsPage = new ClangdSettingsPage;
        const auto clangdPanelFactory = new ProjectPanelFactory;
        clangdPanelFactory->setPriority(100);
        clangdPanelFactory->setDisplayName(Tr::tr("Clangd"));
        clangdPanelFactory->setCreateWidgetFunction([](Project *project) {
            return new ClangdProjectSettingsWidget(project);
        });
        ProjectPanelFactory::registerFactory(clangdPanelFactory);
    }
}

void CppEditorPlugin::registerVariables()
{
    MacroExpander * const expander = globalMacroExpander();

    // TODO: Per-project variants of these three?
    expander->registerVariable("Cpp:LicenseTemplate",
                               Tr::tr("The license template."),
                               []() { return CppEditorPlugin::licenseTemplate(nullptr); });
    expander->registerFileVariables("Cpp:LicenseTemplatePath",
                                    Tr::tr("The configured path to the license template"),
                                    []() { return CppEditorPlugin::licenseTemplatePath(nullptr); });
    expander->registerVariable(
        "Cpp:PragmaOnce",
        Tr::tr("Insert \"#pragma once\" instead of \"#ifndef\" include guards into header file"),
        [] { return usePragmaOnce(nullptr) ? QString("true") : QString(); });
}

void CppEditorPlugin::registerTests()
{
#ifdef WITH_TESTS
    addTest<CodegenTest>();
    addTest<CompilerOptionsBuilderTest>();
    addTest<CompletionTest>();
    addTest<CppHighlighterTest>();
    addTest<FunctionUtilsTest>();
    addTest<HeaderPathFilterTest>();
    addTest<HeaderSourceTest>();
    addTest<IncludeGroupsTest>();
    addTest<LocalSymbolsTest>();
    addTest<LocatorFilterTest>();
    addTest<ModelManagerTest>();
    addTest<PointerDeclarationFormatterTest>();
    addTest<ProjectFileCategorizerTest>();
    addTest<ProjectInfoGeneratorTest>();
    addTest<ProjectPartChooserTest>();
    addTest<SourceProcessorTest>();
    addTest<SymbolSearcherTest>();
    addTest<TypeHierarchyBuilderTest>();
    addTest<Tests::AutoCompleterTest>();
    addTest<Tests::DoxygenTest>();
    addTest<Tests::FileAndTokenActionsTest>();
    addTest<Tests::FollowSymbolTest>();
    addTest<Tests::IncludeHierarchyTest>();
    addTest<Tests::InsertVirtualMethodsTest>();
    addTest<Tests::QuickfixTest>();
    addTest<Tests::GlobalRenamingTest>();
    addTest<Tests::SelectionsTest>();
#endif
}

void CppEditorPlugin::switchDeclarationDefinition()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->switchDeclarationDefinition(/*inNextSplit*/ false);
}

void CppEditorPlugin::openDeclarationDefinitionInNextSplit()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->switchDeclarationDefinition(/*inNextSplit*/ true);
}

void CppEditorPlugin::renameSymbolUnderCursor()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->renameSymbolUnderCursor();
}

void CppEditorPlugin::showPreProcessorDialog()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->showPreProcessorWidget();
}

void CppEditorPluginPrivate::onTaskStarted(Id type)
{
    if (type == Constants::TASK_INDEX) {
        ActionManager::command(TextEditor::Constants::FIND_USAGES)->action()->setEnabled(false);
        ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action()->setEnabled(false);
        m_reparseExternallyChangedFiles->setEnabled(false);
        m_openTypeHierarchyAction->setEnabled(false);
        m_openIncludeHierarchyAction->setEnabled(false);
    }
}

void CppEditorPluginPrivate::onAllTasksFinished(Id type)
{
    if (type == Constants::TASK_INDEX) {
        ActionManager::command(TextEditor::Constants::FIND_USAGES)->action()->setEnabled(true);
        ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action()->setEnabled(true);
        m_reparseExternallyChangedFiles->setEnabled(true);
        m_openTypeHierarchyAction->setEnabled(true);
        m_openIncludeHierarchyAction->setEnabled(true);
    }
}

void CppEditorPluginPrivate::inspectCppCodeModel()
{
    if (m_cppCodeModelInspectorDialog) {
        ICore::raiseWindow(m_cppCodeModelInspectorDialog);
    } else {
        m_cppCodeModelInspectorDialog = new CppCodeModelInspectorDialog(ICore::dialogParent());
        ICore::registerWindow(m_cppCodeModelInspectorDialog, Context("CppEditor.Inspector"));
        m_cppCodeModelInspectorDialog->show();
    }
}

void CppEditorPlugin::openTypeHierarchy()
{
    if (currentCppEditorWidget()) {
        emit typeHierarchyRequested();
        NavigationWidget::activateSubWidget(Constants::TYPE_HIERARCHY_ID, Side::Left);
    }
}

void CppEditorPlugin::openIncludeHierarchy()
{
    if (currentCppEditorWidget()) {
        emit includeHierarchyRequested();
        NavigationWidget::activateSubWidget(Constants::INCLUDE_HIERARCHY_ID, Side::Left);
    }
}

void CppEditorPlugin::clearHeaderSourceCache()
{
    m_headerSourceMapping.clear();
}

FilePath CppEditorPlugin::licenseTemplatePath(Project *project)
{
    return FilePath::fromString(fileSettings(project).licenseTemplatePath);
}

QString CppEditorPlugin::licenseTemplate(Project *project)
{
    return fileSettings(project).licenseTemplate();
}

bool CppEditorPlugin::usePragmaOnce(Project *project)
{
    return fileSettings(project).headerPragmaOnce;
}

CppCodeModelSettings *CppEditorPlugin::codeModelSettings()
{
    return &d->m_codeModelSettings;
}

CppFileSettings CppEditorPlugin::fileSettings(Project *project)
{
    if (!project)
        return instance()->d->m_fileSettings;
    return CppFileSettingsForProject(project).settings();
}

#ifdef WITH_TESTS
void CppEditorPlugin::setGlobalFileSettings(const CppFileSettings &settings)
{
    instance()->d->m_fileSettings = settings;
}
#endif

static FilePaths findFilesInProject(const QStringList &names, const Project *project,
                                      FileType fileType)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << names << project;

    if (!project)
        return {};

    const auto filter = [&](const Node *n) {
        const auto fn = n->asFileNode();
        return fn && fn->fileType() == fileType && names.contains(fn->filePath().fileName());
    };
    return project->files(filter);
}

// Return the suffixes that should be checked when trying to find a
// source belonging to a header and vice versa
static QStringList matchingCandidateSuffixes(ProjectFile::Kind kind)
{
    switch (kind) {
    case ProjectFile::AmbiguousHeader:
    case ProjectFile::CHeader:
    case ProjectFile::CXXHeader:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        return mimeTypeForName(Constants::C_SOURCE_MIMETYPE).suffixes()
                + mimeTypeForName(Constants::CPP_SOURCE_MIMETYPE).suffixes()
                + mimeTypeForName(Constants::OBJECTIVE_C_SOURCE_MIMETYPE).suffixes()
                + mimeTypeForName(Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE).suffixes()
                + mimeTypeForName(Constants::CUDA_SOURCE_MIMETYPE).suffixes();
    case ProjectFile::CSource:
    case ProjectFile::ObjCSource:
        return mimeTypeForName(Constants::C_HEADER_MIMETYPE).suffixes();
    case ProjectFile::CXXSource:
    case ProjectFile::ObjCXXSource:
    case ProjectFile::CudaSource:
    case ProjectFile::OpenCLSource:
        return mimeTypeForName(Constants::CPP_HEADER_MIMETYPE).suffixes();
    default:
        return {};
    }
}

static QStringList baseNameWithAllSuffixes(const QString &baseName, const QStringList &suffixes)
{
    QStringList result;
    const QChar dot = QLatin1Char('.');
    for (const QString &suffix : suffixes) {
        QString fileName = baseName;
        fileName += dot;
        fileName += suffix;
        result += fileName;
    }
    return result;
}

static QStringList baseNamesWithAllPrefixes(const CppFileSettings &settings,
                                            const QStringList &baseNames, bool isHeader)
{
    QStringList result;
    const QStringList &sourcePrefixes = settings.sourcePrefixes;
    const QStringList &headerPrefixes = settings.headerPrefixes;

    for (const QString &name : baseNames) {
        for (const QString &prefix : isHeader ? headerPrefixes : sourcePrefixes) {
            if (name.startsWith(prefix)) {
                QString nameWithoutPrefix = name.mid(prefix.size());
                result += nameWithoutPrefix;
                for (const QString &prefix : isHeader ? sourcePrefixes : headerPrefixes)
                    result += prefix + nameWithoutPrefix;
            }
        }
        for (const QString &prefix : isHeader ? sourcePrefixes : headerPrefixes)
            result += prefix + name;

    }
    return result;
}

static QStringList baseDirWithAllDirectories(const QDir &baseDir, const QStringList &directories)
{
    QStringList result;
    for (const QString &dir : directories)
        result << QDir::cleanPath(baseDir.absoluteFilePath(dir));
    return result;
}

static int commonFilePathLength(const QString &s1, const QString &s2)
{
    int length = qMin(s1.length(), s2.length());
    for (int i = 0; i < length; ++i)
        if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseSensitive) {
            if (s1[i] != s2[i])
                return i;
        } else {
            if (s1[i].toLower() != s2[i].toLower())
                return i;
        }
    return length;
}

static FilePath correspondingHeaderOrSourceInProject(const FilePath &filePath,
                                                     const QStringList &candidateFileNames,
                                                     const Project *project,
                                                     FileType fileType,
                                                     CacheUsage cacheUsage)
{
    const FilePaths projectFiles = findFilesInProject(candidateFileNames, project, fileType);

    // Find the file having the most common path with fileName
    FilePath bestFilePath;
    int compareValue = 0;
    for (const FilePath &projectFile : projectFiles) {
        int value = commonFilePathLength(filePath.toString(), projectFile.toString());
        if (value > compareValue) {
            compareValue = value;
            bestFilePath = projectFile;
        }
    }
    if (!bestFilePath.isEmpty()) {
        QTC_ASSERT(bestFilePath.isFile(), return {});
        if (cacheUsage == CacheUsage::ReadWrite) {
            m_headerSourceMapping[filePath] = bestFilePath;
            m_headerSourceMapping[bestFilePath] = filePath;
        }
        return bestFilePath;
    }

    return {};
}

} // namespace Internal

using namespace Internal;

FilePath correspondingHeaderOrSource(const FilePath &filePath, bool *wasHeader, CacheUsage cacheUsage)
{
    ProjectFile::Kind kind = ProjectFile::classify(filePath.fileName());
    const bool isHeader = ProjectFile::isHeader(kind);
    if (wasHeader)
        *wasHeader = isHeader;
    if (const auto it = m_headerSourceMapping.constFind(filePath);
        it != m_headerSourceMapping.constEnd()) {
        return it.value();
    }

    Project * const projectForFile = ProjectManager::projectForFile(filePath);
    const CppFileSettings settings = CppEditorPlugin::fileSettings(projectForFile);

    if (debug)
        qDebug() << Q_FUNC_INFO << filePath.fileName() <<  kind;

    if (kind == ProjectFile::Unsupported)
        return {};

    const QString baseName = filePath.completeBaseName();
    const QString privateHeaderSuffix = QLatin1String("_p");
    const QStringList suffixes = matchingCandidateSuffixes(kind);

    QStringList candidateFileNames = baseNameWithAllSuffixes(baseName, suffixes);
    if (isHeader) {
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            candidateFileNames += baseNameWithAllSuffixes(sourceBaseName, suffixes);
        }
    } else {
        QString privateHeaderBaseName = baseName;
        privateHeaderBaseName.append(privateHeaderSuffix);
        candidateFileNames += baseNameWithAllSuffixes(privateHeaderBaseName, suffixes);
    }

    const QDir absoluteDir = filePath.toFileInfo().absoluteDir();
    QStringList candidateDirs(absoluteDir.absolutePath());
    // If directory is not root, try matching against its siblings
    const QStringList searchPaths = isHeader ? settings.sourceSearchPaths
                                             : settings.headerSearchPaths;
    candidateDirs += baseDirWithAllDirectories(absoluteDir, searchPaths);

    candidateFileNames += baseNamesWithAllPrefixes(settings, candidateFileNames, isHeader);

    // Try to find a file in the same or sibling directories first
    for (const QString &candidateDir : std::as_const(candidateDirs)) {
        for (const QString &candidateFileName : std::as_const(candidateFileNames)) {
            const FilePath candidateFilePath
                = FilePath::fromString(candidateDir + '/' + candidateFileName).normalizedPathName();
            if (candidateFilePath.isFile()) {
                if (cacheUsage == CacheUsage::ReadWrite) {
                    m_headerSourceMapping[filePath] = candidateFilePath;
                    if (!isHeader || !baseName.endsWith(privateHeaderSuffix))
                        m_headerSourceMapping[candidateFilePath] = filePath;
                }
                return candidateFilePath;
            }
        }
    }

    // Find files in the current project
    Project *currentProject = projectForFile;
    if (!projectForFile)
        currentProject = ProjectTree::currentProject();
    const FileType requestedFileType = isHeader ? FileType::Source : FileType::Header;
    if (currentProject) {
        const FilePath path = correspondingHeaderOrSourceInProject(
            filePath, candidateFileNames, currentProject, requestedFileType, cacheUsage);
        if (!path.isEmpty())
            return path;

    // Find files in other projects
    } else {
        const QList<ProjectInfo::ConstPtr> projectInfos = CppModelManager::projectInfos();
        for (const ProjectInfo::ConstPtr &projectInfo : projectInfos) {
            const Project *project = projectForProjectInfo(*projectInfo);
            if (project == currentProject)
                continue; // We have already checked the current project.

            const FilePath path = correspondingHeaderOrSourceInProject(
                filePath, candidateFileNames, project, requestedFileType, cacheUsage);
            if (!path.isEmpty())
                return path;
        }
    }

    return {};
}

} // namespace CppEditor
