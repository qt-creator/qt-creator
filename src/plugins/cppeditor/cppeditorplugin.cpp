/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditorplugin.h"

#include "cppautocompleter.h"
#include "cppcodemodelinspectordialog.h"
#include "cppeditorconstants.h"
#include "cppeditor.h"
#include "cppeditordocument.h"
#include "cpphighlighter.h"
#include "cpphoverhandler.h"
#include "cppincludehierarchy.h"
#include "cppoutline.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"
#include "cppsnippetprovider.h"
#include "cpptypehierarchy.h"

#include <coreplugin/editormanager/editormanager.h>

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
#include <cpptools/cpptoolsconstants.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorconstants.h>

#include <utils/hostosinfo.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/theme/theme.h>

#include <QCoreApplication>
#include <QStringList>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

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
        setDisplayName(qApp->translate("OpenWith::Editors", Constants::CPPEDITOR_DISPLAY_NAME));
        addMimeType(Constants::C_SOURCE_MIMETYPE);
        addMimeType(Constants::C_HEADER_MIMETYPE);
        addMimeType(Constants::CPP_SOURCE_MIMETYPE);
        addMimeType(Constants::CPP_HEADER_MIMETYPE);

        setDocumentCreator([]() { return new CppEditorDocument; });
        setEditorWidgetCreator([]() { return new CppEditorWidget; });
        setEditorCreator([]() { return new CppEditor; });
        setAutoCompleterCreator([]() { return new CppAutoCompleter; });
        setCommentStyle(CommentDefinition::CppStyle);
        setCodeFoldingSupported(true);
        setMarksVisible(true);
        setParenthesesMatchingEnabled(true);

        setEditorActionHandlers(TextEditorActionHandler::Format
                              | TextEditorActionHandler::UnCommentSelection
                              | TextEditorActionHandler::UnCollapseAll
                              | TextEditorActionHandler::FollowSymbolUnderCursor);

        addHoverHandler(new CppHoverHandler);
    }
};

///////////////////////////////// CppEditorPlugin //////////////////////////////////

CppEditorPlugin *CppEditorPlugin::m_instance = 0;

CppEditorPlugin::CppEditorPlugin() :
    m_renameSymbolUnderCursorAction(0),
    m_findUsagesAction(0),
    m_reparseExternallyChangedFiles(0),
    m_openTypeHierarchyAction(0),
    m_openIncludeHierarchyAction(0),
    m_quickFixProvider(0)
{
    m_instance = this;
}

CppEditorPlugin::~CppEditorPlugin()
{
    m_instance = 0;
}

CppEditorPlugin *CppEditorPlugin::instance()
{
    return m_instance;
}

CppQuickFixAssistProvider *CppEditorPlugin::quickFixProvider() const
{
    return m_quickFixProvider;
}

bool CppEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"));

    addAutoReleasedObject(new CppEditorFactory);
    addAutoReleasedObject(new CppOutlineWidgetFactory);
    addAutoReleasedObject(new CppTypeHierarchyFactory);
    addAutoReleasedObject(new CppIncludeHierarchyFactory);
    addAutoReleasedObject(new CppSnippetProvider);

    m_quickFixProvider = new CppQuickFixAssistProvider;
    addAutoReleasedObject(m_quickFixProvider);
    registerQuickFixes(this);

    Context context(Constants::CPPEDITOR_ID);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);

    Command *cmd;
    ActionContainer *cppToolsMenu = ActionManager::actionContainer(CppTools::Constants::M_TOOLS_CPP);

    cmd = ActionManager::command(CppTools::Constants::SWITCH_HEADER_SOURCE);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    QAction *openPreprocessorDialog = new QAction(tr("Additional Preprocessor Directives..."), this);
    cmd = ActionManager::registerAction(openPreprocessorDialog,
                                        Constants::OPEN_PREPROCESSOR_DIALOG, context);
    cmd->setDefaultKeySequence(QKeySequence());
    connect(openPreprocessorDialog, SIGNAL(triggered()), this, SLOT(showPreProcessorDialog()));
    cppToolsMenu->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch Between Function Declaration/Definition"), this);
    cmd = ActionManager::registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F2")));
    connect(switchDeclarationDefinition, SIGNAL(triggered()),
            this, SLOT(switchDeclarationDefinition()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR_IN_NEXT_SPLIT);
    cppToolsMenu->addAction(cmd);

    QAction *openDeclarationDefinitionInNextSplit =
            new QAction(tr("Open Function Declaration/Definition in Next Split"), this);
    cmd = ActionManager::registerAction(openDeclarationDefinitionInNextSplit,
        Constants::OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT, context, true);
    cmd->setDefaultKeySequence(QKeySequence(HostOsInfo::isMacHost()
                                            ? tr("Meta+E, Shift+F2")
                                            : tr("Ctrl+E, Shift+F2")));
    connect(openDeclarationDefinitionInNextSplit, SIGNAL(triggered()),
            this, SLOT(openDeclarationDefinitionInNextSplit()));
    cppToolsMenu->addAction(cmd);

    m_findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = ActionManager::registerAction(m_findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(m_findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_openTypeHierarchyAction = new QAction(tr("Open Type Hierarchy"), this);
    cmd = ActionManager::registerAction(m_openTypeHierarchyAction, Constants::OPEN_TYPE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Shift+T") : tr("Ctrl+Shift+T")));
    connect(m_openTypeHierarchyAction, SIGNAL(triggered()), this, SLOT(openTypeHierarchy()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_openIncludeHierarchyAction = new QAction(tr("Open Include Hierarchy"), this);
    cmd = ActionManager::registerAction(m_openIncludeHierarchyAction, Constants::OPEN_INCLUDE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Shift+I") : tr("Ctrl+Shift+I")));
    connect(m_openIncludeHierarchyAction, SIGNAL(triggered()), this, SLOT(openIncludeHierarchy()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    // Refactoring sub-menu
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator();

    m_renameSymbolUnderCursorAction = new QAction(tr("Rename Symbol Under Cursor"),
                                                  this);
    cmd = ActionManager::registerAction(m_renameSymbolUnderCursorAction,
                             Constants::RENAME_SYMBOL_UNDER_CURSOR,
                             context);
    cmd->setDefaultKeySequence(QKeySequence(tr("CTRL+SHIFT+R")));
    connect(m_renameSymbolUnderCursorAction, SIGNAL(triggered()),
            this, SLOT(renameSymbolUnderCursor()));
    cppToolsMenu->addAction(cmd);

    // Update context in global context
    cppToolsMenu->addSeparator();
    m_reparseExternallyChangedFiles = new QAction(tr("Reparse Externally Changed Files"), this);
    cmd = ActionManager::registerAction(m_reparseExternallyChangedFiles, Constants::UPDATE_CODEMODEL);
    CppTools::CppModelManager *cppModelManager = CppTools::CppModelManager::instance();
    connect(m_reparseExternallyChangedFiles, SIGNAL(triggered()), cppModelManager, SLOT(updateModifiedSourceFiles()));
    cppToolsMenu->addAction(cmd);

    cppToolsMenu->addSeparator();
    QAction *inspectCppCodeModel = new QAction(tr("Inspect C++ Code Model..."), this);
    cmd = ActionManager::registerAction(inspectCppCodeModel, Constants::INSPECT_CPP_CODEMODEL);
    cmd->setDefaultKeySequence(QKeySequence(UseMacShortcuts ? tr("Meta+Shift+F12") : tr("Ctrl+Shift+F12")));
    connect(inspectCppCodeModel, SIGNAL(triggered()), this, SLOT(inspectCppCodeModel()));
    cppToolsMenu->addAction(cmd);

    contextMenu->addSeparator(context);

    cmd = ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    connect(ProgressManager::instance(), SIGNAL(taskStarted(Core::Id)),
            this, SLOT(onTaskStarted(Core::Id)));
    connect(ProgressManager::instance(), SIGNAL(allTasksFinished(Core::Id)),
            this, SLOT(onAllTasksFinished(Core::Id)));

    return true;
}

void CppEditorPlugin::extensionsInitialized()
{
    if (!HostOsInfo::isMacHost() && !HostOsInfo::isWindowsHost()) {
        FileIconProvider::registerIconOverlayForMimeType(
                    QIcon(creatorTheme()->imageFile(Theme::IconOverlayCppSource, QLatin1String(":/cppeditor/images/qt_cpp.png"))),
                    Constants::CPP_SOURCE_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(
                    QIcon(creatorTheme()->imageFile(Theme::IconOverlayCSource, QLatin1String(":/cppeditor/images/qt_c.png"))),
                    Constants::C_SOURCE_MIMETYPE);
        FileIconProvider::registerIconOverlayForMimeType(
                    QIcon(creatorTheme()->imageFile(Theme::IconOverlayCppHeader, QLatin1String(":/cppeditor/images/qt_h.png"))),
                    Constants::CPP_HEADER_MIMETYPE);
    }
}

ExtensionSystem::IPlugin::ShutdownFlag CppEditorPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

static CppEditorWidget *currentCppEditorWidget()
{
    if (IEditor *currentEditor = EditorManager::currentEditor())
        return qobject_cast<CppEditorWidget*>(currentEditor->widget());
    return 0;
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

void CppEditorPlugin::findUsages()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->findUsages();
}

void CppEditorPlugin::showPreProcessorDialog()
{
    if (CppEditorWidget *editorWidget = currentCppEditorWidget())
        editorWidget->showPreProcessorWidget();
}

void CppEditorPlugin::onTaskStarted(Id type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(false);
        m_findUsagesAction->setEnabled(false);
        m_reparseExternallyChangedFiles->setEnabled(false);
        m_openTypeHierarchyAction->setEnabled(false);
        m_openIncludeHierarchyAction->setEnabled(false);
    }
}

void CppEditorPlugin::onAllTasksFinished(Id type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(true);
        m_findUsagesAction->setEnabled(true);
        m_reparseExternallyChangedFiles->setEnabled(true);
        m_openTypeHierarchyAction->setEnabled(true);
        m_openIncludeHierarchyAction->setEnabled(true);
    }
}

void CppEditorPlugin::inspectCppCodeModel()
{
    if (m_cppCodeModelInspectorDialog) {
        ICore::raiseWindow(m_cppCodeModelInspectorDialog);
    } else {
        m_cppCodeModelInspectorDialog = new CppCodeModelInspectorDialog(ICore::mainWindow());
        m_cppCodeModelInspectorDialog->show();
    }
}

#ifdef WITH_TESTS
QList<QObject *> CppEditorPlugin::createTestObjects() const
{
    return QList<QObject *>()
        << new Tests::DoxygenTest
        ;
}
#endif

void CppEditorPlugin::openTypeHierarchy()
{
    if (currentCppEditorWidget()) {
        NavigationWidget *navigation = NavigationWidget::instance();
        navigation->activateSubWidget(Constants::TYPE_HIERARCHY_ID);
        emit typeHierarchyRequested();
    }
}

void CppEditorPlugin::openIncludeHierarchy()
{
    if (currentCppEditorWidget()) {
        NavigationWidget *navigation = NavigationWidget::instance();
        navigation->activateSubWidget(Id(Constants::INCLUDE_HIERARCHY_ID));
        emit includeHierarchyRequested();
    }
}

} // namespace Internal
} // namespace CppEditor
