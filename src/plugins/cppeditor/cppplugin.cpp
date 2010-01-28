/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <texteditor/completionsupport.h>
#include <texteditor/fontsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorplugin.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtGui/QMenu>

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
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_cpp.png"),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_c.png"),
                                                 mimeDatabase->findByType(QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)));
    iconProvider->registerIconOverlayForMimeType(QIcon(":/cppeditor/images/qt_h.png"),
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
    CPPEditor *editor = new CPPEditor(parent);
    editor->setRevisionsVisible(true);
    m_owner->initializeEditor(editor);
    return editor->editableInterface();
}

QStringList CppEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

///////////////////////////////// CppPlugin //////////////////////////////////

static inline
        Core::Command *createSeparator(Core::ActionManager *am,
                                       QObject *parent,
                                       const QList<int> &context,
                                       const char *id)
{
    QAction *separator = new QAction(parent);
    separator->setSeparator(true);
    return am->registerAction(separator, QLatin1String(id), context);
}

CppPlugin *CppPlugin::m_instance = 0;

CppPlugin::CppPlugin() :
    m_actionHandler(0),
    m_sortedMethodOverview(false),
    m_renameSymbolUnderCursorAction(0),
    m_findUsagesAction(0),
    m_updateCodeModelAction(0)

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

void CppPlugin::initializeEditor(CPPEditor *editor)
{
    m_actionHandler->setupActions(editor);

    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);

    // auto completion
    connect(editor, SIGNAL(requestAutoCompletion(TextEditor::ITextEditable*, bool)),
            TextEditor::Internal::CompletionSupport::instance(), SLOT(autoComplete(TextEditor::ITextEditable*, bool)));

    // quick fix
    connect(editor, SIGNAL(requestQuickFix(TextEditor::ITextEditable*)),
            this, SLOT(quickFix(TextEditor::ITextEditable*)));

    // method combo box sorting
    connect(this, SIGNAL(methodOverviewSortingChanged(bool)),
            editor, SLOT(setSortedMethodOverview(bool)));
}

void CppPlugin::setSortedMethodOverview(bool sorted)
{
    m_sortedMethodOverview = sorted;
    emit methodOverviewSortingChanged(sorted);
}

bool CppPlugin::sortedMethodOverview() const
{
    return m_sortedMethodOverview;
}

CPPQuickFixCollector *CppPlugin::quickFixCollector() const
{ return m_quickFixCollector; }

bool CppPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"), errorMessage))
        return false;

    addAutoReleasedObject(new CppEditorFactory(this));
    addAutoReleasedObject(new CppHoverHandler);

    m_quickFixCollector = new CPPQuickFixCollector;
    addAutoReleasedObject(m_quickFixCollector);

    CppFileWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);

    wizardParameters.setCategory(QLatin1String("O.C++"));
    wizardParameters.setDisplayCategory(tr("C++"));
    wizardParameters.setDisplayName(tr("C++ Class"));
    wizardParameters.setId(QLatin1String("A.Class"));
    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setDescription(tr("Creates a header and a source file for a new class."));
    addAutoReleasedObject(new CppClassWizard(wizardParameters, core));

    wizardParameters.setKind(Core::IWizard::FileWizard);
    wizardParameters.setDescription(tr("Creates a C++ source file."));
    wizardParameters.setDisplayName(tr("C++ Source File"));
    wizardParameters.setId(QLatin1String("B.Source"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Source, core));

    wizardParameters.setDescription(tr("Creates a C++ header file."));
    wizardParameters.setDisplayName(tr("C++ Header File"));
    wizardParameters.setId(QLatin1String("C.Header"));
    addAutoReleasedObject(new CppFileWizard(wizardParameters, Header, core));

    QList<int> context;
    context << core->uniqueIDManager()->uniqueIdentifier(CppEditor::Constants::C_CPPEDITOR);

    Core::ActionManager *am = core->actionManager();
    Core::ActionContainer *contextMenu= am->createMenu(CppEditor::Constants::M_CONTEXT);

    Core::Command *cmd;
    Core::ActionContainer *cppToolsMenu = am->actionContainer(QLatin1String(CppTools::Constants::M_TOOLS_CPP));

    QAction *jumpToDefinition = new QAction(tr("Follow Symbol Under Cursor"), this);
    cmd = am->registerAction(jumpToDefinition,
        Constants::JUMP_TO_DEFINITION, context);
    cmd->setDefaultKeySequence(QKeySequence(Qt::Key_F2));
    connect(jumpToDefinition, SIGNAL(triggered()),
            this, SLOT(jumpToDefinition()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch Between Method Declaration/Definition"), this);
    cmd = am->registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context);
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

    m_renameSymbolUnderCursorAction = new QAction(tr("Rename Symbol Under Cursor"), this);
    cmd = am->registerAction(m_renameSymbolUnderCursorAction,
        Constants::RENAME_SYMBOL_UNDER_CURSOR, context);
    cmd->setDefaultKeySequence(QKeySequence("CTRL+SHIFT+R"));
    connect(m_renameSymbolUnderCursorAction, SIGNAL(triggered()), this, SLOT(renameSymbolUnderCursor()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    // Update context in global context
    QList<int> globalContext;
    globalContext.append(Core::Constants::C_GLOBAL_ID);
    cppToolsMenu->addAction(createSeparator(am, this, globalContext, CppEditor::Constants::SEPARATOR2));
    m_updateCodeModelAction = new QAction(tr("Update code model"), this);
    cmd = am->registerAction(m_updateCodeModelAction, QLatin1String(Constants::UPDATE_CODEMODEL), globalContext);
    CppTools::CppModelManagerInterface *cppModelManager = CppTools::CppModelManagerInterface::instance();
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
    readSettings();
    return true;
}

void CppPlugin::readSettings()
{
    m_sortedMethodOverview = Core::ICore::instance()->settings()->value("CppTools/SortedMethodOverview", false).toBool();
}

void CppPlugin::writeSettings()
{
    Core::ICore::instance()->settings()->setValue("CppTools/SortedMethodOverview", m_sortedMethodOverview);
}

void CppPlugin::extensionsInitialized()
{
}

void CppPlugin::shutdown()
{
    writeSettings();
}

void CppPlugin::switchDeclarationDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->switchDeclarationDefinition();
}

void CppPlugin::jumpToDefinition()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->jumpToDefinition();
}

void CppPlugin::renameSymbolUnderCursor()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->renameSymbolUnderCursor();
}

void CppPlugin::findUsages()
{
    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *editor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());
    if (editor)
        editor->findUsages();
}

void CppPlugin::quickFix(TextEditor::ITextEditable *editable)
{
    m_currentTextEditable = editable;
    quickFixNow();
}

void CppPlugin::quickFixNow()
{
    if (! m_currentTextEditable)
        return;

    Core::EditorManager *em = Core::EditorManager::instance();
    CPPEditor *currentEditor = qobject_cast<CPPEditor*>(em->currentEditor()->widget());

    if (CPPEditor *editor = qobject_cast<CPPEditor*>(m_currentTextEditable->widget())) {
        if (currentEditor == editor) {
            if (editor->isOutdated())
                m_quickFixTimer->start(QUICKFIX_INTERVAL);
            else
                TextEditor::Internal::CompletionSupport::instance()->quickFix(m_currentTextEditable);
        }
    }
}

void CppPlugin::onTaskStarted(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(false);
        m_findUsagesAction->setEnabled(false);
        m_updateCodeModelAction->setEnabled(false);
    }
}

void CppPlugin::onAllTasksFinished(const QString &type)
{
    if (type == CppTools::Constants::TASK_INDEX) {
        m_renameSymbolUnderCursorAction->setEnabled(true);
        m_findUsagesAction->setEnabled(true);
        m_updateCodeModelAction->setEnabled(true);
    }
}

Q_EXPORT_PLUGIN(CppPlugin)
