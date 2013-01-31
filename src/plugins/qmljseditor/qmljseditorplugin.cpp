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

#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditorfactory.h"
#include "qmljshoverhandler.h"
#include "qmlfilewizard.h"
#include "jsfilewizard.h"
#include "qmljsoutline.h"
#include "qmljspreviewrunner.h"
#include "qmljssnippetprovider.h"
#include "qmltaskmanager.h"
#include "quicktoolbar.h"
#include "quicktoolbarsettingspage.h"
#include "qmljscompletionassist.h"
#include "qmljsquickfixassist.h"

#include <qmljs/qmljsicons.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <qmldesigner/qmldesignerconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/id.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/texteditoractionhandler.h>
#include <utils/qtcassert.h>
#include <utils/json.h>

#include <QtPlugin>
#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QTimer>
#include <QMenu>
#include <QAction>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;
using namespace ProjectExplorer;

enum {
    QUICKFIX_INTERVAL = 20
};

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);

QmlJSEditorPlugin *QmlJSEditorPlugin::m_instance = 0;

QmlJSEditorPlugin::QmlJSEditorPlugin() :
        m_modelManager(0),
    m_editor(0),
    m_actionHandler(0),
    m_quickFixAssistProvider(0),
    m_reformatFileAction(0),
    m_currentEditor(0),
    m_jsonManager(new Utils::JsonSchemaManager(
            QStringList() << Core::ICore::instance()->userResourcePath() + QLatin1String("/json/")
                          << Core::ICore::instance()->resourcePath() + QLatin1String("/json/")))
{
    m_instance = this;
}

QmlJSEditorPlugin::~QmlJSEditorPlugin()
{
    removeObject(m_editor);
    delete m_actionHandler;
    m_instance = 0;
}

bool QmlJSEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    if (!Core::ICore::mimeDatabase()->addMimeTypes(QLatin1String(":/qmljseditor/QmlJSEditor.mimetypes.xml"), errorMessage))
        return false;

    m_modelManager = QmlJS::ModelManagerInterface::instance();
    addAutoReleasedObject(new QmlJSSnippetProvider);

    // QML task updating manager
    m_qmlTaskManager = new QmlTaskManager;
    addAutoReleasedObject(m_qmlTaskManager);
    connect(m_modelManager, SIGNAL(documentChangedOnDisk(QmlJS::Document::Ptr)),
            m_qmlTaskManager, SLOT(updateMessages()));
    // recompute messages when information about libraries changes
    connect(m_modelManager, SIGNAL(libraryInfoUpdated(QString,QmlJS::LibraryInfo)),
            m_qmlTaskManager, SLOT(updateMessages()));
    // recompute messages when project data changes (files added or removed)
    connect(m_modelManager, SIGNAL(projectInfoUpdated(ProjectInfo)),
            m_qmlTaskManager, SLOT(updateMessages()));
    connect(m_modelManager, SIGNAL(aboutToRemoveFiles(QStringList)),
            m_qmlTaskManager, SLOT(documentsRemoved(QStringList)));

    Core::Context context(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);

    m_editor = new QmlJSEditorFactory(this);
    addObject(m_editor);

    QObject *core = Core::ICore::instance();
    Core::BaseFileWizardParameters qml1WizardParameters(Core::IWizard::FileWizard);
    qml1WizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    qml1WizardParameters.setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    qml1WizardParameters.setDescription(tr("Creates a QML file with boilerplate code, starting with \"import QtQuick 1.1\"."));
    qml1WizardParameters.setDisplayName(tr("QML File (Qt Quick 1)"));
    qml1WizardParameters.setId(QLatin1String(Constants::WIZARD_QML1FILE));
    addAutoReleasedObject(new QmlFileWizard(qml1WizardParameters, core));

    Core::BaseFileWizardParameters qml2WizardParameters(Core::IWizard::FileWizard);
    qml2WizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    qml2WizardParameters.setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    qml2WizardParameters.setDescription(tr("Creates a QML file with boilerplate code, starting with \"import QtQuick 2.0\"."));
    qml2WizardParameters.setDisplayName(tr("QML File (Qt Quick 2)"));
    qml2WizardParameters.setId(QLatin1String(Constants::WIZARD_QML2FILE));
    addAutoReleasedObject(new QmlFileWizard(qml2WizardParameters, core));

    Core::BaseFileWizardParameters jsWizardParameters(Core::IWizard::FileWizard);
    jsWizardParameters.setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    jsWizardParameters.setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    jsWizardParameters.setDescription(tr("Creates a JavaScript file."));
    jsWizardParameters.setDisplayName(tr("JS File"));
    jsWizardParameters.setId(QLatin1String("Z.Js"));
    addAutoReleasedObject(new JsFileWizard(jsWizardParameters, core));

    m_actionHandler = new TextEditor::TextEditorActionHandler(QmlJSEditor::Constants::C_QMLJSEDITOR_ID,
          TextEditor::TextEditorActionHandler::Format
        | TextEditor::TextEditorActionHandler::UnCommentSelection
        | TextEditor::TextEditorActionHandler::UnCollapseAll
        | TextEditor::TextEditorActionHandler::FollowSymbolUnderCursor);
    m_actionHandler->initializeActions();

    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(QmlJSEditor::Constants::M_CONTEXT);
    Core::ActionContainer *qmlToolsMenu = Core::ActionManager::actionContainer(Core::Id(QmlJSTools::Constants::M_TOOLS_QMLJS));

    Core::Context globalContext(Core::Constants::C_GLOBAL);
    qmlToolsMenu->addSeparator(globalContext);

    Core::Command *cmd;
    cmd = Core::ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = Core::ActionManager::registerAction(findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *renameUsagesAction = new QAction(tr("Rename Symbol Under Cursor"), this);
    cmd = Core::ActionManager::registerAction(renameUsagesAction, Constants::RENAME_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+R")));
    connect(renameUsagesAction, SIGNAL(triggered()), this, SLOT(renameUsages()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *semanticScan = new QAction(tr("Run Checks"), this);
    cmd = Core::ActionManager::registerAction(semanticScan, Core::Id(Constants::RUN_SEMANTIC_SCAN), globalContext);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+C")));
    connect(semanticScan, SIGNAL(triggered()), this, SLOT(runSemanticScan()));
    qmlToolsMenu->addAction(cmd);

    m_reformatFileAction = new QAction(tr("Reformat File"), this);
    cmd = Core::ActionManager::registerAction(m_reformatFileAction, Core::Id(Constants::REFORMAT_FILE), context);
    connect(m_reformatFileAction, SIGNAL(triggered()), this, SLOT(reformatFile()));
    qmlToolsMenu->addAction(cmd);

    QAction *showQuickToolbar = new QAction(tr("Show Qt Quick Toolbar"), this);
    cmd = Core::ActionManager::registerAction(showQuickToolbar, Constants::SHOW_QT_QUICK_HELPER, context);
    cmd->setDefaultKeySequence(Core::UseMacShortcuts ? QKeySequence(Qt::META + Qt::ALT + Qt::Key_Space)
                                                     : QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Space));
    connect(showQuickToolbar, SIGNAL(triggered()), this, SLOT(showContextPane()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    // Insert marker for "Refactoring" menu:
    Core::Command *sep = contextMenu->addSeparator(globalContext);
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator(globalContext);

    cmd = Core::ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = Core::ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    m_quickFixAssistProvider = new QmlJSQuickFixAssistProvider;
    addAutoReleasedObject(m_quickFixAssistProvider);
    addAutoReleasedObject(new QmlJSCompletionAssistProvider);

    addAutoReleasedObject(new HoverHandler);

    errorMessage->clear();

    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/qmljseditor/images/qmlfile.png")), QLatin1String("qml"));

    registerQuickFixes(this);

    addAutoReleasedObject(new QmlJSOutlineWidgetFactory);

    addAutoReleasedObject(new QuickToolBar);
    addAutoReleasedObject(new Internal::QuickToolBarSettingsPage);

    connect(Core::ICore::editorManager(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currentEditorChanged(Core::IEditor*)));

    return true;
}

void QmlJSEditorPlugin::extensionsInitialized()
{
    TaskHub *taskHub = ProjectExplorerPlugin::instance()->taskHub();
    taskHub->addCategory(Constants::TASK_CATEGORY_QML, tr("QML"));
    taskHub->addCategory(Constants::TASK_CATEGORY_QML_ANALYSIS, tr("QML Analysis"), false);
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSEditorPlugin::aboutToShutdown()
{
    delete QmlJS::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

void QmlJSEditorPlugin::initializeEditor(QmlJSEditor::QmlJSTextEditorWidget *editor)
{
    QTC_CHECK(m_instance);

    m_actionHandler->setupActions(editor);

    editor->setLanguageSettingsId(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);
}

Utils::JsonSchemaManager *QmlJSEditorPlugin::jsonManager() const
{
    return m_jsonManager.data();
}

void QmlJSEditorPlugin::findUsages()
{
    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->findUsages();
}

void QmlJSEditorPlugin::renameUsages()
{
    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->renameUsages();
}

void QmlJSEditorPlugin::reformatFile()
{
    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(Core::EditorManager::currentEditor()->widget())) {
        QTC_ASSERT(!editor->isSemanticInfoOutdated(), return);

        const QString &newText = QmlJS::reformat(editor->semanticInfo().document);
        QTextCursor tc(editor->textCursor());
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        tc.insertText(newText);
    }
}

void QmlJSEditorPlugin::showContextPane()
{
    if (QmlJSTextEditorWidget *editor = qobject_cast<QmlJSTextEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->showContextPane();

}

Core::Command *QmlJSEditorPlugin::addToolAction(QAction *a,
                                          Core::Context &context, const Core::Id &id,
                                          Core::ActionContainer *c1, const QString &keySequence)
{
    Core::Command *command = Core::ActionManager::registerAction(a, id, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QmlJSQuickFixAssistProvider *QmlJSEditorPlugin::quickFixAssistProvider() const
{
    return m_quickFixAssistProvider;
}

void QmlJSEditorPlugin::currentEditorChanged(Core::IEditor *editor)
{
    QmlJSTextEditorWidget *newTextEditor = 0;
    if (editor)
        newTextEditor = qobject_cast<QmlJSTextEditorWidget *>(editor->widget());

    if (m_currentEditor) {
        disconnect(m_currentEditor.data(), SIGNAL(contentsChanged()),
                   this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
        disconnect(m_currentEditor.data(), SIGNAL(semanticInfoUpdated()),
                   this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
    }
    m_currentEditor = newTextEditor;
    if (newTextEditor) {
        connect(newTextEditor, SIGNAL(contentsChanged()),
                this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
        connect(newTextEditor, SIGNAL(semanticInfoUpdated()),
                this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
        newTextEditor->reparseDocumentNow();
    }
}

void QmlJSEditorPlugin::runSemanticScan()
{
    m_qmlTaskManager->updateSemanticMessagesNow();
    TaskHub *hub = ProjectExplorerPlugin::instance()->taskHub();
    hub->setCategoryVisibility(Constants::TASK_CATEGORY_QML_ANALYSIS, true);
    hub->requestPopup();
}

void QmlJSEditorPlugin::checkCurrentEditorSemanticInfoUpToDate()
{
    const bool semanticInfoUpToDate = m_currentEditor && !m_currentEditor->isSemanticInfoOutdated();
    m_reformatFileAction->setEnabled(semanticInfoUpToDate);
}

Q_EXPORT_PLUGIN(QmlJSEditorPlugin)
