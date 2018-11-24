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

#include "qmljseditingsettingspage.h"
#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"
#include "qmljseditorplugin.h"
#include "qmljshighlighter.h"
#include "qmljsoutline.h"
#include "qmljsquickfixassist.h"
#include "qmltaskmanager.h"
#include "quicktoolbar.h"

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
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/tabsettings.h>
#include <utils/qtcassert.h>
#include <utils/json.h>

#include <QTextDocument>
#include <QMenu>
#include <QAction>

using namespace QmlJSEditor::Constants;
using namespace ProjectExplorer;
using namespace Core;

namespace QmlJSEditor {
namespace Internal {

class QmlJSEditorPluginPrivate : public QObject
{
public:
    QmlJSEditorPluginPrivate();

    void currentEditorChanged(IEditor *editor);
    void runSemanticScan();
    void checkCurrentEditorSemanticInfoUpToDate();
    void autoFormatOnSave(IDocument *document);

    Command *addToolAction(QAction *a, Context &context, Id id,
                           ActionContainer *c1, const QString &keySequence);

    void renameUsages();
    void reformatFile();
    void showContextPane();

    QmlJSQuickFixAssistProvider m_quickFixAssistProvider;
    QmlTaskManager m_qmlTaskManager;

    QAction *m_reformatFileAction = nullptr;

    QPointer<QmlJSEditorDocument> m_currentDocument;

    Utils::JsonSchemaManager m_jsonManager{{ICore::userResourcePath() + "/json/",
                                            ICore::resourcePath() + "/json/"}};
    QmlJSEditorFactory m_qmlJSEditorFactory;
    QmlJSOutlineWidgetFactory m_qmlJSOutlineWidgetFactory;
    QuickToolBar m_quickToolBar;
    QmlJsEditingSettingsPage m_qmJSEditingSettingsPage;
};

static QmlJSEditorPlugin *m_instance = nullptr;

QmlJSEditorPlugin::QmlJSEditorPlugin()
{
    m_instance = this;
}

QmlJSEditorPlugin::~QmlJSEditorPlugin()
{
    delete d;
    d = nullptr;
    m_instance = nullptr;
}

bool QmlJSEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);

    d = new QmlJSEditorPluginPrivate;

    return true;
}

QmlJSEditorPluginPrivate::QmlJSEditorPluginPrivate()
{
    TextEditor::SnippetProvider::registerGroup(Constants::QML_SNIPPETS_GROUP_ID,
                                               QmlJSEditorPlugin::tr("QML", "SnippetProvider"),
                                               &QmlJSEditorFactory::decorateEditor);

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();

    // QML task updating manager
    connect(modelManager, &QmlJS::ModelManagerInterface::documentChangedOnDisk,
            &m_qmlTaskManager, &QmlTaskManager::updateMessages);
    // recompute messages when information about libraries changes
    connect(modelManager, &QmlJS::ModelManagerInterface::libraryInfoUpdated,
            &m_qmlTaskManager, &QmlTaskManager::updateMessages);
    // recompute messages when project data changes (files added or removed)
    connect(modelManager, &QmlJS::ModelManagerInterface::projectInfoUpdated,
            &m_qmlTaskManager, &QmlTaskManager::updateMessages);
    connect(modelManager, &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            &m_qmlTaskManager, &QmlTaskManager::documentsRemoved);

    Context context(Constants::C_QMLJSEDITOR_ID);

    ActionContainer *contextMenu = ActionManager::createMenu(Constants::M_CONTEXT);
    ActionContainer *qmlToolsMenu = ActionManager::actionContainer(Id(QmlJSTools::Constants::M_TOOLS_QMLJS));

    qmlToolsMenu->addSeparator();

    Command *cmd;
    cmd = ActionManager::command(TextEditor::Constants::FOLLOW_SYMBOL_UNDER_CURSOR);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::FIND_USAGES);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *renameUsagesAction = new QAction(QmlJSEditorPlugin::tr("Rename Symbol Under Cursor"), this);
    cmd = ActionManager::registerAction(renameUsagesAction, Constants::RENAME_USAGES, context);
    cmd->setDefaultKeySequence(QKeySequence(QmlJSEditorPlugin::tr("Ctrl+Shift+R")));
    connect(renameUsagesAction, &QAction::triggered, this, &QmlJSEditorPluginPrivate::renameUsages);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *semanticScan = new QAction(QmlJSEditorPlugin::tr("Run Checks"), this);
    cmd = ActionManager::registerAction(semanticScan, Id(Constants::RUN_SEMANTIC_SCAN));
    cmd->setDefaultKeySequence(QKeySequence(QmlJSEditorPlugin::tr("Ctrl+Shift+C")));
    connect(semanticScan, &QAction::triggered, this, &QmlJSEditorPluginPrivate::runSemanticScan);
    qmlToolsMenu->addAction(cmd);

    m_reformatFileAction = new QAction(QmlJSEditorPlugin::tr("Reformat File"), this);
    cmd = ActionManager::registerAction(m_reformatFileAction, Id(Constants::REFORMAT_FILE), context);
    connect(m_reformatFileAction, &QAction::triggered, this, &QmlJSEditorPluginPrivate::reformatFile);
    qmlToolsMenu->addAction(cmd);

    QAction *inspectElementAction = new QAction(QmlJSEditorPlugin::tr("Inspect API for Element Under Cursor"), this);
    cmd = ActionManager::registerAction(inspectElementAction,
                                              Id(Constants::INSPECT_ELEMENT_UNDER_CURSOR), context);
    connect(inspectElementAction, &QAction::triggered, [] {
        if (auto widget = qobject_cast<QmlJSEditorWidget *>(EditorManager::currentEditor()->widget()))
            widget->inspectElementUnderCursor();
    });
    qmlToolsMenu->addAction(cmd);

    QAction *showQuickToolbar = new QAction(QmlJSEditorPlugin::tr("Show Qt Quick Toolbar"), this);
    cmd = ActionManager::registerAction(showQuickToolbar, Constants::SHOW_QT_QUICK_HELPER, context);
    cmd->setDefaultKeySequence(useMacShortcuts ? QKeySequence(Qt::META + Qt::ALT + Qt::Key_Space)
                                                     : QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_Space));
    connect(showQuickToolbar, &QAction::triggered, this, &QmlJSEditorPluginPrivate::showContextPane);
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

    FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QML, "qml");

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmlJSEditorPluginPrivate::currentEditorChanged);

    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, &QmlJSEditorPluginPrivate::autoFormatOnSave);
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

Utils::JsonSchemaManager *QmlJSEditorPlugin::jsonManager()
{
    return &m_instance->d->m_jsonManager;
}

QuickToolBar *QmlJSEditorPlugin::quickToolBar()
{
    QTC_ASSERT(m_instance && m_instance->d, return new QuickToolBar());
    return &m_instance->d->m_quickToolBar;
}

void QmlJSEditorPluginPrivate::renameUsages()
{
    if (auto editor = qobject_cast<QmlJSEditorWidget*>(EditorManager::currentEditor()->widget()))
        editor->renameUsages();
}

void QmlJSEditorPluginPrivate::reformatFile()
{
    if (m_currentDocument) {
        QmlJS::Document::Ptr document = m_currentDocument->semanticInfo().document;
        QmlJS::Snapshot snapshot = QmlJS::ModelManagerInterface::instance()->snapshot();

        if (m_currentDocument->isSemanticInfoOutdated()) {
            QmlJS::Document::MutablePtr latestDocument;

            const QString fileName = m_currentDocument->filePath().toString();
            latestDocument = snapshot.documentFromSource(QString::fromUtf8(m_currentDocument->contents()),
                                                         fileName,
                                                         QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName));
            latestDocument->parseQml();
            snapshot.insert(latestDocument);
            document = latestDocument;
        }

        if (!document->isParsedCorrectly())
            return;

        TextEditor::TabSettings tabSettings = m_currentDocument->tabSettings();
        const QString &newText = QmlJS::reformat(document,
                                                 tabSettings.m_indentSize,
                                                 tabSettings.m_tabSize);

        //  QTextDocument::setPlainText cannot be used, as it would reset undo/redo history
        const auto setNewText = [this, &newText]() {
            QTextCursor tc(m_currentDocument->document());
            tc.movePosition(QTextCursor::Start);
            tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            tc.insertText(newText);
        };

        IEditor *ed = EditorManager::currentEditor();
        if (ed) {
            int line = ed->currentLine();
            int column = ed->currentColumn();
            setNewText();
            ed->gotoLine(line, column);
        } else {
            setNewText();
        }
    }
}

void QmlJSEditorPluginPrivate::showContextPane()
{
    if (auto editor = qobject_cast<QmlJSEditorWidget*>(EditorManager::currentEditor()->widget()))
        editor->showContextPane();
}

Command *QmlJSEditorPluginPrivate::addToolAction(QAction *a,
                                                 Context &context, Id id,
                                                 ActionContainer *c1, const QString &keySequence)
{
    Command *command = ActionManager::registerAction(a, id, context);
    if (!keySequence.isEmpty())
        command->setDefaultKeySequence(QKeySequence(keySequence));
    c1->addAction(command);
    return command;
}

QmlJSQuickFixAssistProvider *QmlJSEditorPlugin::quickFixAssistProvider()
{
    return &m_instance->d->m_quickFixAssistProvider;
}

void QmlJSEditorPluginPrivate::currentEditorChanged(IEditor *editor)
{
    QmlJSEditorDocument *document = nullptr;
    if (editor)
        document = qobject_cast<QmlJSEditorDocument *>(editor->document());

    if (m_currentDocument)
        m_currentDocument->disconnect(this);
    m_currentDocument = document;
    if (document) {
        connect(document->document(), &QTextDocument::contentsChanged,
                this, &QmlJSEditorPluginPrivate::checkCurrentEditorSemanticInfoUpToDate);
        connect(document, &QmlJSEditorDocument::semanticInfoUpdated,
                this, &QmlJSEditorPluginPrivate::checkCurrentEditorSemanticInfoUpToDate);
    }
}

void QmlJSEditorPluginPrivate::runSemanticScan()
{
    m_qmlTaskManager.updateSemanticMessagesNow();
    TaskHub::setCategoryVisibility(Constants::TASK_CATEGORY_QML_ANALYSIS, true);
    TaskHub::requestPopup();
}

void QmlJSEditorPluginPrivate::checkCurrentEditorSemanticInfoUpToDate()
{
    const bool semanticInfoUpToDate = m_currentDocument && !m_currentDocument->isSemanticInfoOutdated();
    m_reformatFileAction->setEnabled(semanticInfoUpToDate);
}

void QmlJSEditorPluginPrivate::autoFormatOnSave(IDocument *document)
{
    if (!QmlJsEditingSettings::get().autoFormatOnSave())
        return;

    // Check that we are dealing with a QML/JS editor
    if (document->id() != Constants::C_QMLJSEDITOR_ID)
        return;

    // Check if file is contained in the current project (if wished)
    if (QmlJsEditingSettings::get().autoFormatOnlyCurrentProject()) {
        const Project *pro = ProjectTree::currentProject();
        if (!pro || !pro->files(Project::SourceFiles).contains(document->filePath()))
            return;
    }

    reformatFile();
}

} // namespace Internal
} // namespace QmlJSEditor
