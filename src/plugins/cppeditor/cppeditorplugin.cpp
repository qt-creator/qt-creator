/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppeditorplugin.h"

#include "cppautocompleter.h"
#include "cppcodemodelinspectordialog.h"
#include "cppcodemodelsettings.h"
#include "cppcodemodelsettingspage.h"
#include "cppcodestylesettingspage.h"
#include "cppeditorconstants.h"
#include "cppeditordocument.h"
#include "cppeditorwidget.h"
#include "cppfilesettingspage.h"
#include "cpphighlighter.h"
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
#include "stringtable.h"

#ifdef WITH_TESTS
#include "compileroptionsbuilder_test.h"
#include "cppcodegen_test.h"
#include "cppcompletion_test.h"
#include "cppdoxygen_test.h"
#include "cppheadersource_test.h"
#include "cppincludehierarchy_test.h"
#include "cppinsertvirtualmethods.h"
#include "cpplocalsymbols_test.h"
#include "cpplocatorfilter_test.h"
#include "cppmodelmanager_test.h"
#include "cpppointerdeclarationformatter_test.h"
#include "cppquickfix_test.h"
#include "cppsourceprocessor_test.h"
#include "cppuseselections_test.h"
#include "fileandtokenactions_test.h"
#include "followsymbol_switchmethoddecldef_test.h"
#include "functionutils.h"
#include "includeutils.h"
#include "projectinfo_test.h"
#include "senddocumenttracker.h"
#include "symbolsearcher_test.h"
#include "typehierarchybuilder_test.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projecttree.h>

#include <texteditor/colorpreviewhoverhandler.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>
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
        setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::CPPEDITOR_DISPLAY_NAME));
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
                                | TextEditorActionHandler::RenameSymbol);
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

    void initialize() { m_codeModelSettings.fromSettings(ICore::settings()); }
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

    QPointer<TextEditor::BaseTextEditor> m_currentEditor;

    CppOutlineWidgetFactory m_cppOutlineWidgetFactory;
    CppTypeHierarchyFactory m_cppTypeHierarchyFactory;
    CppIncludeHierarchyFactory m_cppIncludeHierarchyFactory;
    CppEditorFactory m_cppEditorFactory;

    StringTable stringTable;
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
static QHash<QString, QString> m_headerSourceMapping;

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

bool CppEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)

    d = new CppEditorPluginPrivate;
    d->initialize();

    CppModelManager::instance()->registerJsExtension();
    ExtensionSystem::PluginManager::addObject(&d->m_cppProjectUpdaterFactory);

    // Menus
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mcpptools = ActionManager::createMenu(Constants::M_TOOLS_CPP);
    QMenu *menu = mcpptools->menu();
    menu->setTitle(tr("&C++"));
    menu->setEnabled(true);
    mtools->addMenu(mcpptools);

    // Actions
    Context context(Constants::CPPEDITOR_ID);

    QAction *switchAction = new QAction(tr("Switch Header/Source"), this);
    Command *command = ActionManager::registerAction(switchAction, Constants::SWITCH_HEADER_SOURCE, context, true);
    command->setDefaultKeySequence(QKeySequence(Qt::Key_F4));
    mcpptools->addAction(command);
    connect(switchAction, &QAction::triggered, this, &CppEditorPlugin::switchHeaderSource);

    QAction *openInNextSplitAction = new QAction(tr("Open Corresponding Header/Source in Next Split"), this);
    command = ActionManager::registerAction(openInNextSplitAction, Constants::OPEN_HEADER_SOURCE_IN_NEXT_SPLIT, context, true);
    command->setDefaultKeySequence(QKeySequence(HostOsInfo::isMacHost()
                                                ? tr("Meta+E, F4")
                                                : tr("Ctrl+E, F4")));
    mcpptools->addAction(command);
    connect(openInNextSplitAction, &QAction::triggered,
            this, &CppEditorPlugin::switchHeaderSourceInNextSplit);

    MacroExpander *expander = globalMacroExpander();
    expander->registerVariable("Cpp:LicenseTemplate",
                               tr("The license template."),
                               []() { return CppEditorPlugin::licenseTemplate(); });
    expander->registerFileVariables("Cpp:LicenseTemplatePath",
                                    tr("The configured path to the license template"),
                                    []() { return CppEditorPlugin::licenseTemplatePath(); });

    expander->registerVariable(
                "Cpp:PragmaOnce",
                tr("Insert \"#pragma once\" instead of \"#ifndef\" include guards into header file"),
                [] { return usePragmaOnce() ? QString("true") : QString(); });

    const auto clangdPanelFactory = new ProjectPanelFactory;
    clangdPanelFactory->setPriority(100);
    clangdPanelFactory->setDisplayName(tr("Clangd"));
    clangdPanelFactory->setCreateWidgetFunction([](Project *project) {
        return new ClangdProjectSettingsWidget(project);
    });
    ProjectPanelFactory::registerFactory(clangdPanelFactory);

    const auto quickFixSettingsPanelFactory = new ProjectPanelFactory;
    quickFixSettingsPanelFactory->setPriority(100);
    quickFixSettingsPanelFactory->setId(Constants::QUICK_FIX_PROJECT_PANEL_ID);
    quickFixSettingsPanelFactory->setDisplayName(
        QCoreApplication::translate("CppEditor", Constants::QUICK_FIX_SETTINGS_DISPLAY_NAME));
    quickFixSettingsPanelFactory->setCreateWidgetFunction([](Project *project) {
        return new CppQuickFixProjectSettingsWidget(project);
    });
    ProjectPanelFactory::registerFactory(quickFixSettingsPanelFactory);

    SnippetProvider::registerGroup(Constants::CPP_SNIPPETS_GROUP_ID, tr("C++", "SnippetProvider"),
                                   &decorateCppEditor);

    createCppQuickFixes();

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    contextMenu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_CONTEXT_FIRST);

    Command *cmd;
    ActionContainer *cppToolsMenu = ActionManager::actionContainer(Constants::M_TOOLS_CPP);
    ActionContainer *touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);

    cmd = ActionManager::command(Constants::SWITCH_HEADER_SOURCE);
    cmd->setTouchBarText(tr("Header/Source", "text on macOS touch bar"));
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);

    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    cmd->setTouchBarText(tr("Follow", "text on macOS touch bar"));
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);

    QAction *openPreprocessorDialog = new QAction(tr("Additional Preprocessor Directives..."), this);
    cmd = ActionManager::registerAction(openPreprocessorDialog,
                                        Constants::OPEN_PREPROCESSOR_DIALOG, context);
    cmd->setDefaultKeySequence(QKeySequence());
    connect(openPreprocessorDialog, &QAction::triggered, this, &CppEditorPlugin::showPreProcessorDialog);
    cppToolsMenu->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch Between Function Declaration/Definition"), this);
    cmd = ActionManager::registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F2")));
    cmd->setTouchBarText(tr("Decl/Def", "text on macOS touch bar"));
    connect(switchDeclarationDefinition, &QAction::triggered,
            this, &CppEditorPlugin::switchDeclarationDefinition);
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);
    touchBar->addAction(cmd, Core::Constants::G_TOUCHBAR_NAVIGATION);

    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT);
    cppToolsMenu->addAction(cmd);

    QAction *openDeclarationDefinitionInNextSplit =
            new QAction(tr("Open Function Declaration/Definition in Next Split"), this);
    cmd = ActionManager::registerAction(openDeclarationDefinitionInNextSplit,
        Constants::OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT, context, true);
    cmd->setDefaultKeySequence(QKeySequence(HostOsInfo::isMacHost()
                                            ? tr("Meta+E, Shift+F2")
                                            : tr("Ctrl+E, Shift+F2")));
    connect(openDeclarationDefinitionInNextSplit, &QAction::triggered,
            this, &CppEditorPlugin::openDeclarationDefinitionInNextSplit);
    cppToolsMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::FIND_USAGES);
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);

    d->m_findRefsCategorizedAction = new QAction(tr("Find References With Access Type"), this);
    cmd = ActionManager::registerAction(d->m_findRefsCategorizedAction,
                                        "CppEditor.FindRefsCategorized", context);
    connect(d->m_findRefsCategorizedAction, &QAction::triggered, this, [this] {
        if (const auto w = currentCppEditorWidget()) {
            codeModelSettings()->setCategorizeFindReferences(true);
            w->findUsages();
            codeModelSettings()->setCategorizeFindReferences(false);
        }
    });
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);

    d->m_openTypeHierarchyAction = new QAction(tr("Open Type Hierarchy"), this);
    cmd = ActionManager::registerAction(d->m_openTypeHierarchyAction, Constants::OPEN_TYPE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+T") : tr("Ctrl+Shift+T")));
    connect(d->m_openTypeHierarchyAction, &QAction::triggered, this, &CppEditorPlugin::openTypeHierarchy);
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);

    d->m_openIncludeHierarchyAction = new QAction(tr("Open Include Hierarchy"), this);
    cmd = ActionManager::registerAction(d->m_openIncludeHierarchyAction, Constants::OPEN_INCLUDE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+I") : tr("Ctrl+Shift+I")));
    connect(d->m_openIncludeHierarchyAction, &QAction::triggered, this, &CppEditorPlugin::openIncludeHierarchy);
    contextMenu->addAction(cmd, Constants::G_CONTEXT_FIRST);
    cppToolsMenu->addAction(cmd);

    // Refactoring sub-menu
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator();
    cppToolsMenu->addAction(ActionManager::command(TextEditor::Constants::RENAME_SYMBOL));

    // Update context in global context
    cppToolsMenu->addSeparator(Core::Constants::G_DEFAULT_THREE);
    d->m_reparseExternallyChangedFiles = new QAction(tr("Reparse Externally Changed Files"), this);
    cmd = ActionManager::registerAction(d->m_reparseExternallyChangedFiles, Constants::UPDATE_CODEMODEL);
    CppModelManager *cppModelManager = CppModelManager::instance();
    connect(d->m_reparseExternallyChangedFiles, &QAction::triggered,
            cppModelManager, &CppModelManager::updateModifiedSourceFiles);
    cppToolsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    ActionContainer *toolsDebug = ActionManager::actionContainer(Core::Constants::M_TOOLS_DEBUG);
    QAction *inspectCppCodeModel = new QAction(tr("Inspect C++ Code Model..."), this);
    cmd = ActionManager::registerAction(inspectCppCodeModel, Constants::INSPECT_CPP_CODEMODEL);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+F12") : tr("Ctrl+Shift+F12")));
    connect(inspectCppCodeModel, &QAction::triggered, d, &CppEditorPluginPrivate::inspectCppCodeModel);
    toolsDebug->addAction(cmd);

    contextMenu->addSeparator(context);

    cmd = ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    connect(ProgressManager::instance(), &ProgressManager::taskStarted,
            d, &CppEditorPluginPrivate::onTaskStarted);
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            d, &CppEditorPluginPrivate::onAllTasksFinished);

    return true;
}

void CppEditorPlugin::extensionsInitialized()
{
    d->m_fileSettings.fromSettings(ICore::settings());
    if (!d->m_fileSettings.applySuffixesToMimeDB())
        qWarning("Unable to apply cpp suffixes to mime database (cpp mime types not found).\n");
    if (CppModelManager::instance()->isClangCodeModelActive())
        d->m_clangdSettingsPage = new ClangdSettingsPage;

    // Add the hover handler factories here instead of in initialize()
    // so that the Clang Code Model has a chance to hook in.
    d->m_cppEditorFactory.addHoverHandler(CppModelManager::instance()->createHoverHandler());
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
        m_cppCodeModelInspectorDialog->show();
    }
}

QVector<QObject *> CppEditorPlugin::createTestObjects() const
{
    return {
#ifdef WITH_TESTS
        new CodegenTest,
        new CompilerOptionsBuilderTest,
        new CompletionTest,
        new FunctionUtilsTest,
        new HeaderPathFilterTest,
        new HeaderSourceTest,
        new IncludeGroupsTest,
        new LocalSymbolsTest,
        new LocatorFilterTest,
        new ModelManagerTest,
        new PointerDeclarationFormatterTest,
        new ProjectFileCategorizerTest,
        new ProjectInfoGeneratorTest,
        new ProjectPartChooserTest,
        new DocumentTrackerTest,
        new SourceProcessorTest,
        new SymbolSearcherTest,
        new TypeHierarchyBuilderTest,
        new Tests::AutoCompleterTest,
        new Tests::DoxygenTest,
        new Tests::FileAndTokenActionsTest,
        new Tests::FollowSymbolTest,
        new Tests::IncludeHierarchyTest,
        new Tests::InsertVirtualMethodsTest,
        new Tests::QuickfixTest,
        new Tests::SelectionsTest,
#endif
    };
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

FilePath CppEditorPlugin::licenseTemplatePath()
{
    return FilePath::fromString(m_instance->d->m_fileSettings.licenseTemplatePath);
}

QString CppEditorPlugin::licenseTemplate()
{
    return CppFileSettings::licenseTemplate();
}

bool CppEditorPlugin::usePragmaOnce()
{
    return m_instance->d->m_fileSettings.headerPragmaOnce;
}

const QStringList &CppEditorPlugin::headerSearchPaths()
{
    return m_instance->d->m_fileSettings.headerSearchPaths;
}

const QStringList &CppEditorPlugin::sourceSearchPaths()
{
    return m_instance->d->m_fileSettings.sourceSearchPaths;
}

const QStringList &CppEditorPlugin::headerPrefixes()
{
    return m_instance->d->m_fileSettings.headerPrefixes;
}

const QStringList &CppEditorPlugin::sourcePrefixes()
{
    return m_instance->d->m_fileSettings.sourcePrefixes;
}

CppCodeModelSettings *CppEditorPlugin::codeModelSettings()
{
    return &d->m_codeModelSettings;
}

CppFileSettings *CppEditorPlugin::fileSettings()
{
    return &instance()->d->m_fileSettings;
}

void CppEditorPlugin::switchHeaderSource()
{
    CppEditor::switchHeaderSource();
}

void CppEditorPlugin::switchHeaderSourceInNextSplit()
{
    const auto otherFile = FilePath::fromString(
        correspondingHeaderOrSource(EditorManager::currentDocument()->filePath().toString()));
    if (!otherFile.isEmpty())
        EditorManager::openEditor(otherFile, Id(), EditorManager::OpenInOtherSplit);
}

static QStringList findFilesInProject(const QString &name, const Project *project)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << name << project;

    if (!project)
        return QStringList();

    QString pattern = QString(1, QLatin1Char('/'));
    pattern += name;
    const QStringList projectFiles
            = transform(project->files(Project::AllFiles), &FilePath::toString);
    const QStringList::const_iterator pcend = projectFiles.constEnd();
    QStringList candidateList;
    for (QStringList::const_iterator it = projectFiles.constBegin(); it != pcend; ++it) {
        if (it->endsWith(pattern, HostOsInfo::fileNameCaseSensitivity()))
            candidateList.append(*it);
    }
    return candidateList;
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
        return QStringList();
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

static QStringList baseNamesWithAllPrefixes(const QStringList &baseNames, bool isHeader)
{
    QStringList result;
    const QStringList &sourcePrefixes = m_instance->sourcePrefixes();
    const QStringList &headerPrefixes = m_instance->headerPrefixes();

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

static QString correspondingHeaderOrSourceInProject(const QFileInfo &fileInfo,
                                                    const QStringList &candidateFileNames,
                                                    const Project *project,
                                                    CacheUsage cacheUsage)
{
    QString bestFileName;
    int compareValue = 0;
    const QString filePath = fileInfo.filePath();
    for (const QString &candidateFileName : candidateFileNames) {
        const QStringList projectFiles = findFilesInProject(candidateFileName, project);
        // Find the file having the most common path with fileName
        for (const QString &projectFile : projectFiles) {
            int value = commonFilePathLength(filePath, projectFile);
            if (value > compareValue) {
                compareValue = value;
                bestFileName = projectFile;
            }
        }
    }
    if (!bestFileName.isEmpty()) {
        const QFileInfo candidateFi(bestFileName);
        QTC_ASSERT(candidateFi.isFile(), return QString());
        if (cacheUsage == CacheUsage::ReadWrite) {
            m_headerSourceMapping[fileInfo.absoluteFilePath()] = candidateFi.absoluteFilePath();
            m_headerSourceMapping[candidateFi.absoluteFilePath()] = fileInfo.absoluteFilePath();
        }
        return candidateFi.absoluteFilePath();
    }

    return QString();
}

} // namespace Internal

using namespace Internal;

QString correspondingHeaderOrSource(const QString &fileName, bool *wasHeader, CacheUsage cacheUsage)
{
    const QFileInfo fi(fileName);
    ProjectFile::Kind kind = ProjectFile::classify(fileName);
    const bool isHeader = ProjectFile::isHeader(kind);
    if (wasHeader)
        *wasHeader = isHeader;
    if (m_headerSourceMapping.contains(fi.absoluteFilePath()))
        return m_headerSourceMapping.value(fi.absoluteFilePath());

    if (debug)
        qDebug() << Q_FUNC_INFO << fileName <<  kind;

    if (kind == ProjectFile::Unsupported)
        return QString();

    const QString baseName = fi.completeBaseName();
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

    const QDir absoluteDir = fi.absoluteDir();
    QStringList candidateDirs(absoluteDir.absolutePath());
    // If directory is not root, try matching against its siblings
    const QStringList searchPaths = isHeader ? m_instance->sourceSearchPaths()
                                             : m_instance->headerSearchPaths();
    candidateDirs += baseDirWithAllDirectories(absoluteDir, searchPaths);

    candidateFileNames += baseNamesWithAllPrefixes(candidateFileNames, isHeader);

    // Try to find a file in the same or sibling directories first
    for (const QString &candidateDir : qAsConst(candidateDirs)) {
        for (const QString &candidateFileName : qAsConst(candidateFileNames)) {
            const QString candidateFilePath = candidateDir + QLatin1Char('/') + candidateFileName;
            const QString normalized = FileUtils::normalizedPathName(candidateFilePath);
            const QFileInfo candidateFi(normalized);
            if (candidateFi.isFile()) {
                if (cacheUsage == CacheUsage::ReadWrite) {
                    m_headerSourceMapping[fi.absoluteFilePath()] = candidateFi.absoluteFilePath();
                    if (!isHeader || !baseName.endsWith(privateHeaderSuffix))
                        m_headerSourceMapping[candidateFi.absoluteFilePath()] = fi.absoluteFilePath();
                }
                return candidateFi.absoluteFilePath();
            }
        }
    }

    // Find files in the current project
    Project *currentProject = ProjectTree::currentProject();
    if (currentProject) {
        const QString path = correspondingHeaderOrSourceInProject(fi, candidateFileNames,
                                                                  currentProject, cacheUsage);
        if (!path.isEmpty())
            return path;

    // Find files in other projects
    } else {
        CppModelManager *modelManager = CppModelManager::instance();
        const QList<ProjectInfo::ConstPtr> projectInfos = modelManager->projectInfos();
        for (const ProjectInfo::ConstPtr &projectInfo : projectInfos) {
            const Project *project = projectForProjectInfo(*projectInfo);
            if (project == currentProject)
                continue; // We have already checked the current project.

            const QString path = correspondingHeaderOrSourceInProject(fi, candidateFileNames,
                                                                      project, cacheUsage);
            if (!path.isEmpty())
                return path;
        }
    }

    return QString();
}

} // namespace CppEditor
