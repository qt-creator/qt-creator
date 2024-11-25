// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdsettings.h"
#include "cppautocompleter.h"
#include "cppcodemodelinspectordialog.h"
#include "cppcodemodelsettings.h"
#include "cppcodestylesettingspage.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppfilesettingspage.h"
#include "cpphighlighter.h"
#include "cppincludehierarchy.h"
#include "cppheadersource.h"
#include "cppmodelmanager.h"
#include "cppoutline.h"
#include "cppprojectupdater.h"
#include "cpptoolsreuse.h"
#include "cpptoolssettings.h"
#include "cpptypehierarchy.h"
#include "quickfixes/cppquickfix.h"
#include "quickfixes/cppquickfixprojectsettingswidget.h"
#include "quickfixes/cppquickfixsettingspage.h"
#include "resourcepreviewhoverhandler.h"

#ifdef WITH_TESTS
#include "compileroptionsbuilder_test.h"
#include "cppcodegen_test.h"
#include "cppcompletion_test.h"
#include "cppdoxygen_test.h"
#include "cppincludehierarchy_test.h"
#include "cpplocalsymbols_test.h"
#include "cpplocatorfilter_test.h"
#include "cppmodelmanager_test.h"
#include "cpppointerdeclarationformatter_test.h"
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

#include <extensionsystem/iplugin.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/rawprojectpart.h>

#include <texteditor/colorpreviewhoverhandler.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
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

namespace CppEditor::Internal {

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
        addMimeType(Utils::Constants::C_SOURCE_MIMETYPE);
        addMimeType(Utils::Constants::C_HEADER_MIMETYPE);
        addMimeType(Utils::Constants::CPP_SOURCE_MIMETYPE);
        addMimeType(Utils::Constants::CPP_HEADER_MIMETYPE);
        addMimeType(Utils::Constants::QDOC_MIMETYPE);
        addMimeType(Utils::Constants::MOC_MIMETYPE);

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

        setOptionalActionMask(OptionalActions::Format
                                | OptionalActions::UnCommentSelection
                                | OptionalActions::UnCollapseAll
                                | OptionalActions::FollowSymbolUnderCursor
                                | OptionalActions::FollowTypeUnderCursor
                                | OptionalActions::RenameSymbol
                                | OptionalActions::TypeHierarchy
                                | OptionalActions::FindUsage);
    }
};

///////////////////////////////// CppEditorPlugin //////////////////////////////////

class CppEditorPluginPrivate : public QObject
{
public:
    void onTaskStarted(Utils::Id type);
    void onAllTasksFinished(Utils::Id type);

    QAction *m_reparseExternallyChangedFiles = nullptr;
    QAction *m_findRefsCategorizedAction = nullptr;

    CppEditorFactory m_cppEditorFactory;

    CppModelManager modelManager;
    CppToolsSettings settings;
};

class CppEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    ~CppEditorPlugin() final
    {
        destroyCppQuickFixFactories();
        delete d;
        d = nullptr;
    }

private:
    void initialize() final;
    void extensionsInitialized() final;

    void setupMenus();
    void addPerSymbolActions();
    void addActionsForSelections();
    void addPerFileActions();
    void addGlobalActions();
    void registerVariables();
    void registerTests();

    CppEditorPluginPrivate *d = nullptr;
};

QFuture<QTextDocument *> highlightCode(const QString &code, const QString &mimeType)
{
    QTextDocument *document = new QTextDocument;
    document->setPlainText(code);

    std::shared_ptr<QPromise<QTextDocument *>> promise
        = std::make_shared<QPromise<QTextDocument *>>();

    promise->start();

    CppHighlighter *highlighter = new CppHighlighter(document);

    QObject::connect(highlighter, &CppHighlighter::finished, document, [document, promise]() {
        promise->addResult(document);
        promise->finish();
    });

    QFutureWatcher<QTextDocument *> *watcher = new QFutureWatcher<QTextDocument *>(document);
    QObject::connect(watcher, &QFutureWatcher<QTextDocument *>::canceled, document, [document]() {
        document->deleteLater();
    });
    watcher->setFuture(promise->future());

    highlighter->setParent(document);
    highlighter->setFontSettings(TextEditorSettings::fontSettings());
    highlighter->setMimeType(mimeType);
    highlighter->rehighlight();

    return promise->future();
}

void CppEditorPlugin::initialize()
{
    d = new CppEditorPluginPrivate;

    setupCppQuickFixSettings();
    setupCppCodeModelSettingsPage();
    provideCppSettingsRetriever([](const Project *p) {
        return CppCodeModelSettings::settingsForProject(p).toMap();
    });
    setupCppOutline();
    setupCppCodeStyleSettings();
    setupCppProjectUpdater();

    CppModelManager::registerJsExtension();

    setupMenus();
    registerVariables();
    createCppQuickFixFactories();
    registerTests();

    SnippetProvider::registerGroup(Constants::CPP_SNIPPETS_GROUP_ID, Tr::tr("C++", "SnippetProvider"),
                                   &decorateCppEditor);

    connect(ProgressManager::instance(), &ProgressManager::taskStarted,
            d, &CppEditorPluginPrivate::onTaskStarted);
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            d, &CppEditorPluginPrivate::onAllTasksFinished);

    auto oldHighlighter = Utils::Text::codeHighlighter();
    Utils::Text::setCodeHighlighter(
        [oldHighlighter](const QString &code, const QString &mimeType) -> QFuture<QTextDocument *> {
            if (mimeType == "text/x-c++src" || mimeType == "text/x-c++hdr"
                || mimeType == "text/x-csrc" || mimeType == "text/x-chdr") {
                return highlightCode(code, mimeType);
            }

            return oldHighlighter(code, mimeType);
        });
}

void CppEditorPlugin::extensionsInitialized()
{
    setupCppQuickFixProjectPanel();
    setupCppFileSettings(*this);
    setupCppCodeModelProjectSettingsPanel();

    if (CppModelManager::isClangCodeModelActive()) {
        setupClangdProjectSettingsPanel();
        setupClangdSettingsPage();
    }

    // Add the hover handler factories here instead of in initialize()
    // so that the Clang Code Model has a chance to hook in.
    d->m_cppEditorFactory.addHoverHandler(CppModelManager::createHoverHandler());
    d->m_cppEditorFactory.addHoverHandler(new ColorPreviewHoverHandler);
    d->m_cppEditorFactory.addHoverHandler(new ResourcePreviewHoverHandler);

    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCppSource,
                                  ProjectExplorer::Constants::FILEOVERLAY_CPP),
        Utils::Constants::CPP_SOURCE_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCSource,
                                  ProjectExplorer::Constants::FILEOVERLAY_C),
        Utils::Constants::C_SOURCE_MIMETYPE);
    FileIconProvider::registerIconOverlayForMimeType(
        creatorTheme()->imageFile(Theme::IconOverlayCppHeader,
                                  ProjectExplorer::Constants::FILEOVERLAY_H),
        Utils::Constants::CPP_HEADER_MIMETYPE);
}

static void insertIntoMenus(const QList<ActionContainer *> &menus,
                            const std::function<void(ActionContainer *)> &func)
{
    for (ActionContainer * const menu : menus)
        func(menu);
}

static void addActionToMenus(const QList<Id> &menuIds, Id actionId, Id groupId)
{
    for (const Id menuId : menuIds) {
        ActionContainer * const menu = ActionManager::actionContainer(menuId);
        menu->addAction(ActionManager::command(actionId), groupId);
    }
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

    ActionBuilder inspectCppCodeModel(this, Constants::INSPECT_CPP_CODEMODEL);
    inspectCppCodeModel.setText(Tr::tr("Inspect C++ Code Model..."));
    inspectCppCodeModel.setDefaultKeySequence(Tr::tr("Meta+Shift+F12"), Tr::tr("Ctrl+Shift+F12"));
    inspectCppCodeModel.addToContainer(Core::Constants::M_TOOLS_DEBUG);
    inspectCppCodeModel.addOnTriggered(d, &Internal::inspectCppCodeModel);
}

void CppEditorPlugin::addPerSymbolActions()
{
    const QList<Id> menus{Constants::M_TOOLS_CPP, Constants::M_CONTEXT};
    const auto addSymbolActionToMenus = [&menus](Id id) {
        addActionToMenus(menus, id, Constants::G_SYMBOL);
    };
    const Context context(Constants::CPPEDITOR_ID);

    addSymbolActionToMenus(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    Command *cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    cmd->setTouchBarText(Tr::tr("Follow", "text on macOS touch bar"));
    ActionContainer * const touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);

    addSymbolActionToMenus(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT);
    addSymbolActionToMenus(TextEditor::Constants::FOLLOW_SYMBOL_TO_TYPE);
    addSymbolActionToMenus(TextEditor::Constants::FOLLOW_SYMBOL_TO_TYPE_IN_NEXT_SPLIT);

    ActionBuilder switchDeclDef(this, Constants::SWITCH_DECLARATION_DEFINITION);
    switchDeclDef.setText(Tr::tr("Switch Between Function Declaration/Definition"));
    switchDeclDef.setContext(context);
    switchDeclDef.setScriptable(true);
    switchDeclDef.setDefaultKeySequence(Tr::tr("Shift+F2"));
    switchDeclDef.setTouchBarText(Tr::tr("Decl/Def", "text on macOS touch bar"));
    switchDeclDef.addToContainers(menus, Constants::G_SYMBOL);
    switchDeclDef.addToContainer(Core::Constants::TOUCH_BAR, Core::Constants::G_TOUCHBAR_NAVIGATION);
    switchDeclDef.addOnTriggered(this, [] {
        if (CppEditorWidget *editorWidget = currentCppEditorWidget())
            editorWidget->switchDeclarationDefinition(/*inNextSplit*/ false);
    });

    ActionBuilder openDeclDefSplit(this, Constants::OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT);
    openDeclDefSplit.setText(Tr::tr("Open Function Declaration/Definition in Next Split"));
    openDeclDefSplit.setContext(context);
    openDeclDefSplit.setScriptable(true);
    openDeclDefSplit.setDefaultKeySequence(Tr::tr("Meta+E, Shift+F2"), Tr::tr("Ctrl+E, Shift+F2"));
    openDeclDefSplit.addToContainers(menus, Constants::G_SYMBOL);
    openDeclDefSplit.addOnTriggered(this, [] {
        if (CppEditorWidget *editorWidget = currentCppEditorWidget())
            editorWidget->switchDeclarationDefinition(/*inNextSplit*/ true);
    });

    addSymbolActionToMenus(TextEditor::Constants::FIND_USAGES);

    ActionBuilder findRefsCategorized(this,  "CppEditor.FindRefsCategorized");
    findRefsCategorized.setText(Tr::tr("Find References With Access Type"));
    findRefsCategorized.setContext(context);
    findRefsCategorized.bindContextAction(&d->m_findRefsCategorizedAction);
    findRefsCategorized.addToContainers(menus, Constants::G_SYMBOL);
    findRefsCategorized.addOnTriggered(this, [] {
        if (const auto w = currentCppEditorWidget()) {
            CppCodeModelSettings::setCategorizeFindReferences(true);
            w->findUsages();
            CppCodeModelSettings::setCategorizeFindReferences(false);
        }
    });

    addSymbolActionToMenus(TextEditor::Constants::RENAME_SYMBOL);

    setupCppTypeHierarchy();

    addSymbolActionToMenus(TextEditor::Constants::OPEN_TYPE_HIERARCHY);
    addSymbolActionToMenus(TextEditor::Constants::OPEN_CALL_HIERARCHY);

    // Refactoring sub-menu
    Command *sep = ActionManager::actionContainer(Constants::M_CONTEXT)
        ->addSeparator(Constants::G_SYMBOL);
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
}

void CppEditorPlugin::addActionsForSelections()
{
    const QList<Id> menus{Constants::M_TOOLS_CPP,  Constants::M_CONTEXT};

    addActionToMenus(menus, TextEditor::Constants::AUTO_INDENT_SELECTION, Constants::G_SELECTION);
    addActionToMenus(menus, TextEditor::Constants::UN_COMMENT_SELECTION, Constants::G_SELECTION);
}

void CppEditorPlugin::addPerFileActions()
{
    const QList<Id> menus{Constants::M_TOOLS_CPP, Constants::M_CONTEXT};
    const Context context(Constants::CPPEDITOR_ID);

    ActionBuilder switchAction(this, Constants::SWITCH_HEADER_SOURCE);
    switchAction.setText(Tr::tr("Switch Header/Source"));
    switchAction.setContext(context);
    switchAction.setScriptable(true);
    switchAction.setTouchBarText(Tr::tr("Header/Source", "text on macOS touch bar"));
    switchAction.addToContainers(menus, Constants::G_FILE);
    switchAction.addToContainer(Core::Constants::TOUCH_BAR, Core::Constants::G_TOUCHBAR_NAVIGATION);
    switchAction.setDefaultKeySequence(Qt::Key_F4);
    switchAction.addOnTriggered([] { CppModelManager::switchHeaderSource(false); });

    ActionBuilder switchInNextSplit(this, Constants::OPEN_HEADER_SOURCE_IN_NEXT_SPLIT);
    switchInNextSplit.setText(Tr::tr("Open Corresponding Header/Source in Next Split"));
    switchInNextSplit.setContext(context);
    switchInNextSplit.setScriptable(true);
    switchInNextSplit.setDefaultKeySequence(Tr::tr("Meta+E, F4"), Tr::tr("Ctrl+E, F4"));
    switchInNextSplit.addToContainers(menus, Constants::G_FILE);
    switchInNextSplit.addOnTriggered([] { CppModelManager::switchHeaderSource(true); });

    ActionBuilder openPreprocessor(this, Constants::OPEN_PREPROCESSOR_DIALOG);
    openPreprocessor.setText(Tr::tr("Additional Preprocessor Directives..."));
    openPreprocessor.setContext(context);
    openPreprocessor.setDefaultKeySequence({});
    openPreprocessor.addToContainers(menus, Constants::G_FILE);
    openPreprocessor.addOnTriggered(this, [] {
        if (CppEditorWidget *editorWidget = currentCppEditorWidget())
            editorWidget->showPreProcessorWidget();
    });

    ActionBuilder showPreprocessed(this, Constants::SHOW_PREPROCESSED_FILE);
    showPreprocessed.setText(Tr::tr("Show Preprocessed Source"));
    showPreprocessed.setContext(context);
    showPreprocessed.addToContainers(menus, Constants::G_FILE);
    showPreprocessed.addOnTriggered(this, [] { CppModelManager::showPreprocessedFile(false); });

    ActionBuilder showPreprocessedInSplit(this, Constants::SHOW_PREPROCESSED_FILE_SPLIT);
    showPreprocessedInSplit.setText(Tr::tr("Show Preprocessed Source in Next Split"));
    showPreprocessedInSplit.setContext(context);
    showPreprocessedInSplit.addToContainers(menus, Constants::G_FILE);
    showPreprocessedInSplit.addOnTriggered([] { CppModelManager::showPreprocessedFile(true); });

    ActionBuilder foldComments(this, "CppTools.FoldCommentBlocks");
    foldComments.setText(Tr::tr("Fold All Comment Blocks"));
    foldComments.setContext(context);
    foldComments.addToContainers(menus, Constants::G_FILE);
    foldComments.addOnTriggered(this, [] { CppModelManager::foldComments(); });

    ActionBuilder unfoldComments(this, "CppTools.UnfoldCommentBlocks");
    unfoldComments.setText(Tr::tr("Unfold All Comment Blocks"));
    unfoldComments.setContext(context);
    unfoldComments.addToContainers(menus, Constants::G_FILE);
    unfoldComments.addOnTriggered(this, [] { CppModelManager::unfoldComments(); });

    setupCppIncludeHierarchy();
}

void CppEditorPlugin::addGlobalActions()
{
    const QList<Id> menus{Constants::M_TOOLS_CPP, Constants::M_CONTEXT};

    ActionBuilder findUnusedFunctions(this, "CppTools.FindUnusedFunctions");
    findUnusedFunctions.setText(Tr::tr("Find Unused Functions"));
    findUnusedFunctions.addToContainers(menus, Constants::G_GLOBAL);
    findUnusedFunctions.addOnTriggered(this, [] { CppModelManager::findUnusedFunctions({}); });

    ActionBuilder findUnusedFunctionsSubProject(this, "CppTools.FindUnusedFunctionsInSubProject");
    findUnusedFunctionsSubProject.setText(Tr::tr("Find Unused C/C++ Functions"));
    for (ActionContainer *const projectContextMenu :
         {ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT),
          ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT)}) {
        projectContextMenu->addSeparator(ProjectExplorer::Constants::G_PROJECT_TREE);
        projectContextMenu->addAction(findUnusedFunctionsSubProject.command(),
                                      ProjectExplorer::Constants::G_PROJECT_TREE);
    }
    findUnusedFunctionsSubProject.addOnTriggered(this, [] {
        if (const Node *const node = ProjectTree::currentNode(); node && node->asFolderNode())
            CppModelManager::findUnusedFunctions(node->directory());
    });

    ActionBuilder reparseChangedFiles(this,  Constants::UPDATE_CODEMODEL);
    reparseChangedFiles.setText(Tr::tr("Reparse Externally Changed Files"));
    reparseChangedFiles.bindContextAction(&d->m_reparseExternallyChangedFiles);
    reparseChangedFiles.addToContainers(menus, Constants::G_GLOBAL);
    reparseChangedFiles.addOnTriggered(CppModelManager::instance(),
                                       &CppModelManager::updateModifiedSourceFiles);
}

void CppEditorPlugin::registerVariables()
{
    MacroExpander * const expander = globalMacroExpander();

    // TODO: Per-project variants of these three?
    expander->registerVariable("Cpp:LicenseTemplate",
        Tr::tr("The license template."),
        [] { return globalCppFileSettings().licenseTemplate(); });
    expander->registerFileVariables("Cpp:LicenseTemplatePath",
        Tr::tr("The configured path to the license template"),
        [] { return globalCppFileSettings().licenseTemplatePath; });
    expander->registerVariable(
        "Cpp:PragmaOnce",
        Tr::tr("Insert \"#pragma once\" instead of \"#ifndef\" include guards into header file"),
        [] { return globalCppFileSettings().headerPragmaOnce ? QString("true") : QString(); });
}

void CppEditorPlugin::registerTests()
{
    registerHighlighterTests(*this);
#ifdef WITH_TESTS
    addTest<CodegenTest>();
    addTest<CompilerOptionsBuilderTest>();
    addTest<CompletionTest>();
    addTest<FunctionUtilsTest>();
    addTest<HeaderPathFilterTest>();
    addTestCreator(createCppHeaderSourceTest);
    addTestCreator(createIncludeGroupsTest);
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
    addTest<Tests::GlobalRenamingTest>();
    addTest<Tests::SelectionsTest>();
#endif
}

void CppEditorPluginPrivate::onTaskStarted(Id type)
{
    if (type == Constants::TASK_INDEX) {
        ActionManager::command(TextEditor::Constants::FIND_USAGES)->action()->setEnabled(false);
        ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action()->setEnabled(false);
        m_reparseExternallyChangedFiles->setEnabled(false);
    }
}

void CppEditorPluginPrivate::onAllTasksFinished(Id type)
{
    if (type == Constants::TASK_INDEX) {
        ActionManager::command(TextEditor::Constants::FIND_USAGES)->action()->setEnabled(true);
        ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action()->setEnabled(true);
        m_reparseExternallyChangedFiles->setEnabled(true);
    }
}

} // CppEditor::Internal

#include "cppeditorplugin.moc"
