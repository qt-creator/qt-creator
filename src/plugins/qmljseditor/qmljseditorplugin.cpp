/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "qmljseditordocument.h"
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

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/taskhub.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/textfilewizard.h>
#include <texteditor/highlighterfactory.h>
#include <utils/qtcassert.h>
#include <utils/json.h>

#include <QtPlugin>
#include <QSettings>
#include <QDir>
#include <QCoreApplication>
#include <QTextDocument>
#include <QTimer>
#include <QMenu>
#include <QAction>

using namespace QmlJSEditor::Constants;
using namespace ProjectExplorer;
using namespace Core;

enum {
    QUICKFIX_INTERVAL = 20
};

namespace QmlJSEditor {
using namespace Internal;

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);

QmlJSEditorPlugin *QmlJSEditorPlugin::m_instance = 0;

QmlJSEditorPlugin::QmlJSEditorPlugin() :
        m_modelManager(0),
    m_quickFixAssistProvider(0),
    m_reformatFileAction(0),
    m_currentDocument(0),
    m_jsonManager(new Utils::JsonSchemaManager(
            QStringList() << Core::ICore::userResourcePath() + QLatin1String("/json/")
                          << Core::ICore::resourcePath() + QLatin1String("/json/")))
{
    m_instance = this;
}

QmlJSEditorPlugin::~QmlJSEditorPlugin()
{
    m_instance = 0;
}

bool QmlJSEditorPlugin::initialize(const QStringList & /*arguments*/, QString *errorMessage)
{
    m_modelManager = QmlJS::ModelManagerInterface::instance();
    addAutoReleasedObject(new QmlJSSnippetProvider);

    auto hf = new TextEditor::HighlighterFactory;
    hf->setProductType<Highlighter>();
    hf->setId(Constants::C_QMLJSEDITOR_ID);
    hf->addMimeType(QmlJSTools::Constants::QML_MIMETYPE);
    hf->addMimeType(QmlJSTools::Constants::QMLPROJECT_MIMETYPE);
    hf->addMimeType(QmlJSTools::Constants::QBS_MIMETYPE);
    hf->addMimeType(QmlJSTools::Constants::QMLTYPES_MIMETYPE);
    hf->addMimeType(QmlJSTools::Constants::JS_MIMETYPE);
    hf->addMimeType(QmlJSTools::Constants::JSON_MIMETYPE);
    addAutoReleasedObject(hf);

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

    Core::Context context(Constants::C_QMLJSEDITOR_ID);

    addAutoReleasedObject(new QmlJSEditorFactory);

    IWizardFactory *wizard = new QmlFileWizard;
    wizard->setWizardKind(Core::IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizard->setDescription(tr("Creates a QML file with boilerplate code, starting with \"import QtQuick 1.1\"."));
    wizard->setDisplayName(tr("QML File (Qt Quick 1)"));
    wizard->setId(QLatin1String(Constants::WIZARD_QML1FILE));
    addAutoReleasedObject(wizard);

    wizard = new QmlFileWizard;
    wizard->setWizardKind(Core::IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizard->setDescription(tr("Creates a QML file with boilerplate code, starting with \"import QtQuick 2.0\"."));
    wizard->setDisplayName(tr("QML File (Qt Quick 2)"));
    wizard->setId(QLatin1String(Constants::WIZARD_QML2FILE));
    addAutoReleasedObject(wizard);

    wizard = new JsFileWizard;
    wizard->setWizardKind(Core::IWizardFactory::FileWizard);
    wizard->setCategory(QLatin1String(Core::Constants::WIZARD_CATEGORY_QT));
    wizard->setDisplayCategory(QCoreApplication::translate("QmlJsEditor", Core::Constants::WIZARD_TR_CATEGORY_QT));
    wizard->setDescription(tr("Creates a JavaScript file."));
    wizard->setDisplayName(tr("JS File"));
    wizard->setId(QLatin1String("Z.Js"));
    addAutoReleasedObject(wizard);

    Core::ActionContainer *contextMenu = Core::ActionManager::createMenu(Constants::M_CONTEXT);
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

    Core::FileIconProvider::registerIconOverlayForSuffix(":/qmljseditor/images/qmlfile.png", "qml");

    registerQuickFixes(this);

    addAutoReleasedObject(new QmlJSOutlineWidgetFactory);

    addAutoReleasedObject(new QuickToolBar);
    addAutoReleasedObject(new Internal::QuickToolBarSettingsPage);

    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)), SLOT(currentEditorChanged(Core::IEditor*)));

    return true;
}

void QmlJSEditorPlugin::extensionsInitialized()
{
    TaskHub::addCategory(Constants::TASK_CATEGORY_QML, tr("QML"));
    TaskHub::addCategory(Constants::TASK_CATEGORY_QML_ANALYSIS, tr("QML Analysis"), false);
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSEditorPlugin::aboutToShutdown()
{
    delete QmlJS::Icons::instance(); // delete object held by singleton

    return IPlugin::aboutToShutdown();
}

Utils::JsonSchemaManager *QmlJSEditorPlugin::jsonManager() const
{
    return m_jsonManager.data();
}

void QmlJSEditorPlugin::findUsages()
{
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->findUsages();
}

void QmlJSEditorPlugin::renameUsages()
{
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->renameUsages();
}

void QmlJSEditorPlugin::reformatFile()
{
    if (m_currentDocument) {
        QTC_ASSERT(!m_currentDocument->isSemanticInfoOutdated(), return);

        const QString &newText = QmlJS::reformat(m_currentDocument->semanticInfo().document);
        QTextCursor tc(m_currentDocument->document());
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        tc.insertText(newText);
    }
}

void QmlJSEditorPlugin::showContextPane()
{
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(Core::EditorManager::currentEditor()->widget()))
        editor->showContextPane();
}

Core::Command *QmlJSEditorPlugin::addToolAction(QAction *a,
                                          Core::Context &context, Core::Id id,
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
    QmlJSEditorDocument *document = 0;
    if (editor)
        document = qobject_cast<QmlJSEditorDocument *>(editor->document());

    if (m_currentDocument)
        m_currentDocument->disconnect(this);
    m_currentDocument = document;
    if (document) {
        connect(document->document(), SIGNAL(contentsChanged()),
                this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
        connect(document, SIGNAL(semanticInfoUpdated(QmlJSTools::SemanticInfo)),
                this, SLOT(checkCurrentEditorSemanticInfoUpToDate()));
    }
}

void QmlJSEditorPlugin::runSemanticScan()
{
    m_qmlTaskManager->updateSemanticMessagesNow();
    TaskHub::setCategoryVisibility(Constants::TASK_CATEGORY_QML_ANALYSIS, true);
    TaskHub::requestPopup();
}

void QmlJSEditorPlugin::checkCurrentEditorSemanticInfoUpToDate()
{
    const bool semanticInfoUpToDate = m_currentDocument && !m_currentDocument->isSemanticInfoOutdated();
    m_reformatFileAction->setEnabled(semanticInfoUpToDate);
}

} // namespace QmlJSEditor

Q_EXPORT_PLUGIN(QmlJSEditor::Internal::QmlJSEditorPlugin)
