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

bool applyTextDocumentEdit(const TextDocumentEdit &edit)
{
    const QList<TextEdit> &edits = edit.edits();
    if (edits.isEmpty())
        return true;
    const DocumentUri &uri = edit.textDocument().uri();
    if (TextDocument* doc = TextDocument::textDocumentForFilePath(uri.toFilePath())) {
        LanguageClientValue<int> version = edit.textDocument().version();
        if (!version.isNull() && version.value(0) < doc->document()->revision())
            return false;
    }
    return applyTextEdits(uri, edits);
}

bool applyTextEdits(const DocumentUri &uri, const QList<TextEdit> &edits)
{
    if (edits.isEmpty())
        return true;
    RefactoringChanges changes;
    RefactoringFilePtr file;
    file = changes.file(uri.toFilePath().toString());
    file->setChangeSet(editsToChangeSet(edits, file->document()));
    return file->apply();
}

void applyTextEdit(TextDocumentManipulatorInterface &manipulator, const TextEdit &edit)
{
    using namespace Utils::Text;
    const Range range = edit.range();
    const QTextDocument *doc = manipulator.textCursorAt(manipulator.currentPosition()).document();
    const int start = positionInText(doc, range.start().line() + 1, range.start().character() + 1);
    const int end = positionInText(doc, range.end().line() + 1, range.end().character() + 1);
    manipulator.replace(start, end - start, edit.newText());
}

bool applyWorkspaceEdit(const WorkspaceEdit &edit)
{
    bool result = true;
    const QList<TextDocumentEdit> &documentChanges
        = edit.documentChanges().value_or(QList<TextDocumentEdit>());
    if (!documentChanges.isEmpty()) {
        for (const TextDocumentEdit &documentChange : documentChanges)
            result |= applyTextDocumentEdit(documentChange);
    } else {
        const WorkspaceEdit::Changes &changes = edit.changes().value_or(WorkspaceEdit::Changes());
        for (const DocumentUri &file : changes.keys())
            result |= applyTextEdits(file, changes.value(file));
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
    if (action.isValid(nullptr))
        marker.tooltip = action.title();
    if (action.edit().has_value()) {
        WorkspaceEdit edit = action.edit().value();
        marker.callback = [edit](const TextEditorWidget *) {
            applyWorkspaceEdit(edit);
        };
        if (diagnostics.isEmpty()) {
            QList<TextEdit> edits;
            if (optional<QList<TextDocumentEdit>> documentChanges = edit.documentChanges()) {
                QList<TextDocumentEdit> changesForUri = Utils::filtered(
                    documentChanges.value(), [uri](const TextDocumentEdit &edit) {
                    return edit.textDocument().uri() == uri;
                });
                for (const TextDocumentEdit &edit : changesForUri)
                    edits << edit.edits();
            } else if (optional<WorkspaceEdit::Changes> localChanges = edit.changes()) {
                edits = localChanges.value()[uri];
            }
            for (const TextEdit &edit : edits) {
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

    static QMap<QWidget *, QAction *> actions;

    if (actions.contains(widget)) {
        auto action = actions[widget];
        if (client) {
            action->setText(client->name());
        } else {
            widget->toolBar()->removeAction(action);
            actions.remove(widget);
            delete action;
        }
    } else if (client) {
        const QIcon icon = Utils::Icon({{":/languageclient/images/languageclient.png",
                                         Utils::Theme::IconsBaseColor}})
                               .icon();
        actions[widget] = widget->toolBar()->addAction(
            icon, client->name(), [document]() {
                auto menu = new QMenu;
                auto clientsGroup = new QActionGroup(menu);
                clientsGroup->setExclusive(true);
                for (auto client : LanguageClientManager::clientsSupportingDocument(document)) {
                    auto action = clientsGroup->addAction(client->name());
                    auto reopen = [action, client = QPointer<Client>(client), document]() {
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
                menu->addAction("Language Client Logs", []() {
                    LanguageClientManager::showLogger();
                });
                menu->addAction("Manage...", []() {
                    Core::ICore::showOptionsDialog(Constants::LANGUAGECLIENT_SETTINGS_PAGE);
                });
                menu->popup(QCursor::pos());
            });
        QObject::connect(widget, &QWidget::destroyed, [widget]() {
            actions.remove(widget);
        });
    }

    static QMap<QWidget *, QPair<Client *, QAction *>> outlines;

    if (outlines.contains(widget)) {
        auto outline = outlines[widget];
        if (outline.first != client
            || !LanguageClientOutlineWidgetFactory::clientSupportsDocumentSymbols(client,
                                                                                  document)) {
            auto oldAction = outline.second;
            widget->toolBar()->removeAction(oldAction);
            delete oldAction;
            outlines.remove(widget);
        }
    }

    if (!outlines.contains(widget)) {
        if (QWidget *comboBox = LanguageClientOutlineWidgetFactory::createComboBox(client, editor)) {
            outlines[widget] = {client,
                                widget->insertExtraToolBarWidget(TextEditorWidget::Left, comboBox)};
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
