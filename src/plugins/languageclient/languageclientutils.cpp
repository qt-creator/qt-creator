/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "languageclientutils.h"

#include "client.h"
#include "languageclient_global.h"
#include "languageclientmanager.h"
#include "languageclientoutline.h"
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
    return ChangeSet::Range(
        Text::positionInText(doc, range.start().line() + 1, range.start().character() + 1),
        Text::positionInText(doc, range.end().line() + 1, range.end().character()) + 1);
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
    const FilePath &filePath = uri.toFilePath();
    LanguageClientValue<int> version = edit.textDocument().version();
    if (!version.isNull() && version.value(0) < client->documentVersion(filePath))
        return false;
    return applyTextEdits(client, uri, edits);
}

bool applyTextEdits(const Client *client, const DocumentUri &uri, const QList<TextEdit> &edits)
{
    if (edits.isEmpty())
        return true;
    RefactoringChangesData * const backend = client->createRefactoringChangesBackend();
    RefactoringChanges changes(backend);
    RefactoringFilePtr file;
    file = changes.file(uri.toFilePath());
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
    using namespace Utils::Text;
    const Range range = edit.range();
    const QTextDocument *doc = manipulator.textCursorAt(manipulator.currentPosition()).document();
    const int start = positionInText(doc, range.start().line() + 1, range.start().character() + 1);
    const int end = positionInText(doc, range.end().line() + 1, range.end().character() + 1);
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
    const QList<TextDocumentEdit> &documentChanges
        = edit.documentChanges().value_or(QList<TextDocumentEdit>());
    if (!documentChanges.isEmpty()) {
        for (const TextDocumentEdit &documentChange : documentChanges)
            result |= applyTextDocumentEdit(client, documentChange);
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
                                       const CodeAction &action,
                                       const DocumentUri &uri)
{
    TextDocument* doc = TextDocument::textDocumentForFilePath(uri.toFilePath());
    if (!doc)
        return;
    const QVector<BaseTextEditor *> editors = BaseTextEditor::textEditorsForDocument(doc);
    if (editors.isEmpty())
        return;

    const QList<Diagnostic> &diagnostics = action.diagnostics().value_or(QList<Diagnostic>());

    RefactorMarkers markers;
    RefactorMarker marker;
    marker.type = client->id();
    if (action.isValid())
        marker.tooltip = action.title();
    if (Utils::optional<WorkspaceEdit> edit = action.edit()) {
        marker.callback = [client, edit](const TextEditorWidget *) {
            applyWorkspaceEdit(client, *edit);
        };
        if (diagnostics.isEmpty()) {
            QList<TextEdit> edits;
            if (optional<QList<TextDocumentEdit>> documentChanges = edit->documentChanges()) {
                QList<TextDocumentEdit> changesForUri = Utils::filtered(
                    *documentChanges, [uri](const TextDocumentEdit &edit) {
                    return edit.textDocument().uri() == uri;
                });
                for (const TextDocumentEdit &edit : changesForUri)
                    edits << edit.edits();
            } else if (optional<WorkspaceEdit::Changes> localChanges = edit->changes()) {
                edits = (*localChanges)[uri];
            }
            for (const TextEdit &edit : qAsConst(edits)) {
                marker.cursor = endOfLineCursor(edit.range().start().toTextCursor(doc->document()));
                markers << marker;
            }
        }
    } else if (action.command().has_value()) {
        const Command command = action.command().value();
        marker.callback = [command, client = QPointer<Client>(client)](const TextEditorWidget *) {
            if (client)
                client->executeCommand(command);
        };
    } else {
        return;
    }
    for (const Diagnostic &diagnostic : diagnostics) {
        marker.cursor = endOfLineCursor(diagnostic.range().start().toTextCursor(doc->document()));
        markers << marker;
    }
    for (BaseTextEditor *editor : editors) {
        if (TextEditorWidget *editorWidget = editor->editorWidget())
            editorWidget->setRefactorMarkers(markers + editorWidget->refactorMarkers());
    }
}

static const char clientExtrasName[] = "__qtcreator_client_extras__";

class ClientExtras : public QObject
{
public:
    ClientExtras(QObject *parent) : QObject(parent) { setObjectName(clientExtrasName); };

    QPointer<QAction> m_popupAction;
    QPointer<Client> m_client;
    QPointer<QAction> m_outlineAction;
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
                    icon, client->name(), [document = QPointer(document)] {
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
            menu->addAction("Inspect Language Clients", [] {
                LanguageClientManager::showInspector();
            });
            menu->addAction("Manage...", [] {
                Core::ICore::showOptionsDialog(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
            });
            menu->popup(QCursor::pos());
        });
    }

    if (!extras->m_client || !client || extras->m_client != client
        || !client->supportsDocumentSymbols(document)) {
        if (extras->m_outlineAction) {
            widget->toolBar()->removeAction(extras->m_outlineAction);
            delete extras->m_outlineAction;
        }
        extras->m_client.clear();
    }

    if (!extras->m_client) {
        QWidget *comboBox = LanguageClientOutlineWidgetFactory::createComboBox(client, textEditor);
        if (comboBox) {
            extras->m_client = client;
            extras->m_outlineAction = widget->insertExtraToolBarWidget(TextEditorWidget::Left,
                                                                       comboBox);
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

} // namespace LanguageClient
