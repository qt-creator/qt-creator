/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppplugin.h"
#include "cppclasswizard.h"
#include "cppeditor.h"
#include "cppeditorconstants.h"
#include "cppeditorenums.h"
#include "cppfilewizard.h"
#include "cpphoverhandler.h"
#include "cppoutline.h"
#include "cpptypehierarchy.h"
#include "cppsnippetprovider.h"
#include "cppquickfixassistant.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/id.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/navigationwidget.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorplugin.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditorconstants.h>
#include <utils/hostosinfo.h>
#include <cpptools/ModelManagerInterface.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolssettings.h>

#include <QFileInfo>
#include <QSettings>
#include <QTimer>
#include <QCoreApplication>
#include <QStringList>

#include <QMenu>

using namespace CppEditor;
using namespace CppEditor::Internal;

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);

enum { QUICKFIX_INTERVAL = 20 };

//////////////////////////// CppEditorFactory /////////////////////////////

CppEditorFactory::CppEditorFactory(CppPlugin *owner) :
    m_owner(owner)
{
    m_mimeTypes << QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::C_HEADER_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)
            << QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE);

    if (!Utils::HostOsInfo::isMacHost() && !Utils::HostOsInfo::isWindowsHost()) {
        Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
        Core::MimeDatabase *mimeDatabase = Core::ICore::mimeDatabase();
        iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_cpp.png")),
                                                     mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_SOURCE_MIMETYPE)));
        iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_c.png")),
                                                     mimeDatabase->findByType(QLatin1String(CppEditor::Constants::C_SOURCE_MIMETYPE)));
        iconProvider->registerIconOverlayForMimeType(QIcon(QLatin1String(":/cppeditor/images/qt_h.png")),
                                                     mimeDatabase->findByType(QLatin1String(CppEditor::Constants::CPP_HEADER_MIMETYPE)));
    }
}

Core::Id CppEditorFactory::id() const
{
    return Core::Id(CppEditor::Constants::CPPEDITOR_ID);
}

QString CppEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", CppEditor::Constants::CPPEDITOR_DISPLAY_NAME);
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

CppPlugin *CppPlugin::m_instance = 0;

CppPlugin::CppPlugin() :
    m_actionHandler(0),
    m_sortedOutline(false),
    m_renameSymbolUnderCursorAction(0),
    m_findUsagesAction(0),
    m_updateCodeModelAction(0),
    m_openTypeHierarchyAction(0),
    m_quickFixProvider(0)
{
    m_instance = this;
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

    editor->setLanguageSettingsId(CppTools::Constants::CPP_SETTINGS_ID);
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

CppQuickFixAssistProvider *CppPlugin::quickFixProvider() const
{
    return m_quickFixProvider;
}

bool CppPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/cppeditor/CppEditor.mimetypes.xml"), errorMessage))
        return false;

    addAutoReleasedObject(new CppEditorFactory(this));
    addAutoReleasedObject(new CppHoverHandler);
    addAutoReleasedObject(new CppOutlineWidgetFactory);
    addAutoReleasedObject(new CppTypeHierarchyFactory);
    addAutoReleasedObject(new CppSnippetProvider);

    m_quickFixProvider = new CppQuickFixAssistProvider;
    addAutoReleasedObject(m_quickFixProvider);
    registerQuickFixes(this);

    QObject *core = Core::ICore::instance();
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

    Core::ActionContainer *contextMenu= Core::ActionManager::createMenu(CppEditor::Constants::M_CONTEXT);

    Core::Command *cmd;
    Core::ActionContainer *cppToolsMenu = Core::ActionManager::actionContainer(Core::Id(CppTools::Constants::M_TOOLS_CPP));

    cmd = Core::ActionManager::command(Core::Id(CppTools::Constants::SWITCH_HEADER_SOURCE));
    contextMenu->addAction(cmd);

    cmd = Core::ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    QAction *switchDeclarationDefinition = new QAction(tr("Switch Between Method Declaration/Definition"), this);
    cmd = Core::ActionManager::registerAction(switchDeclarationDefinition,
        Constants::SWITCH_DECLARATION_DEFINITION, context, true);
    cmd->setDefaultKeySequence(QKeySequence(tr("Shift+F2")));
    connect(switchDeclarationDefinition, SIGNAL(triggered()),
            this, SLOT(switchDeclarationDefinition()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = Core::ActionManager::registerAction(m_findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(m_findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    m_openTypeHierarchyAction = new QAction(tr("Open Type Hierarchy"), this);
    cmd = Core::ActionManager::registerAction(m_openTypeHierarchyAction, Constants::OPEN_TYPE_HIERARCHY, context);
    cmd->setDefaultKeySequence(QKeySequence(Core::UseMacShortcuts ? tr("Meta+Shift+T") : tr("Ctrl+Shift+T")));
    connect(m_openTypeHierarchyAction, SIGNAL(triggered()), this, SLOT(openTypeHierarchy()));
    contextMenu->addAction(cmd);
    cppToolsMenu->addAction(cmd);

    // Refactoring sub-menu
    Core::Context globalContext(Core::Constants::C_GLOBAL);
    Core::Command *sep = contextMenu->addSeparator(globalContext);
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator(globalContext);

    m_renameSymbolUnderCursorAction = new QAction(tr("Rename Symbol Under Cursor"),
                                                  this);
    cmd = Core::ActionManager::registerAction(m_renameSymbolUnderCursorAction,
                             Constants::RENAME_SYMBOL_UNDER_CURSOR,
                             context);
    cmd->setDefaultKeySequence(QKeySequence(tr("CTRL+SHIFT+R")));
    connect(m_renameSymbolUnderCursorAction, SIGNAL(triggered()),
            this, SLOT(renameSymbolUnderCursor()));
    cppToolsMenu->addAction(cmd);

    // Update context in global context
    cppToolsMenu->addSeparator(globalContext);
    m_updateCodeModelAction = new QAction(tr("Update Code Model"), this);
    cmd = Core::ActionManager::registerAction(m_updateCodeModelAction, Core::Id(Constants::UPDATE_CODEMODEL), globalContext);
    CPlusPlus::CppModelManagerInterface *cppModelManager = CPlusPlus::CppModelManagerInterface::instance();
    connect(m_updateCodeModelAction, SIGNAL(triggered()), cppModelManager, SLOT(updateModifiedSourceFiles()));
    cppToolsMenu->addAction(cmd);

    m_actionHandler = new TextEditor::TextEditorActionHandler(CppEditor::Constants::C_CPPEDITOR,
        TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll
        | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor);

    m_actionHandler->initializeActions();

    contextMenu->addSeparator(context);

    cmd = Core::ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    connect(Core::ICore::progressManager(), SIGNAL(taskStarted(QString)),
            this, SLOT(onTaskStarted(QString)));
    connect(Core::ICore::progressManager(), SIGNAL(allTasksFinished(QString)),
            this, SLOT(onAllTasksFinished(QString)));

    connect(Core::ICore::editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currentEditorChanged(Core::IEditor*)));

    readSettings();
    return true;
}

void CppPlugin::readSettings()
{
    m_sortedOutline = Core::ICore::settings()->value(QLatin1String("CppTools/SortedMethodOverview"), false).toBool();
}

void CppPlugin::writeSettings()
{
    Core::ICore::settings()->setValue(QLatin1String("CppTools/SortedMethodOverview"), m_sortedOutline);
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
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(Core::EditorManager::currentEditor()->widget());
    if (editor)
        editor->switchDeclarationDefinition();
}

void CppPlugin::renameSymbolUnderCursor()
{
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(Core::EditorManager::currentEditor()->widget());
    if (editor)
        editor->renameSymbolUnderCursor();
}

void CppPlugin::findUsages()
{
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(Core::EditorManager::currentEditor()->widget());
    if (editor)
        editor->findUsages();
}

void CppPlugin::onTaskStarted(const QString &type)
{
    if (type == QLatin1String(CppTools::Constants::TASK_INDEX)) {
        m_renameSymbolUnderCursorAction->setEnabled(false);
        m_findUsagesAction->setEnabled(false);
        m_updateCodeModelAction->setEnabled(false);
        m_openTypeHierarchyAction->setEnabled(false);
    }
}

void CppPlugin::onAllTasksFinished(const QString &type)
{
    if (type == QLatin1String(CppTools::Constants::TASK_INDEX)) {
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
        textEditor->semanticRehighlight(/*force = */ true);
    }
}

void CppPlugin::openTypeHierarchy()
{
    CPPEditorWidget *editor = qobject_cast<CPPEditorWidget*>(Core::EditorManager::currentEditor()->widget());
    if (editor) {
        Core::NavigationWidget *navigation = Core::NavigationWidget::instance();
        navigation->activateSubWidget(Core::Id(Constants::TYPE_HIERARCHY_ID));
        emit typeHierarchyRequested();
    }
}

Q_EXPORT_PLUGIN(CppPlugin)
