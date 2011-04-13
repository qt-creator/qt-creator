/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppplugin.h"
#include "cppclasswizard.h"
#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppeditorenums.h"
#include "cppfilewizard.h"
#include "cpphoverhandler.h"
#include "cppquickfix.h"
#include "cppoutline.h"
#include "cppquickfixcollector.h"
#include "cpptypehierarchy.h"
#include "cppsnippetprovider.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/navigationwidget.h>
#include <texteditor/completionsupport.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorplugin.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <cplusplus/ModelManagerInterface.h>
#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>

#include <QtGui/QMenu>

using namespace CppEditor;
using namespace CppEditor::Internal;

enum { QUICKFIX_INTERVAL = 20 };

//////////////////////////// CppEditorFactory /////////////////////////////

CppEditorFactory::CppEditorFactory(CppPlugin *owner) :
    m_owner(owner)
{
    m_mimeTypes << QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::C_HEADER_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE);

#if !defined(Q_WS_MAC) && !defined(Q_WS_WIN)
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    Core::MimeDatabase *mimeDatabase = Core::ICore::instance()->mimeDatabase();
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_cpp.png")),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_c.png")),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_h.png")),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE)));
#endif
}

QString CppEditorFactory::id() const
{
    return QLatin1String(CppEditor::Constants::CPPEDITOR_ID);
}

QString CppEditorFactory::displayName() const
{
    return tr(CppEditor::Constants::CPPEDITOR_DISPLAY_NAME);
}

Core::IFile *CppEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *CppEditorFactory::createEditor(QWidget *parent)
{
    CPPEditorWidget *editor = new CPPEditorWidget(parent);
    editor->setRevisionsVisible(true);
    m_owner->initializeEditor(editor);
    return editor->editor();
}

QStringList CppEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// CppPlugin //////////////////////////////////

static inline
Core::Command *createSeparator(Core::ActionManager *am,
                               QObject *parent,
                               Core::Context &context,
                               const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, Core::Id(id), context);
}

CppPlugin *CppPlugin::m_instance = 0;

CppPlugin::CppPlugin() :
    m_actionHandler(0),
    m_sortedOutline(false),
    m_renameSymbolUnderCursorAction(0),
    m_findUsagesAction(0),
    m_updateCodeModelAction(0),
    m_openTypeHierarchyAction(0)
{
    m_instance = this;

    m_quickFixCollector = 0;
    m_quickFixTimer = new QTimer(this);
    m_quickFixTimer->setInterval(20);
    m_quickFixTimer->setSingleShot(true);
    connect(m_quickFixTimer, SIGNAL(timeout()), this, SLOT(quickFixNow()));
}

CppPlugin::~CppPlugin()
{
    delete m_actionHandler;
    m_instance = 0;
}

CppPlugin *CppPlugin::instance()
{
    return m_instance;
}

void CppPlugin::initializeEditor(CPPEditorWidget *editor)
{
    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // method combo box sorting
    connect(this, SIGNAL(outlineSortingChanged(bool)),
            editor, SLOT(setSortedOutline(bool)));
}

void CppPlugin::setSortedOutline(bool sorted)
{
    m_sortedOutline = sorted;
    emit outlineSortingChanged(sorted);
}

bool CppPlugin::sortedOutline() const
{
    return m_sortedOutline;
}

CppQuickFixCollector *CppPlugin::quickFixCollector() const
{ return m_quickFixCollector; }

bool CppPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"), errorMessage))
        return false;

    addAutoReleasedObject(new CppEditorFactory(this));
    addAutoReleasedObject(new CppHoverHandler);
    addAutoReleasedObject(new CppOutlineWidgetFactory);
    addAutoReleasedObject(new CppTypeHierarchyFactory);
    addAutoReleasedObject(new CppSnippetProvider);

    m_quickFixCollector = new CppQuickFixCollector;
    addAutoReleasedObject(m_quickFixCollector);
    CppQuickFixCollector::registerQuickFixes(this);

    CppFileWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);

    wizardParameters.setCategory(QLatin1String(Constants::WIZARD_CATEGORY));
    wizardParameters.setDisplayCategory(QCoreApplication::translate(Constants::WIZARD_CATEGORY,
                                                                    Constants::WIZARD_TR_CATEGORY));
    wizardParameters.setDisplayName(tr("C++ Class"));
    wizardParameters.setId(QLatin1String("A.Class"));
    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setDescription(tr("Creates a C++ header and a source file for a new class that you can add to a C++ project."));
    addAutoReleasedObject(new CppClassWizard(wizardParameters, core));

    wizardParameters.setKind(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a C++ source file that you can add to a C++ project."));
    wizardParameters.setDisplayName(tr("C++ Source File"));
    wizardParameters.setId(QLatin1String("B.Source"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Source, core));

    wizardParameters.setDescription(tr("Creates a C++ header file that you can add to a C++ project."));
    wizardParameters.setDisplayName(tr("C++ Header File"));
    wizardParameters.setId(QLatin1String("C.Header"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Header, core));

    Core::Context context(CppEditor::Constants::C_CPPEDITOR);

    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *contextMenu= am->createMenu(CppEditor::Constants::M_CONTEXT);

    Core::Command *cmd;
    Core::ActionContainer *cppToolsMenu = am->actionContainer(Core::Id(CppTools::Constants::M_TOOLS_CPP));

    QAction *jumpToDefinition = new QAction(tr("Follow Symbol Under Cursor"), this);
    cmd = am->registerAction(jumpToDefinition,
        Constants::JUMP_TO_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(jumpToDefinition, SIGNAL(triggered()),
            this, SLOT(jumpToDefinition()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch Between Method Declaration/Definition"), this);
    cmd = am->registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence("Shift+F2"));
    connect(switchDeclarationDefinition, SIGNAL(triggered()),
            this, SLOT(switchDeclarationDefinition()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = am->registerAction(m_findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(m_findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_openTypeHierarchyAction = new QAction(tr("Open Type Hierarchy"), this);
    cmd = am->registerAction(m_openTypeHierarchyAction, Constants::OPEN_TYPE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+T")));
    connect(m_openTypeHierarchyAction, SIGNAL(triggered()), this, SLOT(openTypeHierarchy()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    // Refactoring sub-menu
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = createSeparator(am, this, globalContext,
                                         Constants::SEPARATOR2);
    sep->action()->setObjectName(Constants::M_REFACTORING_MENU_INSERTION_POINT);
    contextMenu->addAction(sep);
    contextMenu->addAction(createSeparator(am, this, globalContext,
                                           Constants::SEPARATOR3));

    m_renameSymbolUnderCursorAction = new QAction(tr("Rename Symbol Under Cursor"),
                                                  this);
    cmd = am->registerAction(m_renameSymbolUnderCursorAction,
                             Constants::RENAME_SYMBOL_UNDER_CURSOR,
                             context);
    cmd->setDefaultKeySequence(QKeySequence("CTRL+SHIFT+R"));
    connect(m_renameSymbolUnderCursorAction, SIGNAL(triggered()),
            this, SLOT(renameSymbolUnderCursor()));
    cppToolsMenu->addAction(cmd);

    // Update context in global context
    cppToolsMenu->addAction(createSeparator(am, this, globalContext, CppEditor::Constants::SEPARATOR4));
    m_updateCodeModelAction = new QAction(tr("Update Code Model"), this);
    cmd = am->registerAction(m_updateCodeModelAction, Core::Id(Constants::UPDATE_CODEMODEL), globalContext);
    CPlusPlus::CppModelManagerInterface *cppModelManager = CPlusPlus::CppModelManagerInterface::instance();
    connect(m_updateCodeModelAction, SIGNAL(triggered()), cppModelManager, SLOT(updateModifiedSourceFiles()));
    cppToolsMenu->addAction(cmd);

    m_actionHandler = new TextEditor::TextEditorActionHandler(CppEditor::Constants::C_CPPEDITOR,
        TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll);

    m_actionHandler->initializeActions();

    contextMenu->addAction(createSeparator(am, this, context, CppEditor::Constants::SEPARATOR));

    cmd = am->command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = am->command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    connect(core->progressManager(), SIGNAL(taskStarted(QString)),
            this, SLOT(onTaskStarted(QString)));
    connect(core->progressManager(), SIGNAL(allTasksFinished(QString)),
            this, SLOT(onAllTasksFinished(QString)));

    connect(core->editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currentEditorChanged(Core::IEditor*)));

    readSettings();
    return true;
}

void CppPlugin::readSettings()
{
    m_sortedOutline = Core::ICore::instance()->settings()->value("CppTools/SortedMethodOverview", false).toBool();
}

void CppPlugin::writeSettings()
{
    Core::ICore::instance()->settings()->setValue("CppTools/SortedMethodOverview", m_sortedOutline);
}

void CppPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag CppPlugin::aboutToShutdown()
{
    writeSettings();
    return SynchronousShutdown;
}

void CppPlugin::switchDeclarationDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());
    if (editor)
        editor->switchDeclarationDefinition();
}

void CppPlugin::jumpToDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());
    if (editor)
        editor->jumpToDefinition();
}

void CppPlugin::renameSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());
    if (editor)
        editor->renameSymbolUnderCursor();
}

void CppPlugin::findUsages()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());
    if (editor)
        editor->findUsages();
}

void CppPlugin::quickFix(TextEditor::ITextEditor *editor)
{
    m_currentEditor = editor;
    quickFixNow();
}

void CppPlugin::quickFixNow()
{
    if (! m_currentEditor)
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *currentEditor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());

    if (CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(m_currentEditor->widget())) {
        if (currentEditor == editor) {
            if (editor->isOutdated())
                m_quickFixTimer->start(QUICKFIX_INTERVAL);
            else
                TextEditor::CompletionSupport::instance()->
                    complete(m_currentEditor, TextEditor::QuickFixCompletion, true);
        }
    }
}

void CppPlugin::onTaskStarted(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(false);
        m_findUsagesAction->setEnabled(false);
        m_updateCodeModelAction->setEnabled(false);
        m_openTypeHierarchyAction->setEnabled(false);
    }
}

void CppPlugin::onAllTasksFinished(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(true);
        m_findUsagesAction->setEnabled(true);
        m_updateCodeModelAction->setEnabled(true);
        m_openTypeHierarchyAction->setEnabled(true);
    }
}

void CppPlugin::currentEditorChanged(Core::IEditor *editor)
{
    if (! editor)
        return;

    else if (CPPEditorWidget *textEditor = qobject_cast<CPPEditorWidget *>(editor->widget())) {
        textEditor->rehighlight(/*force = */ true);
    }
}

void CppPlugin::openTypeHierarchy()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(em->currentEditor()->widget());
    if (editor) {
        Core::NavigationWidget *navigation = Core::NavigationWidget::instance();
        navigation->activateSubWidget(QLatin1String(Constants::TYPE_HIERARCHY_ID));
        emit typeHierarchyRequested();
    }
}

Q_EXPORT_PLUGIN(CppPlugin)
