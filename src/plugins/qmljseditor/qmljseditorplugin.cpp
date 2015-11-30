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

#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"
#include "qmljsoutline.h"
#include "qmljspreviewrunner.h"
#include "qmljssnippetprovider.h"
#include "qmltaskmanager.h"
#include "quicktoolbar.h"
#include "quicktoolbarsettingspage.h"
#include "qmljsquickfixassist.h"

#include <qmljs/qmljsicons.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>
#include <qmljstools/qmljstoolsconstants.h>

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
            QStringList() << ICore::userResourcePath() + QLatin1String("/json/")
                          << ICore::resourcePath() + QLatin1String("/json/")))
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

    Context context(Constants::C_QMLJSEDITOR_ID);

    addAutoReleasedObject(new QmlJSEditorFactory);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    ActionContainer *qmlToolsMenu = ActionManager::actionContainer(Id(QmlJSTools::Constants::M_TOOLS_QMLJS));

    qmlToolsMenu->addSeparator();

    Command *cmd;
    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *findUsagesAction = new QAction(tr("Find Usages"), this);
    cmd = ActionManager::registerAction(findUsagesAction, Constants::FIND_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+U")));
    connect(findUsagesAction, SIGNAL(triggered()), this, SLOT(findUsages()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *renameUsagesAction = new QAction(tr("Rename Symbol Under Cursor"), this);
    cmd = ActionManager::registerAction(renameUsagesAction, Constants::RENAME_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+R")));
    connect(renameUsagesAction, SIGNAL(triggered()), this, SLOT(renameUsages()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *semanticScan = new QAction(tr("Run Checks"), this);
    cmd = ActionManager::registerAction(semanticScan, Id(Constants::RUN_SEMANTIC_SCAN));
    cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+C")));
    connect(semanticScan, SIGNAL(triggered()), this, SLOT(runSemanticScan()));
    qmlToolsMenu->addAction(cmd);

    m_reformatFileAction = new QAction(tr("Reformat File"), this);
    cmd = ActionManager::registerAction(m_reformatFileAction, Id(Constants::REFORMAT_FILE), context);
    connect(m_reformatFileAction, SIGNAL(triggered()), this, SLOT(reformatFile()));
    qmlToolsMenu->addAction(cmd);

    QAction *inspectElementAction = new QAction(tr("Inspect API for Element Under Cursor"), this);
    cmd = ActionManager::registerAction(inspectElementAction,
                                              Id(Constants::INSPECT_ELEMENT_UNDER_CURSOR), context);
    connect(inspectElementAction, &QAction::triggered, [] {
        if (auto widget = qobject_cast<QmlJSEditorWidget *>(EditorManager::currentEditor()->widget()))
            widget->inspectElementUnderCursor();
    });
    qmlToolsMenu->addAction(cmd);

    QAction *showQuickToolbar = new QAction(tr("Show Qt Quick Toolbar"), this);
    cmd = ActionManager::registerAction(showQuickToolbar, Constants::SHOW_QT_QUICK_HELPER, context);
    cmd->setDefaultKeySequence(UseMacShortcuts ? QKeySequence(Qt::META + Qt::ALT + Qt::Key_Space)
                                                     : QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Space));
    connect(showQuickToolbar, SIGNAL(triggered()), this, SLOT(showContextPane()));
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    // Insert marker for "Refactoring" menu:
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator();

    cmd = ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    m_quickFixAssistProvider = new QmlJSQuickFixAssistProvider;
    addAutoReleasedObject(m_quickFixAssistProvider);

    errorMessage->clear();

    FileIconProvider::registerIconOverlayForSuffix(":/qmljseditor/images/qmlfile.png", "qml");

    registerQuickFixes(this);

    addAutoReleasedObject(new QmlJSOutlineWidgetFactory);

    addAutoReleasedObject(new QuickToolBar);
    addAutoReleasedObject(new QuickToolBarSettingsPage);

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
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(EditorManager::currentEditor()->widget()))
        editor->findUsages();
}

void QmlJSEditorPlugin::renameUsages()
{
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(EditorManager::currentEditor()->widget()))
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
    if (QmlJSEditorWidget *editor = qobject_cast<QmlJSEditorWidget*>(EditorManager::currentEditor()->widget()))
        editor->showContextPane();
}

Command *QmlJSEditorPlugin::addToolAction(QAction *a,
                                          Context &context, Id id,
                                          ActionContainer *c1, const QString &keySequence)
{
    Command *command = ActionManager::registerAction(a, id, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QmlJSQuickFixAssistProvider *QmlJSEditorPlugin::quickFixAssistProvider() const
{
    return m_quickFixAssistProvider;
}

void QmlJSEditorPlugin::currentEditorChanged(IEditor *editor)
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
