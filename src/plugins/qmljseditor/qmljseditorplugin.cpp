// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljseditorplugin.h"

#include "qmljseditorconstants.h"
#include "qmljseditordocument.h"
#include "qmljseditorsettings.h"
#include "qmljseditortr.h"
#include "qmljsoutline.h"
#include "qmljsquickfixassist.h"
#include "qmllsclientsettings.h"
#include "qmltaskmanager.h"

#include <qmljs/jsoncheck.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>

#include <qmljstools/qmlformatsettings.h>
#include <qmljstools/qmljscodestylesettings.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qmljstools/qmljstoolssettings.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/iplugin.h>

#include <languageclient/languageclientmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/taskhub.h>

#include <texteditor/command.h>
#include <texteditor/formattexteditor.h>
#include <texteditor/snippets/snippetprovider.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/filepath.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/macroexpander.h>
#include <utils/mimeconstants.h>
#include <utils/qtcassert.h>

#include <QTextDocument>
#include <QMenu>
#include <QAction>

using namespace ProjectExplorer;
using namespace Core;
using namespace Utils;
using namespace QmlJSTools;

namespace QmlJSEditor::Internal {

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

    FormatResult reformatFile();

    QmlJS::JsonSchemaManager *jsonManager() { return &m_jsonManager;}
    QmlJSQuickFixAssistProvider *quickFixAssistProvider() { return &m_quickFixAssistProvider; }

private:
    QmlJSQuickFixAssistProvider m_quickFixAssistProvider;
    QmlTaskManager m_qmlTaskManager;

    QAction *m_reformatFileAction = nullptr;

    QPointer<QmlJSEditorDocument> m_currentDocument;

    QmlJS::JsonSchemaManager m_jsonManager{{ICore::userResourcePath("json"), ICore::resourcePath("json")}};
    QmlJsEditingSettingsPage m_qmJSEditingSettingsPage;
};

static QmlJSEditorPluginPrivate *dd = nullptr;

QmlJSEditorPluginPrivate::QmlJSEditorPluginPrivate()
{
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
    // restart qmlls when project data changes (qt kit changed, for example)
    connect(
        modelManager,
        &QmlJS::ModelManagerInterface::projectInfoUpdated,
        LanguageClient::LanguageClientManager::instance(),
        []() { LanguageClient::LanguageClientManager::applySettings(qmllsSettings()); });
    connect(modelManager,
            &QmlJS::ModelManagerInterface::aboutToRemoveFiles,
            &m_qmlTaskManager,
            &QmlTaskManager::documentsRemoved);

    Context context(Constants::C_QMLJSEDITOR_ID, Constants::C_QTQUICKDESIGNEREDITOR_ID);

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

    cmd = ActionManager::command(TextEditor::Constants::RENAME_SYMBOL);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    QAction *semanticScan = new QAction(Tr::tr("Run Checks"), this);
    cmd = ActionManager::registerAction(semanticScan, Id("QmlJSEditor.RunSemanticScan"));
    cmd->setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+C")));
    connect(semanticScan, &QAction::triggered, this, &QmlJSEditorPluginPrivate::runSemanticScan);
    qmlToolsMenu->addAction(cmd);


    m_reformatFileAction = ActionBuilder(this, TextEditor::Constants::REFORMAT_FILE)
                             .setContext(context)
                             .addOnTriggered([this] { reformatFile(); })
                             .setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+Shift+;")))
                             .setText(Tr::tr("Reformat Document"))
                             .addToContainer(Core::Constants::M_EDIT_ADVANCED, Core::Constants::G_EDIT_FORMAT)
                             .contextAction();
    cmd = ActionManager::command(TextEditor::Constants::REFORMAT_FILE);
    qmlToolsMenu->addAction(cmd);

    QAction *inspectElementAction = new QAction(Tr::tr("Inspect API for Element Under Cursor"), this);
    cmd = ActionManager::registerAction(inspectElementAction,
                                        Id("QmlJSEditor.InspectElementUnderCursor"),
                                        context);
    connect(inspectElementAction, &QAction::triggered, &Internal::inspectElement);

    qmlToolsMenu->addAction(cmd);

    QAction *showQuickToolbar = new QAction(Tr::tr("Show Qt Quick Toolbar"), this);
    cmd = ActionManager::registerAction(showQuickToolbar, Constants::SHOW_QT_QUICK_HELPER, context);
    cmd->setDefaultKeySequence(useMacShortcuts ? QKeySequence(Qt::META | Qt::ALT | Qt::Key_Space)
                                               : QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_Space));
    connect(showQuickToolbar, &QAction::triggered, this, &showContextPane);
    contextMenu->addAction(cmd);
    qmlToolsMenu->addAction(cmd);

    // Insert marker for "Refactoring" menu:
    Command *sep = contextMenu->addSeparator();
    sep->action()->setObjectName(QLatin1String(Constants::M_REFACTORING_MENU_INSERTION_POINT));
    contextMenu->addSeparator();

    cmd = ActionManager::command(TextEditor::Constants::AUTO_INDENT_SELECTION);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::REFORMAT_FILE);
    contextMenu->addAction(cmd);

    cmd = ActionManager::command(TextEditor::Constants::UN_COMMENT_SELECTION);
    contextMenu->addAction(cmd);

    FileIconProvider::registerIconOverlayForSuffix(ProjectExplorer::Constants::FILEOVERLAY_QML, "qml");

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &QmlJSEditorPluginPrivate::currentEditorChanged);

    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, &QmlJSEditorPluginPrivate::autoFormatOnSave);
}

QmlJS::JsonSchemaManager *jsonManager()
{
    return dd->jsonManager();
}

static void overrideTabSettings(QPointer<QmlJSEditorDocument> document)
{
    // Search .qmlformat.ini and read the tab settings from it
    if (!document)
        return;

    TextEditor::TabSettings tabSettings = document->tabSettings();
    QSettings settings(
        QmlJSTools::QmlFormatSettings::currentQmlFormatIniFile(document->filePath()).toUrlishString(),
        QSettings::IniFormat);

    if (settings.contains("IndentWidth"))
        tabSettings.m_indentSize = settings.value("IndentWidth").toInt();
    if (settings.contains("UseTabs"))
        tabSettings.m_tabPolicy = settings.value("UseTabs").toBool()
                                      ? TextEditor::TabSettings::TabPolicy::TabsOnlyTabPolicy
                                      : TextEditor::TabSettings::TabPolicy::SpacesOnlyTabPolicy;
    document->setTabSettings(tabSettings);
}

static FormatResult reformatByQmlFormat(QPointer<QmlJSEditorDocument> document)
{
    const FilePath &qmlformatPath = QmlFormatSettings::instance().latestQmlFormatPath();
    if (!qmlformatPath.isExecutableFile()) {
        Core::MessageManager::writeSilently(
            Tr::tr("QmlFormat not found."));
        return FormatResult::Failed;
    }
    const CommandLine commandLine(qmlformatPath, {});
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return FormatResult::Failed;
    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForDocument(document);
    if (editors.isEmpty())
        return FormatResult::Failed;
    IEditor *currentEditor = EditorManager::currentEditor();
    IEditor *editor = editors.contains(currentEditor) ? currentEditor : editors.first();
    if (auto widget = TextEditor::TextEditorWidget::fromEditor(editor)) {
        overrideTabSettings(document);
        TextEditor::formatEditor(widget, command);
        return FormatResult::Success;
    }
    return FormatResult::Failed;
}

static FormatResult reformatByBuiltInFormatter(QPointer<QmlJSEditorDocument> document)
{
    if (!document)
        return FormatResult::Failed;
    auto *doc = document->document();
    if (!doc)
        return FormatResult::Failed;

    QmlJS::Document::Ptr documentPtr = document->semanticInfo().document;
    QmlJS::Snapshot snapshot = QmlJS::ModelManagerInterface::instance()->snapshot();

    if (document->isSemanticInfoOutdated()) {
        QmlJS::Document::MutablePtr latestDocument;

        const FilePath fileName = document->filePath();
        latestDocument = snapshot.documentFromSource(
            QString::fromUtf8(document->contents()),
            fileName,
            QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName));
        latestDocument->parseQml();
        snapshot.insert(latestDocument);
        documentPtr = latestDocument;
    }

    if (!documentPtr->isParsedCorrectly())
        return FormatResult::Failed;

    QmlJSTools::QmlJSCodeStylePreferences *codeStyle
        = QmlJSTools::QmlJSToolsSettings::globalCodeStyle();
    TextEditor::TabSettings tabSettings = codeStyle->currentTabSettings();
    const QString newText = QmlJS::reformat(
        documentPtr,
        tabSettings.m_indentSize,
        tabSettings.m_tabSize,
        codeStyle->currentCodeStyleSettings().lineLength);

    QTextCursor tc(document->document());
    auto ed = qobject_cast<TextEditor::BaseTextEditor *>(EditorManager::currentEditor());
    if (ed) {
        TextEditor::updateEditorText(ed->editorWidget(), newText);
    } else {
        tc.movePosition(QTextCursor::Start);
        tc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        tc.insertText(newText);
    }
    // Rewrite doesn't respect tabs, only inserts spaces.
    // Also use indenter to take tabs into account.
    QTextBlock block = doc->firstBlock();
    tc.beginEditBlock();
    while (block.isValid()) {
        if (auto *indenter = document->indenter()) {
            indenter->indentBlock(block, QChar::Null, tabSettings);
        }
        block = block.next();
    }
    tc.endEditBlock();
    return FormatResult::Success;
}

static FormatResult reformatUsingLanguageServer(QPointer<QmlJSEditorDocument> document)
{
    if (!document)
        return FormatResult::Failed;

    if (!document->formatter())
        return FormatResult::Failed;

    TextEditor::BaseTextEditor *editor = qobject_cast<TextEditor::BaseTextEditor *>(
        EditorManager::currentEditor());
    if (!editor)
        return FormatResult::Failed;

    TextEditor::TextEditorWidget *editorWidget = editor->editorWidget();
    if (!editorWidget)
        return FormatResult::Failed;

    overrideTabSettings(document);
    document->setFormatterMode(TextEditor::Formatter::FormatMode::FullDocument);
    editorWidget->autoFormat();
    return FormatResult::Success;
}

static FormatResult reformatByCustomFormatter(
    QPointer<QmlJSEditorDocument> document, const QmlJSTools::QmlJSCodeStyleSettings &settings)
{
    const FilePath &formatter = settings.customFormatterPath;
    const QStringList &args = settings.customFormatterArguments.split(" ", Qt::SkipEmptyParts);
    if (!formatter.isExecutableFile()) {
        MessageManager::writeSilently(Tr::tr("Custom formatter path not found."));
        return FormatResult::Failed;
    }
    const CommandLine commandLine(formatter, args);
    TextEditor::Command command;
    command.setExecutable(commandLine.executable());
    command.setProcessing(TextEditor::Command::FileProcessing);
    command.addOptions(commandLine.splitArguments());
    command.addOption("--inplace");
    command.addOption("%file");
    if (!command.isValid())
        return FormatResult::Failed;
    const QList<Core::IEditor *> editors = Core::DocumentModel::editorsForDocument(document);
    if (editors.isEmpty())
        return FormatResult::Failed;
    IEditor *currentEditor = EditorManager::currentEditor();
    IEditor *editor = editors.contains(currentEditor) ? currentEditor : editors.first();
    if (auto widget = TextEditor::TextEditorWidget::fromEditor(editor)) {
        TextEditor::formatEditor(widget, command);
        return FormatResult::Success;
    }
    return FormatResult::Failed;
}

FormatResult QmlJSEditorPluginPrivate::reformatFile()
{
    if (!m_currentDocument) {
        MessageManager::writeSilently(Tr::tr("Error: No current document to format."));
        return FormatResult::Failed;
    }

    QmlJSTools::QmlJSCodeStylePreferences *codeStyle
        = QmlJSTools::QmlJSToolsSettings::globalCodeStyle();
    const QmlJSCodeStyleSettings settings = codeStyle->currentCodeStyleSettings();

    const auto tryReformat = [this, codeStyle](auto formatterFunction) {
        m_currentDocument->setCodeStyle(codeStyle);
        const FormatResult result = formatterFunction(m_currentDocument);
        if (result != FormatResult::Success) {
            MessageManager::writeSilently(
                Tr::tr("Error: Formatting failed with the selected formatter."));
        }
        return result;
    };

    switch (settings.formatter) {
    case QmlJSCodeStyleSettings::Formatter::QmlFormat:
        return tryReformat([](auto doc) {
            return LanguageClient::LanguageClientManager::clientForDocument(doc)
                       ? reformatUsingLanguageServer(doc)
                       : reformatByQmlFormat(doc);
        });

    case QmlJSCodeStyleSettings::Formatter::Custom:
        return tryReformat(
            [&settings](auto doc) { return reformatByCustomFormatter(doc, settings); });

    case QmlJSCodeStyleSettings::Formatter::Builtin:
    default:
        return tryReformat([](auto doc) { return reformatByBuiltInFormatter(doc); });
    }
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

QmlJSQuickFixAssistProvider *quickFixAssistProvider()
{
    return dd->quickFixAssistProvider();
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
    if (!settings().autoFormatOnSave())
        return;

    // Check that we are dealing with a QML/JS editor
    if (document->id() != Constants::C_QMLJSEDITOR_ID
        && document->id() != Constants::C_QTQUICKDESIGNEREDITOR_ID)
        return;

    // Check if file is contained in the current project (if wished)
    if (settings().autoFormatOnlyCurrentProject()) {
        const Project *pro = ProjectTree::currentProject();
        if (!pro || !pro->files(Project::SourceFiles).contains(document->filePath()))
            return;
    }

    reformatFile();
}

class QmlJSEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlJSEditor.json")

    ~QmlJSEditorPlugin() final
    {
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new QmlJSEditorPluginPrivate;

        registerQmllsSettings();
        setupQmlJsOutline();
        setupQmlJSEditor();
        setupQmlJsEditingProjectPanel();
    }

    void extensionsInitialized() final
    {
        FileIconProvider::registerIconOverlayForMimeType(ProjectExplorer::Constants::FILEOVERLAY_UI,
                                                         Utils::Constants::QMLUI_MIMETYPE);

        TaskHub::addCategory({Constants::TASK_CATEGORY_QML,
                              Tr::tr("QML"),
                              Tr::tr("Issues that the QML code parser found.")});
        TaskHub::addCategory({Constants::TASK_CATEGORY_QML_ANALYSIS,
                              Tr::tr("QML Analysis"),
                              Tr::tr("Issues that the QML static analyzer found."),
                              false});
    }

    bool delayedInitialize() final
    {
        setupQmllsClient();
        return true;
    }
};

} // QmlJSEditor::Internal

#include "qmljseditorplugin.moc"
