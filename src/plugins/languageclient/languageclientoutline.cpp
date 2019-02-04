/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "languageclientoutline.h"

#include "languageclientmanager.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/editormanager/ieditor.h>
#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/itemviews.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QBoxLayout>

using namespace LanguageServerProtocol;

namespace LanguageClient {

static const QIcon symbolIcon(int type)
{
    using namespace Utils::CodeModelIcon;
    static QMap<SymbolKind, QIcon> icons;
    if (type < int(SymbolKind::FirstSymbolKind) || type > int(SymbolKind::LastSymbolKind))
        return {};
    auto kind = static_cast<SymbolKind>(type);
    if (icons.contains(kind)) {
        switch (kind) {
        case SymbolKind::File: icons[kind] = Utils::Icons::NEWFILE.icon(); break;
        case SymbolKind::Module: icons[kind] = iconForType(Namespace); break;
        case SymbolKind::Namespace: icons[kind] = iconForType(Namespace); break;
        case SymbolKind::Package: icons[kind] = iconForType(Namespace); break;
        case SymbolKind::Class: icons[kind] = iconForType(Class); break;
        case SymbolKind::Method: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::Property: icons[kind] = iconForType(Property); break;
        case SymbolKind::Field: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Constructor: icons[kind] = iconForType(Class); break;
        case SymbolKind::Enum: icons[kind] = iconForType(Enum); break;
        case SymbolKind::Interface: icons[kind] = iconForType(Class); break;
        case SymbolKind::Function: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::Variable: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Constant: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::String: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Number: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Boolean: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Array: icons[kind] = iconForType(VarPublic); break;
        case SymbolKind::Object: icons[kind] = iconForType(Class); break;
        case SymbolKind::Key: icons[kind] = iconForType(Keyword); break;
        case SymbolKind::Null: icons[kind] = iconForType(Keyword); break;
        case SymbolKind::EnumMember: icons[kind] = iconForType(Enumerator); break;
        case SymbolKind::Struct: icons[kind] = iconForType(Struct); break;
        case SymbolKind::Event: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::Operator: icons[kind] = iconForType(FuncPublic); break;
        case SymbolKind::TypeParameter: icons[kind] = iconForType(VarPublic); break;
        }
    }
    return icons[kind];
}

class LanguageClientOutlineItem : public Utils::TypedTreeItem<LanguageClientOutlineItem>
{
public:
    LanguageClientOutlineItem() = default;
    LanguageClientOutlineItem(const SymbolInformation &info)
        : m_name(info.name())
        , m_range(info.location().range())
        , m_type(info.kind())
    { }

    LanguageClientOutlineItem(const DocumentSymbol &info)
        : m_name(info.name())
        , m_detail(info.detail().value_or(QString()))
        , m_range(info.range())
        , m_type(info.kind())
    {
        for (const DocumentSymbol &child : info.children().value_or(QList<DocumentSymbol>()))
            appendChild(new LanguageClientOutlineItem(child));
    }

    // TreeItem interface
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            return symbolIcon(m_type);
        case Qt::DisplayRole:
            return m_name;
        default:
            return Utils::TreeItem::data(column, role);
        }
    }

    Position pos() const { return m_range.start(); }
    bool contains(const Position &pos) const { return m_range.contains(pos); }

private:
    QString m_name;
    QString m_detail;
    Range m_range;
    int m_type = -1;
};

class LanguageClientOutlineModel : public Utils::TreeModel<LanguageClientOutlineItem>
{
public:
    using Utils::TreeModel<LanguageClientOutlineItem>::TreeModel;
    void setInfo(const QList<SymbolInformation> &info)
    {
        clear();
        for (const SymbolInformation &symbol : info)
            rootItem()->appendChild(new LanguageClientOutlineItem(symbol));
    }
    void setInfo(const QList<DocumentSymbol> &info)
    {
        clear();
        for (const DocumentSymbol &symbol : info)
            rootItem()->appendChild(new LanguageClientOutlineItem(symbol));
    }
};

class LanguageClientOutlineWidget : public TextEditor::IOutlineWidget
{
public:
    LanguageClientOutlineWidget(Client *client, TextEditor::BaseTextEditor *editor);

    // IOutlineWidget interface
public:
    QList<QAction *> filterMenuActions() const override;
    void setCursorSynchronization(bool syncWithCursor) override;

private:
    void handleResponse(const LanguageServerProtocol::DocumentSymbolsRequest::Response &response);
    void updateTextCursor(const QModelIndex &proxyIndex);
    void updateSelectionInTree(const QTextCursor &currentCursor);
    void onItemActivated(const QModelIndex &index);

    QPointer<Client> m_client;
    QPointer<TextEditor::BaseTextEditor> m_editor;
    LanguageClientOutlineModel m_model;
    Utils::TreeView m_view;
    bool m_sync = false;
};

LanguageClientOutlineWidget::LanguageClientOutlineWidget(Client *client,
                                                         TextEditor::BaseTextEditor *editor)
    : m_client(client)
    , m_editor(editor)
    , m_view(this)
{
    const DocumentSymbolParams params(
                TextDocumentIdentifier(
                    DocumentUri::fromFileName(editor->textDocument()->filePath())));
    DocumentSymbolsRequest request(params);
    request.setResponseCallback([self = QPointer<LanguageClientOutlineWidget>(this)]
                                (const DocumentSymbolsRequest::Response &response){
                                    if (self)
                                        self->handleResponse(response);
    });

    auto *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(&m_view));
    setLayout(layout);
    client->sendContent(request);
    m_view.setModel(&m_model);
    m_view.setHeaderHidden(true);
    connect(&m_view, &QAbstractItemView::activated,
            this, &LanguageClientOutlineWidget::onItemActivated);
    connect(m_editor->editorWidget(), &TextEditor::TextEditorWidget::cursorPositionChanged,
            this, [this](){
        if (m_sync)
            updateSelectionInTree(m_editor->textCursor());
    });
}

QList<QAction *> LanguageClientOutlineWidget::filterMenuActions() const
{
    return {};
}

void LanguageClientOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_sync = syncWithCursor;
    if (m_sync && m_editor)
        updateSelectionInTree(m_editor->textCursor());
}

void LanguageClientOutlineWidget::handleResponse(const DocumentSymbolsRequest::Response &response)
{
    if (Utils::optional<DocumentSymbolsRequest::Response::Error> error = response.error()) {
        if (m_client)
            m_client->log(error.value());
    }
    if (Utils::optional<DocumentSymbolsResult> result = response.result()) {
        if (Utils::holds_alternative<QList<SymbolInformation>>(result.value()))
            m_model.setInfo(Utils::get<QList<SymbolInformation>>(result.value()));
        else if (Utils::holds_alternative<QList<DocumentSymbol>>(result.value()))
            m_model.setInfo(Utils::get<QList<DocumentSymbol>>(result.value()));
        else
            m_model.clear();
    }
}

void LanguageClientOutlineWidget::updateTextCursor(const QModelIndex &proxyIndex)
{
    LanguageClientOutlineItem *item = m_model.itemForIndex(proxyIndex);
    const Position &pos = item->pos();
    // line has to be 1 based, column 0 based!
    m_editor->editorWidget()->gotoLine(pos.line() + 1, pos.character(), true, true);
}

void LanguageClientOutlineWidget::updateSelectionInTree(const QTextCursor &currentCursor)
{
    QItemSelection selection;
    const Position pos(currentCursor);
    m_model.forAllItems([&](const LanguageClientOutlineItem *item) {
        if (item->contains(pos))
            selection.select(m_model.indexForItem(item), m_model.indexForItem(item));
    });
    m_view.selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
}

void LanguageClientOutlineWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid() || !m_editor)
        return;

    updateTextCursor(index);
    m_editor->widget()->setFocus();
}

static bool clientSupportsDocumentSymbols(const Client *client, const TextEditor::TextDocument *doc)
{
    DynamicCapabilities dc = client->dynamicCapabilities();
    if (dc.isRegistered(DocumentSymbolsRequest::methodName).value_or(false)) {
        TextDocumentRegistrationOptions options(dc.option(DocumentSymbolsRequest::methodName));
        return !options.isValid(nullptr)
               || options.filterApplies(doc->filePath(), Utils::mimeTypeForName(doc->mimeType()));
    }
    return client->capabilities().documentSymbolProvider().value_or(false);
}

bool LanguageClientOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
    if (!doc)
        return false;
    auto clients = LanguageClientManager::clientsSupportingDocument(doc);
    return Utils::anyOf(clients, [doc](const Client *client){
        return clientSupportsDocumentSymbols(client, doc);
    });
}

TextEditor::IOutlineWidget *LanguageClientOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QTC_ASSERT(textEditor, return nullptr);
    QList<Client *> clients = LanguageClientManager::clientsSupportingDocument(textEditor->textDocument());
    QTC_ASSERT(!clients.isEmpty(), return nullptr);
    clients = Utils::filtered(clients, [doc = textEditor->textDocument()](const Client *client){
        return clientSupportsDocumentSymbols(client, doc);
    });
    return new LanguageClientOutlineWidget(clients.first(), textEditor);
}

} // namespace LanguageClient
