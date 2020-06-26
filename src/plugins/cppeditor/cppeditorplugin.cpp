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
#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppeditorwidget.h"
#include "cppeditordocument.h"
#include "cpphighlighter.h"
#include "cppincludehierarchy.h"
#include "cppoutline.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"
#include "cpptypehierarchy.h"
#include "resourcepreviewhoverhandler.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#ifdef WITH_TESTS
#  include "cppdoxygen_test.h"
#endif

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icore.h>
#include <coreplugin/navigationwidget.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cpphoverhandler.h>
#include <cpptools/cpptoolsconstants.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/colorpreviewhoverhandler.h>
#include <texteditor/snippets/snippetprovider.h>

#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QStringList>

using namespace Core;
using namespace TextEditor;
using namespace Utils;
using namespace CppTools;

namespace CppEditor {
namespace Internal {

enum { QUICKFIX_INTERVAL = 20 };

//////////////////////////// CppEditorFactory /////////////////////////////

class CppEditorFactory : public TextEditorFactory
{
public:
    CppEditorFactory()
    {
        setId(Constants::CPPEDITOR_ID);
        setDisplayName(QCoreApplication::translate("OpenWith::Editors", Constants::CPPEDITOR_DISPLAY_NAME));
        addMimeType(CppTools::Constants::C_SOURCE_MIMETYPE);
        addMimeType(CppTools::Constants::C_HEADER_MIMETYPE);
        addMimeType(CppTools::Constants::CPP_SOURCE_MIMETYPE);
        addMimeType(CppTools::Constants::CPP_HEADER_MIMETYPE);
        addMimeType(CppTools::Constants::QDOC_MIMETYPE);
        addMimeType(CppTools::Constants::MOC_MIMETYPE);

        setDocumentCreator([]() { return new CppEditorDocument; });
        setEditorWidgetCreator([]() { return new CppEditorWidget; });
        setEditorCreator([]() { return new CppEditor; });
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
    void onTaskStarted(Utils::Id type);
    void onAllTasksFinished(Utils::Id type);
    void inspectCppCodeModel();

    QAction *m_reparseExternallyChangedFiles = nullptr;
    QAction *m_openTypeHierarchyAction = nullptr;
    QAction *m_openIncludeHierarchyAction = nullptr;

    CppQuickFixAssistProvider m_quickFixProvider;

    QPointer<CppCodeModelInspectorDialog> m_cppCodeModelInspectorDialog;

    QPointer<TextEditor::BaseTextEditor> m_currentEditor;

    CppOutlineWidgetFactory m_cppOutlineWidgetFactory;
    CppTypeHierarchyFactory m_cppTypeHierarchyFactory;
    CppIncludeHierarchyFactory m_cppIncludeHierarchyFactory;
    CppEditorFactory m_cppEditorFactory;
};

static CppEditorPlugin *m_instance = nullptr;

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

    SnippetProvider::registerGroup(Constants::CPP_SNIPPETS_GROUP_ID, tr("C++", "SnippetProvider"),
                                   &CppEditor::decorateEditor);

    createCppQuickFixes();

    Context context(Constants::CPPEDITOR_ID);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    contextMenu->insertGroup(Core::Constants::G_DEFAULT_ONE, Constants::G_CONTEXT_FIRST);

    Command *cmd;
    ActionContainer *cppToolsMenu = ActionManager::actionContainer(CppTools::Constants::M_TOOLS_CPP);
    ActionContainer *touchBar = ActionManager::actionContainer(Core::Constants::TOUCH_BAR);

    cmd = ActionManager::command(CppTools::Constants::SWITCH_HEADER_SOURCE);
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
    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    connect(d->m_reparseExternallyChangedFiles, &QAction::triggered, cppModelManager, &CppTools::CppModelManager::updateModifiedSourceFiles);
    cppToolsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

    cppToolsMenu->addSeparator(Core::Constants::G_DEFAULT_THREE);
    QAction *inspectCppCodeModel = new QAction(tr("Inspect C++ Code Model..."), this);
    cmd = ActionManager::registerAction(inspectCppCodeModel, Constants::INSPECT_CPP_CODEMODEL);
    cmd->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+Shift+F12") : tr("Ctrl+Shift+F12")));
    connect(inspectCppCodeModel, &QAction::triggered, d, &CppEditorPluginPrivate::inspectCppCodeModel);
    cppToolsMenu->addAction(cmd, Core::Constants::G_DEFAULT_THREE);

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
    // Add the hover handler factories here instead of in initialize()
    // so that the Clang Code Model has a chance to hook in.
    d->m_cppEditorFactory.addHoverHandler(CppModelManager::instance()->createHoverHandler());
    d->m_cppEditorFactory.addHoverHandler(new ColorPreviewHoverHandler);
    d->m_cppEditorFactory.addHoverHandler(new ResourcePreviewHoverHandler);

    if (!HostOsInfo::isMacHost() && !HostOsInfo::isWindowsHost()) {
        FileIconProvider::registerIconOverlayForMimeType(
            creatorTheme()->imageFile(Theme::IconOverlayCppSource, ":/cppeditor/images/qt_cpp.png"),
            CppTools::Constants::CPP_SOURCE_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(
            creatorTheme()->imageFile(Theme::IconOverlayCSource, ":/cppeditor/images/qt_c.png"),
            CppTools::Constants::C_SOURCE_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(
            creatorTheme()->imageFile(Theme::IconOverlayCppHeader, ":/cppeditor/images/qt_h.png"),
            CppTools::Constants::CPP_HEADER_MIMETYPE);
    }
}

static CppEditorWidget *currentCppEditorWidget()
{
    if (IEditor *currentEditor = EditorManager::currentEditor())
        return qobject_cast<CppEditorWidget*>(currentEditor->widget());
    return nullptr;
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
    if (type == CppTools::Constants::TASK_INDEX) {
        ActionManager::command(TextEditor::Constants::FIND_USAGES)->action()->setEnabled(false);
        ActionManager::command(TextEditor::Constants::RENAME_SYMBOL)->action()->setEnabled(false);
        m_reparseExternallyChangedFiles->setEnabled(false);
        m_openTypeHierarchyAction->setEnabled(false);
        m_openIncludeHierarchyAction->setEnabled(false);
    }
}

void CppEditorPluginPrivate::onAllTasksFinished(Id type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
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

#ifdef WITH_TESTS
QVector<QObject *> CppEditorPlugin::createTestObjects() const
{
    return {new Tests::DoxygenTest};
}
#endif

void CppEditorPlugin::openTypeHierarchy()
{
    if (currentCppEditorWidget()) {
        NavigationWidget::activateSubWidget(Constants::TYPE_HIERARCHY_ID, Side::Left);
        emit typeHierarchyRequested();
    }
}

void CppEditorPlugin::openIncludeHierarchy()
{
    if (currentCppEditorWidget()) {
        NavigationWidget::activateSubWidget(Constants::INCLUDE_HIERARCHY_ID, Side::Left);
        emit includeHierarchyRequested();
    }
}

} // namespace Internal
} // namespace CppEditor
