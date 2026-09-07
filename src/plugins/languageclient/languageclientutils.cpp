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
#include <coreplugin/progressmanager/processprogress.h>

#include <languageserverprotocol/lsputils.h>

#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/environment.h>
#include <utils/globaltasktree.h>
#include <utils/infobar.h>
#include <utils/qtcprocess.h>
#include <utils/textutils.h>
#include <utils/treeviewcombobox.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QMenu>
#include <QTextDocument>
#include <QToolBar>

using namespace Core;
using namespace LanguageServerProtocol;
using namespace QtTaskTree;
using namespace TextEditor;
using namespace Utils;

namespace LanguageClient {

static ChangeSet::Range convertRange(const QTextDocument *doc, const Range &range)
{
    int start = positionInDocument(range.start(), doc);
    int end = positionInDocument(range.end(), doc);
    // See the overload above for why a position past the end of the document is
    // treated as its end.
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
    QList<TextEdit> edits;
    for (const TextDocumentEditEditsItem &item : edit.edits()) {
        if (const auto textEdit = std::get_if<TextEdit>(&item))
            edits << *textEdit;
        else if (const auto annotated = std::get_if<AnnotatedTextEdit>(&item))
            edits << TextEdit().range(annotated->range()).newText(annotated->newText());
    }
    if (edits.isEmpty())
        return true;
    const QString &uri = edit.textDocument().uri();
    const FilePath &filePath = client->filePathFor(uri);
    if (const std::optional<int> &version = edit.textDocument().version()) {
        if (*version < client->documentVersion(filePath))
            return false;
    }
    return applyTextEdits(client, uri, edits);
}

bool applyTextEdits(const Client *client, const QString &uri, const QList<TextEdit> &edits)
{
    return applyTextEdits(client, client->filePathFor(uri), edits);
}

bool applyTextEdits(
    const Client *client, const Utils::FilePath &filePath, const QList<TextEdit> &edits)
{
    if (edits.isEmpty())
        return true;
    const RefactoringFilePtr file = client->createRefactoringFile(filePath);
    return file->apply(editsToChangeSet(edits, file->document()));
}

void applyTextEdit(TextEditorWidget *editorWidget, const TextEdit &edit, bool newTextIsSnippet)
{
    const QTextDocument *doc = editorWidget->document();
    const int start = positionInDocument(edit.range().start(), doc);
    const int end = positionInDocument(edit.range().end(), doc);
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
    const auto documentChanges = edit.documentChanges().value_or(
        QList<WorkspaceEditDocumentChangesItem>());
    if (!documentChanges.isEmpty()) {
        for (const WorkspaceEditDocumentChangesItem &documentChange : documentChanges)
            result |= applyDocumentChange(client, documentChange);
    } else {
        const auto changes = edit.changes().value_or(QMap<QString, QList<TextEdit>>());
        for (auto it = changes.cbegin(); it != changes.cend(); ++it)
            result |= applyTextEdits(client, it.key(), it.value());
        return result;
    }
    return result;
}

Utils::Link linkFor(const Client *client, const Location &location)
{
    return Utils::Link(client->filePathFor(location.uri()),
                       location.range().start().line() + 1,
                       location.range().start().character());
}

static QTextCursor endOfLineCursor(const QTextCursor &cursor)
{
    QTextCursor ret = cursor;
    ret.movePosition(QTextCursor::EndOfLine);
    return ret;
}

void updateCodeActionRefactoringMarker(
    Client *client, const QList<CodeAction> &actions, const QString &uri)
{
    TextDocument *doc = TextDocument::textDocumentForFilePath(client->filePathFor(uri));
    if (!doc)
        return;
    const QList<TextEditorWidget *> editorWidgets = TextEditorWidget::textEditorWidgetsForDocument(doc);
    if (editorWidgets.isEmpty())
        return;

    QHash<int, RefactorMarker> markersAtBlock;
    const auto addMarkerForCursor = [&](const CodeAction &action, const Range &range) {
        const QTextCursor cursor = endOfLineCursor(toTextCursor(range.start(), doc->document()));
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
        marker.tooltip = action.title();
        if (action.edit()) {
            marker.callback = [client = QPointer(client),
                               edit = action.edit()](const TextEditorWidget *) {
                if (QTC_GUARD(client))
                    applyWorkspaceEdit(client, *edit);
            };
        } else if (action.command()) {
            marker.callback = [command = action.command(),
                    client = QPointer(client)](const TextEditorWidget *) {
                if (QTC_GUARD(client))
                    client->executeCommand(*command);
            };
        }
        markersAtBlock[cursor.blockNumber()] = marker;
    };

    for (const CodeAction &action : actions) {
        const QList<Diagnostic> diagnostics = action.diagnostics().value_or(QList<Diagnostic>());
        if (const std::optional<WorkspaceEdit> &edit = action.edit()) {
            if (diagnostics.isEmpty()) {
                QList<Range> ranges;
                if (const auto &documentChanges = edit->documentChanges()) {
                    for (const WorkspaceEditDocumentChangesItem &change : *documentChanges) {
                        const auto documentEdit = std::get_if<TextDocumentEdit>(&change);
                        if (!documentEdit || documentEdit->textDocument().uri() != uri)
                            continue;
                        for (const TextDocumentEditEditsItem &item : documentEdit->edits()) {
                            if (const auto textEdit = std::get_if<TextEdit>(&item))
                                ranges << textEdit->range();
                            else if (const auto annotated = std::get_if<AnnotatedTextEdit>(&item))
                                ranges << annotated->range();
                        }
                    }
                } else if (const auto &localChanges = edit->changes()) {
                    for (const TextEdit &textEdit : localChanges->value(uri))
                        ranges << textEdit.range();
                }
                for (const Range &range : std::as_const(ranges))
                    addMarkerForCursor(action, range);
            }
        }
        for (const Diagnostic &diagnostic : diagnostics)
            addMarkerForCursor(action, diagnostic.range());
    }
    const RefactorMarkers markers = markersAtBlock.values();
    for (TextEditorWidget *editorWidget : editorWidgets)
        editorWidget->setRefactorMarkers(markers, client->id());
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

bool registrationApplies(const QJsonValue &registrationOptions,
                         const Utils::FilePath &filePath,
                         const QString &mimeType)
{
    const Utils::Result<TextDocumentRegistrationOptions> options
        = fromJson<TextDocumentRegistrationOptions>(registrationOptions);
    if (!options)
        return true;
    return applies(options->documentSelector(), filePath, Utils::mimeTypeForName(mimeType));
}

void updateEditorToolBar(Core::IEditor *editor)
{
    static QString msgNoLanguageClientSelected = Tr::tr("No language client selected.");
    TextEditorWidget *widget = TextEditorWidget::fromEditor(editor);
    if (!widget)
        return;

    TextDocument *document = widget->textDocument();
    Client *client = LanguageClientManager::clientForDocument(document);
    const QList<Client *> supportingClients
        = LanguageClientManager::clientsSupportingDocument(document, false);

    ClientExtras *extras = dynamic_cast<ClientExtras *>(
        widget->findChild<QObject *>(clientExtrasName, Qt::FindDirectChildrenOnly));
    if (!extras) {
        if (!client && supportingClients.isEmpty())
            return;
        extras = new ClientExtras(widget);
    }
    if (extras->m_popupAction) {
        if (client) {
            extras->m_popupAction->setText(client->name());
        } else if (!supportingClients.isEmpty()) {
            extras->m_popupAction->setText(msgNoLanguageClientSelected);
        } else {
            widget->toolBar()->removeAction(extras->m_popupAction);
            delete extras->m_popupAction;
        }
    } else if (client || !supportingClients.isEmpty()) {
        const QIcon icon = Utils::Icon({{":/languageclient/images/languageclient.png",
                                         Utils::Theme::IconsBaseColor}}).icon();
        const QString name = client ? client->name() : msgNoLanguageClientSelected;
        extras->m_popupAction = widget->toolBar()->addAction(
                    icon, name, [widget, document = QPointer(document)] {
            auto menu = new QMenu(widget);
            menu->setAttribute(Qt::WA_DeleteOnClose);
            auto clientsGroup = new QActionGroup(menu);
            clientsGroup->setExclusive(true);
            Client *currentClient = LanguageClientManager::clientForDocument(document);
            for (auto client : LanguageClientManager::clientsSupportingDocument(document, false)) {
                if (!client->activatable())
                    continue;
                auto action = clientsGroup->addAction(client->name());
                auto reopen = [action, client = QPointer(client), document] {
                    if (!client)
                        return;
                    LanguageClientManager::openDocumentWithClient(document, client);
                    action->setChecked(true);
                };
                action->setCheckable(true);
                action->setChecked(client == currentClient);
                action->setEnabled(client->reachable());
                QObject::connect(client, &Client::stateChanged, action, [action, client] {
                    action->setEnabled(client->reachable());
                });
                QObject::connect(action, &QAction::triggered, reopen);
            }
            menu->addActions(clientsGroup->actions());
            if (!clientsGroup->actions().isEmpty())
                menu->addSeparator();
            if (currentClient && currentClient->reachable()) {
                menu->addAction(Tr::tr("Restart %1").arg(currentClient->name()),
                                [currentClient = QPointer(currentClient)] {
                    if (currentClient && currentClient->reachable())
                        LanguageClientManager::restartClient(currentClient);
                });
            }
            menu->addAction(Tr::tr("Inspect Language Clients"), [] {
                LanguageClientManager::showInspector();
            });
            menu->addAction(Tr::tr("Manage..."), [] {
                Core::ICore::showSettings(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
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
        extras->m_outline = createOutlineComboBox(client, widget);
        if (extras->m_outline) {
            widget->setToolbarOutline(extras->m_outline);
            extras->m_client = client;
        }
    }
}

// Symbol tags beyond the one the protocol defines, which servers use to convey
// the visibility of a symbol.
namespace SymbolTag {
constexpr int Private = 2;
constexpr int Protected = 4;
constexpr int Static = 8;
} // namespace SymbolTag

static CodeModelIcon::Type symbolTypeToIconType(int kind, const QList<int> &tags)
{
    const auto isPrivate = [&] { return tags.contains(SymbolTag::Private); };
    const auto isProtected = [&] { return tags.contains(SymbolTag::Protected); };
    const auto isStatic = [&] { return tags.contains(SymbolTag::Static); };

    using namespace Utils::CodeModelIcon;
    switch (kind) {
    case SymbolKind::Module:
    case SymbolKind::Namespace:
    case SymbolKind::Package:
        return Namespace;
    case SymbolKind::Class:
    case SymbolKind::Interface:
    case SymbolKind::Constructor:
    case SymbolKind::Object:
        return Class;
    case SymbolKind::Property:
        return Property;
    case SymbolKind::Field:
    case SymbolKind::Variable:
        if (isStatic()) {
            if (isPrivate())
                return VarPrivateStatic;
            if (isProtected())
                return VarProtectedStatic;
            return VarPublicStatic;
        }
        if (isPrivate())
            return VarPrivate;
        if (isProtected())
            return VarProtected;
        [[fallthrough]];
    case SymbolKind::Constant:
    case SymbolKind::String:
    case SymbolKind::Number:
    case SymbolKind::Boolean:
    case SymbolKind::Array:
    case SymbolKind::TypeParameter:
        return VarPublic;
    case SymbolKind::Enum:
        return Enum;
    case SymbolKind::Function:
    case SymbolKind::Method:
    case SymbolKind::Operator:
        if (isStatic()) {
            if (isPrivate())
                return FuncPrivateStatic;
            if (isProtected())
                return FuncProtectedStatic;
            return FuncPublicStatic;
        }
        if (isPrivate())
            return FuncPrivate;
        if (isProtected())
            return FuncProtected;
        [[fallthrough]];
    case SymbolKind::Event:
        return FuncPublic;
    case SymbolKind::Key:
    case SymbolKind::Null:
        return Keyword;
    case SymbolKind::EnumMember:
        return Enumerator;
    case SymbolKind::Struct:
        return Struct;
    default:
        break;
    }
    return Unknown;
}

const QIcon symbolIcon(int type, const QList<int> &tags)
{
    if (type < SymbolKind::File || type > SymbolKind::TypeParameter)
        return {};

    if (type == SymbolKind::File)
        return Icons::NEWFILE.icon();

    using namespace Utils::CodeModelIcon;
    const Type iconType = symbolTypeToIconType(type, tags);
    static QMap<Type, QIcon> icons;
    const auto icon = icons.constFind(iconType);
    if (icon != icons.constEnd())
        return *icon;

    return icons[iconType] = iconForType(iconType);
}

bool applyDocumentChange(const Client *client, const WorkspaceEditDocumentChangesItem &change)
{
    if (!client)
        return false;

    if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
        return applyTextDocumentEdit(client, *documentEdit);
    } else if (const auto createOperation = std::get_if<LanguageServerProtocol::CreateFile>(&change)) {
        const FilePath filePath = client->filePathFor(createOperation->uri());
        if (filePath.exists()) {
            if (const std::optional<CreateFileOptions> &options = createOperation->options()) {
                if (options->overwrite().value_or(false)) {
                    if (!filePath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return filePath.ensureExistingFile();
    } else if (const auto renameOperation = std::get_if<RenameFile>(&change)) {
        const FilePath oldPath = client->filePathFor(renameOperation->oldUri());
        if (!oldPath.exists())
            return false;
        const FilePath newPath = client->filePathFor(renameOperation->newUri());
        if (oldPath == newPath)
            return true;
        if (newPath.exists()) {
            if (const std::optional<RenameFileOptions> &options = renameOperation->options()) {
                if (options->overwrite().value_or(false)) {
                    if (!newPath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return bool(oldPath.renameFile(newPath));
    } else if (const auto deleteOperation = std::get_if<LanguageServerProtocol::DeleteFile>(&change)) {
        const FilePath filePath = client->filePathFor(deleteOperation->uri());
        if (const std::optional<DeleteFileOptions> &options = deleteOperation->options()) {
            if (!filePath.exists())
                return options->ignoreIfNotExists().value_or(false);
            if (filePath.isDir() && options->recursive().value_or(false))
                return filePath.removeRecursively().has_value();
        }
        return bool(filePath.removeFile());
    }
    return false;
}

constexpr char installJsonLsInfoBarId[] = "LanguageClient::InstallJsonLs";
constexpr char installYamlLsInfoBarId[] = "LanguageClient::InstallYamlLs";
constexpr char installBashLsInfoBarId[] = "LanguageClient::InstallBashLs";
constexpr char installDockerfileLsInfoBarId[] = "LanguageClient::InstallDockerfileLs";

constexpr char YAML_MIME_TYPE[]{"application/x-yaml"};
constexpr char SHELLSCRIPT_MIME_TYPE[]{"application/x-shellscript"};
constexpr char JSON_MIME_TYPE[]{"application/json"};
constexpr char DOCKERFILE_MIME_TYPE[]{"application/x-dockerfile"};

static FilePath relativePathForServer(const QString &languageServer)
{
    const FilePath relativePath = FilePath::fromPathPart(
        QString("node_modules/.bin/" + languageServer));
    return HostOsInfo::isWindowsHost() ? relativePath.withSuffix(".cmd") : relativePath;
}

static void setupNpmServer(
    TextDocument *document,
    const Id &infoBarId,
    const QString &languageServer,
    const QString &languageServerArgs,
    const QString &language,
    const QStringList &serverMimeTypes,
    const QString &executableName = QString())
{
    InfoBar *infoBar = document->infoBar();
    if (!infoBar->canInfoBeAdded(infoBarId))
        return;

    // check if it is already configured
    const QList<BaseSettings *> settings = LanguageClientManager::currentSettings();
    for (BaseSettings *setting : settings) {
        if (setting->isValid() && setting->languageFilter().isSupported(document))
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
                                    : Tr::tr("Set up %1 language server (%2).")
                                          .arg(language)
                                          .arg(lsExecutable.toUserOutput());
    InfoBarEntry info(infoBarId, message, InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(install ? Tr::tr("Install") : Tr::tr("Set Up"), [=]() {
        const QList<IDocument *> &openedDocuments = DocumentModel::openedDocuments();
        for (IDocument *doc : openedDocuments)
            doc->infoBar()->removeInfo(infoBarId);

        auto setupStdIOSettings = [=](const FilePath &executable) {
            auto settings = new StdIOSettings();

            settings->executable.setValue(executable);
            settings->arguments.setValue(languageServerArgs);
            settings->name.setValue(Tr::tr("%1 Language Server").arg(language));
            settings->mimeTypes.setValue(serverMimeTypes);
            LanguageClientSettings::addSettings(settings);
            LanguageClientManager::applySettings();
        };

        if (install) {
            const FilePath lsPath = ICore::userResourcePath(languageServer);
            if (!lsPath.ensureWritableDir())
                return;

            const auto onInstallSetup = [npm, lsPath, languageServer](Process &process) {
                process.setCommand({npm, {"install", languageServer}});
                process.setWorkingDirectory(lsPath);
                process.setTerminalMode(TerminalMode::Run);
                auto progress = new ProcessProgress(&process);
                progress->setDisplayName(Tr::tr("Install npm Package"));
                MessageManager::writeSilently(Tr::tr("Running \"%1\" to install %2.")
                    .arg(process.commandLine().toUserOutput(), languageServer));
            };
            const auto onInstallDone = [languageServer](const Process &process) {
                MessageManager::writeFlashing(Tr::tr("Installing \"%1\" failed with exit code %2.")
                    .arg(languageServer).arg(process.exitCode()));
            };
            const auto onInstallTimeout = [languageServer] {
                MessageManager::writeFlashing(
                    Tr::tr("The installation of \"%1\" was canceled by timeout.").arg(languageServer));
            };

            const auto onListSetup = [npm, lsPath, languageServer, setupStdIOSettings](Process &process) {
                const FilePath lsExecutable = lsPath.resolvePath(relativePathForServer(languageServer));
                if (lsExecutable.isExecutableFile()) {
                    setupStdIOSettings(lsExecutable);
                    return SetupResult::StopWithSuccess;
                }
                process.setCommand(CommandLine(npm, {"list", languageServer}));
                process.setWorkingDirectory(lsPath);
                return SetupResult::Continue;
            };
            const auto onListDone = [languageServer, setupStdIOSettings, executableName](
                                        const Process &process) {
                const QStringList output = process.stdOutLines();
                // we are expecting output in the form of:
                // tst@ C:\tmp\tst
                // `-- vscode-json-languageserver@1.3.4
                for (const QString &line : output) {
                    const qsizetype splitIndex = line.indexOf('@');
                    if (splitIndex == -1)
                        continue;
                    const FilePath lsExecutable
                        = FilePath::fromUserInput(line.mid(splitIndex + 1).trimmed())
                              .resolvePath(relativePathForServer(
                                  executableName.isEmpty() ? languageServer : executableName));
                    if (lsExecutable.isExecutableFile()) {
                        setupStdIOSettings(lsExecutable);
                        return;
                    }
                }
            };

            using namespace std::literals::chrono_literals;
            const Group recipe {
                ProcessTask(onInstallSetup, onInstallDone, CallDoneFlag::OnError)
                    .withTimeout(5min, onInstallTimeout),
                ProcessTask(onListSetup, onListDone)
            };
            GlobalTaskTree::start(recipe);
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
    } else if (mimeType.inherits(DOCKERFILE_MIME_TYPE)) {
        setupNpmServer(
            document,
            installDockerfileLsInfoBarId,
            "dockerfile-language-server-nodejs",
            "--stdio",
            QString("Dockerfile"),
            {DOCKERFILE_MIME_TYPE},
            "docker-langserver");
    }
}

} // namespace LanguageClient
