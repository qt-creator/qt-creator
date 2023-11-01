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

#include <texteditor/codeassist/textdocumentmanipulatorinterface.h>
#include <texteditor/refactoringchanges.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/textutils.h>
#include <utils/treeviewcombobox.h>
#include <utils/utilsicons.h>

#include <QActionGroup>
#include <QFile>
#include <QMenu>
#include <QTextDocument>
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
    RefactoringChangesData * const backend = client->createRefactoringChangesBackend();
    RefactoringChanges changes(backend);
    RefactoringFilePtr file;
    file = changes.file(filePath);
    file->setChangeSet(editsToChangeSet(edits, file->document()));
    if (backend) {
        for (const TextEdit &edit : edits)
            file->appendIndentRange(convertRange(file->document(), edit.range()));
    }
    return file->apply();
}

void applyTextEdit(TextDocumentManipulatorInterface &manipulator,
                   const TextEdit &edit,
                   bool newTextIsSnippet)
{
    const Range range = edit.range();
    const QTextDocument *doc = manipulator.textCursorAt(manipulator.currentPosition()).document();
    const int start = Text::positionInText(doc, range.start().line() + 1, range.start().character() + 1);
    const int end = Text::positionInText(doc, range.end().line() + 1, range.end().character() + 1);
    if (newTextIsSnippet) {
        manipulator.replace(start, end - start, {});
        manipulator.insertCodeSnippet(start, edit.newText(), &parseSnippet);
    } else {
        manipulator.replace(start, end - start, edit.newText());
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

    ClientExtras *extras = widget->findChild<ClientExtras *>(clientExtrasName,
                                                             Qt::FindDirectChildrenOnly);
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
            for (auto client : LanguageClientManager::clientsSupportingDocument(document)) {
                auto action = clientsGroup->addAction(client->name());
                auto reopen = [action, client = QPointer(client), document] {
                    if (!client)
                        return;
                    LanguageClientManager::openDocumentWithClient(document, client);
                    action->setChecked(true);
                };
                action->setCheckable(true);
                action->setChecked(client == LanguageClientManager::clientForDocument(document));
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

    if (std::holds_alternative<TextDocumentEdit>(change)) {
        return applyTextDocumentEdit(client, std::get<TextDocumentEdit>(change));
    } else if (std::holds_alternative<CreateFileOperation>(change)) {
        const auto createOperation = std::get<CreateFileOperation>(change);
        const FilePath filePath = createOperation.uri().toFilePath(client->hostPathMapper());
        if (filePath.exists()) {
            if (const std::optional<CreateFileOptions> options = createOperation.options()) {
                if (options->overwrite().value_or(false)) {
                    if (!filePath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return filePath.ensureExistingFile();
    } else if (std::holds_alternative<RenameFileOperation>(change)) {
        const RenameFileOperation renameOperation = std::get<RenameFileOperation>(change);
        const FilePath oldPath = renameOperation.oldUri().toFilePath(client->hostPathMapper());
        if (!oldPath.exists())
            return false;
        const FilePath newPath = renameOperation.newUri().toFilePath(client->hostPathMapper());
        if (oldPath == newPath)
            return true;
        if (newPath.exists()) {
            if (const std::optional<CreateFileOptions> options = renameOperation.options()) {
                if (options->overwrite().value_or(false)) {
                    if (!newPath.removeFile())
                        return false;
                } else if (options->ignoreIfExists().value_or(false)) {
                    return true;
                }
            }
        }
        return oldPath.renameFile(newPath);
    } else if (std::holds_alternative<DeleteFileOperation>(change)) {
        const auto deleteOperation = std::get<DeleteFileOperation>(change);
        const FilePath filePath = deleteOperation.uri().toFilePath(client->hostPathMapper());
        if (const std::optional<DeleteFileOptions> options = deleteOperation.options()) {
            if (!filePath.exists())
                return options->ignoreIfNotExists().value_or(false);
            if (filePath.isDir() && options->recursive().value_or(false))
                return filePath.removeRecursively();
        }
        return filePath.removeFile();
    }
    return false;
}

} // namespace LanguageClient
