// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientutils.h"

#include "client.h"
#include "languageclient_global.h"
#include "languageclientmanager.h"
#include "languageclientoutline.h"
#include "languageclienttr.h"
#include "snippet.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/environment.h>
#include <utils/infobar.h>
#include <utils/qtcprocess.h>
#include <utils/textutils.h>
#include <utils/treeviewcombobox.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QFile>
#include <QMenu>
#include <QTextDocument>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

using namespace LanguageServerProtocol;
using namespace Utils;
using namespace TextEditor;

namespace LanguageClient {

QTextCursor rangeToTextCursor(const Range &range, QTextDocument *doc)
{
    QTextCursor cursor(doc);
    cursor.setPosition(range.end().toPositionInDocument(doc));
    cursor.setPosition(range.start().toPositionInDocument(doc), QTextCursor::KeepAnchor);
    return cursor;
}

ChangeSet::Range convertRange(const QTextDocument *doc, const Range &range)
{
    int start = range.start().toPositionInDocument(doc);
    int end = range.end().toPositionInDocument(doc);
    // This addesses an issue from the python language server where the reported end line
    // was behind the actual end of the document. As a workaround treat every position after
    // the end of the document as the end of the document.
    if (end < 0 && range.end().line() >= doc->blockCount()) {
        QTextCursor tc(doc->firstBlock());
        tc.movePosition(QTextCursor::End);
        end = tc.position();
    }
    return ChangeSet::Range(start, end);
}

ChangeSet editsToChangeSet(const QList<TextEdit> &edits, const QTextDocument *doc)
{
    ChangeSet changeSet;
    for (const TextEdit &edit : edits)
        changeSet.replace(convertRange(doc, edit.range()), edit.newText());
    return changeSet;
}

bool applyTextDocumentEdit(const Client *client, const TextDocumentEdit &edit)
{
    const QList<TextEdit> &edits = edit.edits();
    if (edits.isEmpty())
        return true;
    const DocumentUri &uri = edit.textDocument().uri();
    const FilePath &filePath = client->serverUriToHostPath(uri);
    LanguageClientValue<int> version = edit.textDocument().version();
    if (!version.isNull() && version.value(0) < client->documentVersion(filePath))
        return false;
    return applyTextEdits(client, uri, edits);
}

bool applyTextEdits(const Client *client, const DocumentUri &uri, const QList<TextEdit> &edits)
{
    return applyTextEdits(client, client->serverUriToHostPath(uri), edits);
}

bool applyTextEdits(const Client *client,
                    const Utils::FilePath &filePath,
                    const QList<LanguageServerProtocol::TextEdit> &edits)
{
    if (edits.isEmpty())
        return true;
    const RefactoringFilePtr file = client->createRefactoringFile(filePath);
    return file->apply(editsToChangeSet(edits, file->document()));
}

void applyTextEdit(TextEditorWidget *editorWidget, const TextEdit &edit, bool newTextIsSnippet)
{
    const Range range = edit.range();
    const QTextDocument *doc = editorWidget->document();
    const int start = Text::positionInText(doc, range.start().line() + 1, range.start().character() + 1);
    const int end = Text::positionInText(doc, range.end().line() + 1, range.end().character() + 1);
    if (newTextIsSnippet) {
        editorWidget->replace(start, end - start, {});
        editorWidget->insertCodeSnippet(start, edit.newText(), &parseSnippet);
    } else {
        editorWidget->replace(start, end - start, edit.newText());
    }
}

bool applyWorkspaceEdit(const Client *client, const WorkspaceEdit &edit)
{
    bool result = true;
    const auto documentChanges = edit.documentChanges().value_or(QList<DocumentChange>());
    if (!documentChanges.isEmpty()) {
        for (const DocumentChange &documentChange : documentChanges)
            result |= applyDocumentChange(client, documentChange);
    } else {
        const WorkspaceEdit::Changes &changes = edit.changes().value_or(WorkspaceEdit::Changes());
        for (auto it = changes.cbegin(); it != changes.cend(); ++it)
            result |= applyTextEdits(client, it.key(), it.value());
        return result;
    }
    return result;
}

QTextCursor endOfLineCursor(const QTextCursor &cursor)
{
    QTextCursor ret = cursor;
    ret.movePosition(QTextCursor::EndOfLine);
    return ret;
}

void updateCodeActionRefactoringMarker(Client *client,
                                       const QList<CodeAction> &actions,
                                       const DocumentUri &uri)
{
    TextDocument* doc = TextDocument::textDocumentForFilePath(client->serverUriToHostPath(uri));
    if (!doc)
        return;
    const QVector<BaseTextEditor *> editors = BaseTextEditor::textEditorsForDocument(doc);
    if (editors.isEmpty())
        return;

    QHash<int, RefactorMarker> markersAtBlock;
    const auto addMarkerForCursor = [&](const CodeAction &action, const Range &range) {
        const QTextCursor cursor = endOfLineCursor(range.start().toTextCursor(doc->document()));
        const auto it = markersAtBlock.find(cursor.blockNumber());
        if (it != markersAtBlock.end()) {
            it->tooltip = Tr::tr("Show available quick fixes");
            it->callback = [cursor](TextEditorWidget *editor) {
                editor->setTextCursor(cursor);
                editor->invokeAssist(TextEditor::QuickFix);
            };
            return;
        }
        RefactorMarker marker;
        marker.type = client->id();
        marker.cursor = cursor;
        if (action.isValid())
            marker.tooltip = action.title();
        if (action.edit()) {
            marker.callback = [client, edit = action.edit()](const TextEditorWidget *) {
                applyWorkspaceEdit(client, *edit);
            };
        } else if (action.command()) {
            marker.callback = [command = action.command(),
                    client = QPointer(client)](const TextEditorWidget *) {
                if (client)
                    client->executeCommand(*command);
            };
        }
        markersAtBlock[cursor.blockNumber()] = marker;
    };

    for (const CodeAction &action : actions) {
        const QList<Diagnostic> &diagnostics = action.diagnostics().value_or(QList<Diagnostic>());
        if (std::optional<WorkspaceEdit> edit = action.edit()) {
            if (diagnostics.isEmpty()) {
                QList<TextEdit> edits;
                if (std::optional<QList<DocumentChange>> documentChanges = edit->documentChanges()) {
                    for (const DocumentChange &change : *documentChanges) {
                        if (auto edit = std::get_if<TextDocumentEdit>(&change)) {
                            if (edit->textDocument().uri() == uri)
                                edits << edit->edits();
                        }
                    }
                } else if (std::optional<WorkspaceEdit::Changes> localChanges = edit->changes()) {
                    edits = (*localChanges)[uri];
                }
                for (const TextEdit &edit : std::as_const(edits))
                    addMarkerForCursor(action, edit.range());
            }
        }
        for (const Diagnostic &diagnostic : diagnostics)
            addMarkerForCursor(action, diagnostic.range());
    }
    const RefactorMarkers markers = markersAtBlock.values();
    for (BaseTextEditor *editor : editors) {
        if (TextEditorWidget *editorWidget = editor->editorWidget())
            editorWidget->setRefactorMarkers(markers, client->id());
    }
}

static const char clientExtrasName[] = "__qtcreator_client_extras__";

class ClientExtras : public QObject
{
public:
    ClientExtras(QObject *parent) : QObject(parent) { setObjectName(clientExtrasName); };

    QPointer<QAction> m_popupAction;
    QPointer<Client> m_client;
    QPointer<QWidget> m_outline;
};

void updateEditorToolBar(Core::IEditor *editor)
{
    auto *textEditor = qobject_cast<BaseTextEditor *>(editor);
    if (!textEditor)
        return;
    TextEditorWidget *widget = textEditor->editorWidget();
    if (!widget)
        return;

    TextDocument *document = textEditor->textDocument();
    Client *client = LanguageClientManager::clientForDocument(textEditor->textDocument());

    ClientExtras *extras = dynamic_cast<ClientExtras *>(
        widget->findChild<QObject *>(clientExtrasName, Qt::FindDirectChildrenOnly));
    if (!extras) {
        if (!client)
            return;
        extras = new ClientExtras(widget);
    }
    if (extras->m_popupAction) {
        if (client) {
            extras->m_popupAction->setText(client->name());
        } else {
            widget->toolBar()->removeAction(extras->m_popupAction);
            delete extras->m_popupAction;
        }
    } else if (client) {
        const QIcon icon = Utils::Icon({{":/languageclient/images/languageclient.png",
                                         Utils::Theme::IconsBaseColor}}).icon();
        extras->m_popupAction = widget->toolBar()->addAction(
                    icon, client->name(), [document = QPointer(document), client = QPointer<Client>(client)] {
            auto menu = new QMenu;
            auto clientsGroup = new QActionGroup(menu);
            clientsGroup->setExclusive(true);
            for (auto client : LanguageClientManager::clientsSupportingDocument(document, false)) {
                auto action = clientsGroup->addAction(client->name());
                auto reopen = [action, client = QPointer(client), document] {
                    if (!client)
                        return;
                    LanguageClientManager::openDocumentWithClient(document, client);
                    action->setChecked(true);
                };
                action->setCheckable(true);
                action->setChecked(client == LanguageClientManager::clientForDocument(document));
                action->setEnabled(client->reachable());
                QObject::connect(client, &Client::stateChanged, action, [action, client] {
                    action->setEnabled(client->reachable());
                });
                QObject::connect(action, &QAction::triggered, reopen);
            }
            menu->addActions(clientsGroup->actions());
            if (!clientsGroup->actions().isEmpty())
                menu->addSeparator();
            if (client && client->reachable()) {
                menu->addAction(Tr::tr("Restart %1").arg(client->name()), [client] {
                    if (client && client->reachable())
                        LanguageClientManager::restartClient(client);
                });
            }
            menu->addAction(Tr::tr("Inspect Language Clients"), [] {
                LanguageClientManager::showInspector();
            });
            menu->addAction(Tr::tr("Manage..."), [] {
                Core::ICore::showOptionsDialog(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
            });
            menu->popup(QCursor::pos());
        });
    }

    if (!extras->m_client || !client || extras->m_client != client
        || !client->supportsDocumentSymbols(document)) {
        if (extras->m_outline && widget->toolbarOutlineWidget() == extras->m_outline)
            widget->setToolbarOutline(nullptr);
        extras->m_client.clear();
    }

    if (!extras->m_client) {
        extras->m_outline = LanguageClientOutlineWidgetFactory::createComboBox(client, textEditor);
        if (extras->m_outline) {
            widget->setToolbarOutline(extras->m_outline);
            extras->m_client = client;
        }
    }
}

const QIcon symbolIcon(int type)
{
    using namespace Utils::CodeModelIcon;
    static QMap<SymbolKind, QIcon> icons;
    if (type < int(SymbolKind::FirstSymbolKind) || type > int(SymbolKind::LastSymbolKind))
        return {};
    auto kind = static_cast<SymbolKind>(type);
    if (!icons.contains(kind)) {
        switch (kind) {
        case SymbolKind::File: icons[kind] = Utils::Icons::NEWFILE.icon(); break;
        case SymbolKind::Module:
        case SymbolKind::Namespace:
        case SymbolKind::Package: icons[kind] = iconForType(Namespace); break;
        case SymbolKind::Class: icons[kind] = iconForType(Class); break;
        case SymbolKind::Method: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::Property: icons[kind] = iconForType(Property); break;
        case SymbolKind::Field: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Constructor: icons[kind] = iconForType(Class); break;
        case SymbolKind::Enum: icons[kind] = iconForType(Enum); break;
        case SymbolKind::Interface: icons[kind] = iconForType(Class); break;
        case SymbolKind::Function: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::Variable:
        case SymbolKind::Constant:
        case SymbolKind::String:
        case SymbolKind::Number:
        case SymbolKind::Boolean:
        case SymbolKind::Array: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Object: icons[kind] = iconForType(Class); break;
        case SymbolKind::Key:
        case SymbolKind::Null: icons[kind] = iconForType(Keyword); break;
        case SymbolKind::EnumMember: icons[kind] = iconForType(Enumerator); break;
        case SymbolKind::Struct: icons[kind] = iconForType(Struct); break;
        case SymbolKind::Event:
        case SymbolKind::Operator: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::TypeParameter: icons[kind] = iconForType(VarPublic); break;
        }
    }
    return icons[kind];
}

bool applyDocumentChange(const Client *client, const DocumentChange &change)
{
    if (!client)
        return false;

    if (const auto e = std::get_if<TextDocumentEdit>(&change)) {
        return applyTextDocumentEdit(client, *e);
    } else if (const auto createOperation = std::get_if<CreateFileOperation>(&change)) {
        const FilePath filePath = createOperation->uri().toFilePath(client->hostPathMapper());
        if (filePath.exists()) {
            if (const std::optional<CreateFileOptions> options = createOperation->options()) {
                if (options->overwrite().value_or(false)) {
                    if (!filePath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return filePath.ensureExistingFile();
    } else if (const auto renameOperation = std::get_if<RenameFileOperation>(&change)) {
        const FilePath oldPath = renameOperation->oldUri().toFilePath(client->hostPathMapper());
        if (!oldPath.exists())
            return false;
        const FilePath newPath = renameOperation->newUri().toFilePath(client->hostPathMapper());
        if (oldPath == newPath)
            return true;
        if (newPath.exists()) {
            if (const std::optional<CreateFileOptions> options = renameOperation->options()) {
                if (options->overwrite().value_or(false)) {
                    if (!newPath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return oldPath.renameFile(newPath);
    } else if (const auto deleteOperation = std::get_if<DeleteFileOperation>(&change)) {
        const FilePath filePath = deleteOperation->uri().toFilePath(client->hostPathMapper());
        if (const std::optional<DeleteFileOptions> options = deleteOperation->options()) {
            if (!filePath.exists())
                return options->ignoreIfNotExists().value_or(false);
            if (filePath.isDir() && options->recursive().value_or(false))
                return filePath.removeRecursively();
        }
        return filePath.removeFile().has_value();
    }
    return false;
}

constexpr char installJsonLsInfoBarId[] = "LanguageClient::InstallJsonLs";
constexpr char installYamlLsInfoBarId[] = "LanguageClient::InstallYamlLs";
constexpr char installBashLsInfoBarId[] = "LanguageClient::InstallBashLs";

const char npmInstallTaskId[] = "LanguageClient::npmInstallTask";

class NpmInstallTask : public QObject
{
    Q_OBJECT
public:
    NpmInstallTask(const FilePath &npm,
                   const FilePath &workingDir,
                   const QString &package,
                   QObject *parent = nullptr)
        : QObject(parent)
        , m_package(package)
    {
        m_process.setCommand(CommandLine(npm, {"install", package}));
        m_process.setWorkingDirectory(workingDir);
        m_process.setTerminalMode(TerminalMode::Run);
        connect(&m_process, &Process::done, this, &NpmInstallTask::handleDone);
        connect(&m_killTimer, &QTimer::timeout, this, &NpmInstallTask::cancel);
        connect(&m_watcher, &QFutureWatcher<void>::canceled, this, &NpmInstallTask::cancel);
        m_watcher.setFuture(m_future.future());
    }
    void run()
    {
        const QString taskTitle = Tr::tr("Install npm Package");
        Core::ProgressManager::addTask(m_future.future(), taskTitle, npmInstallTaskId);

        m_process.start();

        Core::MessageManager::writeSilently(
            Tr::tr("Running \"%1\" to install %2.")
                .arg(m_process.commandLine().toUserOutput(), m_package));

        m_killTimer.setSingleShot(true);
        m_killTimer.start(5 /*minutes*/ * 60 * 1000);
    }

signals:
    void finished(bool success);

private:
    void cancel()
    {
        m_process.stop();
        m_process.waitForFinished();
        Core::MessageManager::writeFlashing(
            m_killTimer.isActive()
                ? Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(m_package)
                : Tr::tr("The installation of \"%1\" was canceled by the user.")
                      .arg(m_package));
    }
    void handleDone()
    {
        m_future.reportFinished();
        const bool success = m_process.result() == ProcessResult::FinishedWithSuccess;
        if (!success) {
            Core::MessageManager::writeFlashing(Tr::tr("Installing \"%1\" failed with exit code %2.")
                                                    .arg(m_package)
                                                    .arg(m_process.exitCode()));
        }
        emit finished(success);
    }

    QString m_package;
    Utils::Process m_process;
    QFutureInterface<void> m_future;
    QFutureWatcher<void> m_watcher;
    QTimer m_killTimer;
};

constexpr char YAML_MIME_TYPE[]{"application/x-yaml"};
constexpr char SHELLSCRIPT_MIME_TYPE[]{"application/x-shellscript"};
constexpr char JSON_MIME_TYPE[]{"application/json"};

static void setupNpmServer(TextDocument *document,
                           const Id &infoBarId,
                           const QString &languageServer,
                           const QString &languageServerArgs,
                           const QString &language,
                           const QStringList &serverMimeTypes)
{
    InfoBar *infoBar = document->infoBar();
    if (!infoBar->canInfoBeAdded(infoBarId))
        return;

    // check if it is already configured
    const QList<BaseSettings *> settings = LanguageClientManager::currentSettings();
    for (BaseSettings *setting : settings) {
        if (setting->isValid() && setting->m_languageFilter.isSupported(document))
            return;
    }

    // check for npm
    const FilePath npm = Environment::systemEnvironment().searchInPath("npm");
    if (!npm.isExecutableFile())
        return;

    FilePath lsExecutable;

    Process process;
    process.setCommand(CommandLine(npm, {"list", "-g", languageServer}));
    process.start();
    process.waitForFinished();
    if (process.exitCode() == 0) {
        const FilePath lspath = FilePath::fromUserInput(process.stdOutLines().value(0));
        lsExecutable = lspath.pathAppended(languageServer);
        if (HostOsInfo::isWindowsHost())
            lsExecutable = lsExecutable.stringAppended(".cmd");
    }

    const bool install = !lsExecutable.isExecutableFile();

    const QString message = install ? Tr::tr("Install %1 language server via npm.").arg(language)
                                    : Tr::tr("Setup %1 language server (%2).")
                                          .arg(language)
                                          .arg(lsExecutable.toUserOutput());
    InfoBarEntry info(infoBarId, message, InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(install ? Tr::tr("Install") : Tr::tr("Setup"), [=]() {
        const QList<Core::IDocument *> &openedDocuments = Core::DocumentModel::openedDocuments();
        for (Core::IDocument *doc : openedDocuments)
            doc->infoBar()->removeInfo(infoBarId);

        auto setupStdIOSettings = [=](const FilePath &executable) {
            auto settings = new StdIOSettings();

            settings->m_executable = executable;
            settings->m_arguments = languageServerArgs;
            settings->m_name = Tr::tr("%1 Language Server").arg(language);
            settings->m_languageFilter.mimeTypes = serverMimeTypes;
            LanguageClientSettings::addSettings(settings);
            LanguageClientManager::applySettings();
        };

        if (install) {
            const FilePath lsPath = Core::ICore::userResourcePath(languageServer);
            if (!lsPath.ensureWritableDir())
                return;
            auto install = new NpmInstallTask(npm,
                                              lsPath,
                                              languageServer,
                                              LanguageClientManager::instance());

            auto handleInstall = [=](const bool success) {
                install->deleteLater();
                if (!success)
                    return;
                FilePath relativePath = FilePath::fromPathPart(
                    QString("node_modules/.bin/" + languageServer));
                if (HostOsInfo::isWindowsHost())
                    relativePath = relativePath.withSuffix(".cmd");
                FilePath lsExecutable = lsPath.resolvePath(relativePath);
                if (lsExecutable.isExecutableFile()) {
                    setupStdIOSettings(lsExecutable);
                    return;
                }
                Process process;
                process.setCommand(CommandLine(npm, {"list", languageServer}));
                process.setWorkingDirectory(lsPath);
                process.start();
                process.waitForFinished();
                const QStringList output = process.stdOutLines();
                // we are expecting output in the form of:
                // tst@ C:\tmp\tst
                // `-- vscode-json-languageserver@1.3.4
                for (const QString &line : output) {
                    const qsizetype splitIndex = line.indexOf('@');
                    if (splitIndex == -1)
                        continue;
                    lsExecutable = FilePath::fromUserInput(line.mid(splitIndex + 1).trimmed())
                                       .resolvePath(relativePath);
                    if (lsExecutable.isExecutableFile()) {
                        setupStdIOSettings(lsExecutable);
                        return;
                    }
                }
            };

            QObject::connect(install,
                             &NpmInstallTask::finished,
                             LanguageClientManager::instance(),
                             handleInstall);

            install->run();
        } else {
            setupStdIOSettings(lsExecutable);
        }
    });
    infoBar->addInfo(info);
}

void autoSetupLanguageServer(TextDocument *document)
{
    const MimeType mimeType = Utils::mimeTypeForName(document->mimeType());

    if (mimeType.inherits(JSON_MIME_TYPE)) {
        setupNpmServer(document,
                       installJsonLsInfoBarId,
                       "vscode-json-languageserver",
                       "--stdio",
                       QString("JSON"),
                       {JSON_MIME_TYPE});
    } else if (mimeType.inherits(YAML_MIME_TYPE)) {
        setupNpmServer(document,
                       installYamlLsInfoBarId,
                       "yaml-language-server",
                       "--stdio",
                       QString("YAML"),
                       {YAML_MIME_TYPE});
    } else if (mimeType.inherits(SHELLSCRIPT_MIME_TYPE)) {
        setupNpmServer(document,
                       installBashLsInfoBarId,
                       "bash-language-server",
                       "start",
                       QString("Bash"),
                       {SHELLSCRIPT_MIME_TYPE});
    }
}

} // namespace LanguageClient

#include "languageclientutils.moc"
