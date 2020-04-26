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
#include "languageclientutils.h"

#include <coreplugin/find/itemviewfind.h>
#include <coreplugin/editormanager/ieditor.h>
#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <utils/itemviews.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>
#include <utils/utilsicons.h>

#include <QBoxLayout>

using namespace LanguageServerProtocol;

namespace LanguageClient {

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
    void handleResponse(const DocumentUri &uri, const DocumentSymbolsResult &response);
    void updateTextCursor(const QModelIndex &proxyIndex);
    void updateSelectionInTree(const QTextCursor &currentCursor);
    void onItemActivated(const QModelIndex &index);

    QPointer<Client> m_client;
    QPointer<TextEditor::BaseTextEditor> m_editor;
    LanguageClientOutlineModel m_model;
    Utils::TreeView m_view;
    DocumentUri m_uri;
    bool m_sync = false;
};

LanguageClientOutlineWidget::LanguageClientOutlineWidget(Client *client,
                                                         TextEditor::BaseTextEditor *editor)
    : m_client(client)
    , m_editor(editor)
    , m_view(this)
    , m_uri(DocumentUri::fromFilePath(editor->textDocument()->filePath()))
{
    connect(client->documentSymbolCache(),
            &DocumentSymbolCache::gotSymbols,
            this,
            &LanguageClientOutlineWidget::handleResponse);
    connect(editor->textDocument(), &TextEditor::TextDocument::contentsChanged, this, [this]() {
        if (m_client)
            m_client->documentSymbolCache()->requestSymbols(m_uri);
    });

    client->documentSymbolCache()->requestSymbols(m_uri);

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(&m_view));
    setLayout(layout);
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

void LanguageClientOutlineWidget::handleResponse(const DocumentUri &uri,
                                                 const DocumentSymbolsResult &result)
{
    if (uri != m_uri)
        return;
    if (Utils::holds_alternative<QList<SymbolInformation>>(result))
        m_model.setInfo(Utils::get<QList<SymbolInformation>>(result));
    else if (Utils::holds_alternative<QList<DocumentSymbol>>(result))
        m_model.setInfo(Utils::get<QList<DocumentSymbol>>(result));
    else
        m_model.clear();

    // The list has changed, update the current items
    updateSelectionInTree(m_editor->textCursor());
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
    if (!selection.isEmpty())
        m_view.scrollTo(selection.indexes().first());
}

void LanguageClientOutlineWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid() || !m_editor)
        return;

    updateTextCursor(index);
    m_editor->widget()->setFocus();
}

bool LanguageClientOutlineWidgetFactory::clientSupportsDocumentSymbols(
    const Client *client, const TextEditor::TextDocument *doc)
{
    if (!client)
        return false;
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
    return clientSupportsDocumentSymbols(LanguageClientManager::clientForDocument(doc), doc);
}

TextEditor::IOutlineWidget *LanguageClientOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QTC_ASSERT(textEditor, return nullptr);
    Client *client = LanguageClientManager::clientForDocument(textEditor->textDocument());
    if (!client || !clientSupportsDocumentSymbols(client, textEditor->textDocument()))
        return nullptr;
    return new LanguageClientOutlineWidget(client, textEditor);
}

class OutlineComboBox : public Utils::TreeViewComboBox
{
public:
    OutlineComboBox(Client *client, TextEditor::BaseTextEditor *editor);

private:
    void updateModel(const DocumentUri &resultUri, const DocumentSymbolsResult &result);
    void updateEntry();
    void activateEntry();
    void requestSymbols();

    LanguageClientOutlineModel m_model;
    QPointer<Client> m_client;
    TextEditor::TextEditorWidget *m_editorWidget;
    const DocumentUri m_uri;
};

Utils::TreeViewComboBox *LanguageClientOutlineWidgetFactory::createComboBox(Client *client,
                                                                            Core::IEditor *editor)
{
    auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QTC_ASSERT(textEditor, return nullptr);
    TextEditor::TextDocument *document = textEditor->textDocument();
    if (!client || !clientSupportsDocumentSymbols(client, document))
        return nullptr;

    return new OutlineComboBox(client, textEditor);
}

OutlineComboBox::OutlineComboBox(Client *client, TextEditor::BaseTextEditor *editor)
    : m_client(client)
    , m_editorWidget(editor->editorWidget())
    , m_uri(DocumentUri::fromFilePath(editor->document()->filePath()))
{
    setModel(&m_model);
    setMinimumContentsLength(13);
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    setSizePolicy(policy);
    setMaxVisibleItems(40);

    connect(client->documentSymbolCache(), &DocumentSymbolCache::gotSymbols,
            this, &OutlineComboBox::updateModel);
    connect(editor->textDocument(), &TextEditor::TextDocument::contentsChanged,
            this, &OutlineComboBox::requestSymbols);
    connect(m_editorWidget, &TextEditor::TextEditorWidget::cursorPositionChanged,
            this, &OutlineComboBox::updateEntry);
    connect(this, QOverload<int>::of(&QComboBox::activated), this, &OutlineComboBox::activateEntry);

    requestSymbols();
}

void OutlineComboBox::updateModel(const DocumentUri &resultUri, const DocumentSymbolsResult &result)
{
    if (m_uri != resultUri)
        return;
    if (Utils::holds_alternative<QList<SymbolInformation>>(result))
        m_model.setInfo(Utils::get<QList<SymbolInformation>>(result));
    else if (Utils::holds_alternative<QList<DocumentSymbol>>(result))
        m_model.setInfo(Utils::get<QList<DocumentSymbol>>(result));
    else
        m_model.clear();

    // The list has changed, update the current item
    updateEntry();
}

void OutlineComboBox::updateEntry()
{
    const Position pos(m_editorWidget->textCursor());
    LanguageClientOutlineItem *itemForCursor = m_model.findNonRootItem(
        [&](const LanguageClientOutlineItem *item) { return item->contains(pos); });
    if (itemForCursor)
        setCurrentIndex(m_model.indexForItem(itemForCursor));
}

void OutlineComboBox::activateEntry()
{
    const QModelIndex modelIndex = view()->currentIndex();
    if (modelIndex.isValid()) {
        const Position &pos = m_model.itemForIndex(modelIndex)->pos();
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();
        // line has to be 1 based, column 0 based!
        m_editorWidget->gotoLine(pos.line() + 1, pos.character(), true, true);
        emit m_editorWidget->activateEditor();
    }
}

void OutlineComboBox::requestSymbols()
{
    if (m_client)
        m_client->documentSymbolCache()->requestSymbols(m_uri);
}

} // namespace LanguageClient
