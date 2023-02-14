// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "languageclientoutline.h"

#include "documentsymbolcache.h"
#include "languageclientmanager.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/find/itemviewfind.h>
#include <languageserverprotocol/languagefeatures.h>
#include <texteditor/outlinefactory.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditortr.h>
#include <utils/dropsupport.h>
#include <utils/itemviews.h>
#include <utils/navigationtreeview.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QBoxLayout>
#include <QSortFilterProxyModel>

using namespace LanguageServerProtocol;

namespace LanguageClient {

const QList<SymbolInformation> sortedSymbols(const QList<SymbolInformation> &symbols)
{
    return Utils::sorted(symbols, [](const SymbolInformation &a, const SymbolInformation &b){
        return a.location().range().start() < b.location().range().start();
    });
}
const QList<DocumentSymbol> sortedSymbols(const QList<DocumentSymbol> &symbols)
{
    return Utils::sorted(symbols, [](const DocumentSymbol &a, const DocumentSymbol &b){
        return a.range().start() < b.range().start();
    });
}

class LanguageClientOutlineModel : public Utils::TreeModel<LanguageClientOutlineItem>
{
public:
    LanguageClientOutlineModel(Client *client) : m_client(client)  {}
    void setFilePath(const Utils::FilePath &filePath) { m_filePath = filePath; }

    void setInfo(const QList<SymbolInformation> &info)
    {
        clear();
        for (const SymbolInformation &symbol : sortedSymbols(info))
            rootItem()->appendChild(new LanguageClientOutlineItem(symbol));
    }
    void setInfo(const QList<DocumentSymbol> &info)
    {
        clear();
        for (const DocumentSymbol &symbol : sortedSymbols(info))
            rootItem()->appendChild(m_client->createOutlineItem(symbol));
    }

    Qt::DropActions supportedDragActions() const override
    {
        return Qt::MoveAction;
    }

    QStringList mimeTypes() const override
    {
        return Utils::DropSupport::mimeTypesForFilePaths();
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        auto mimeData = new Utils::DropMimeData;
        for (const QModelIndex &index : indexes) {
            if (LanguageClientOutlineItem *item = itemForIndex(index)) {
                const LanguageServerProtocol::Position pos = item->pos();
                mimeData->addFile(m_filePath, pos.line() + 1, pos.character());
            }
        }
        return mimeData;
    }

private:
    Client * const m_client;
    Utils::FilePath m_filePath;
};

class DragSortFilterProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    Qt::DropActions supportedDragActions() const override
    {
        return sourceModel()->supportedDragActions();
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
    void setSorted(bool) override;
    bool isSorted() const override;
    void restoreSettings(const QVariantMap &map) override;
    QVariantMap settings() const override;

private:
    void handleResponse(const DocumentUri &uri, const DocumentSymbolsResult &response);
    void updateTextCursor(const QModelIndex &proxyIndex);
    void updateSelectionInTree(const QTextCursor &currentCursor);
    void onItemActivated(const QModelIndex &index);

    QPointer<Client> m_client;
    QPointer<TextEditor::BaseTextEditor> m_editor;
    LanguageClientOutlineModel m_model;
    DragSortFilterProxyModel m_proxyModel;
    Utils::NavigationTreeView m_view;
    DocumentUri m_uri;
    bool m_sync = false;
    bool m_sorted = false;
};

LanguageClientOutlineWidget::LanguageClientOutlineWidget(Client *client,
                                                         TextEditor::BaseTextEditor *editor)
    : m_client(client)
    , m_editor(editor)
    , m_model(client)
    , m_view(this)
    , m_uri(m_client->hostPathToServerUri(editor->textDocument()->filePath()))
{
    connect(client->documentSymbolCache(),
            &DocumentSymbolCache::gotSymbols,
            this,
            &LanguageClientOutlineWidget::handleResponse);
    connect(client, &Client::documentUpdated, this, [this](TextEditor::TextDocument *document) {
        if (m_client && m_uri == m_client->hostPathToServerUri(document->filePath()))
            m_client->documentSymbolCache()->requestSymbols(m_uri, Schedule::Delayed);
    });

    client->documentSymbolCache()->requestSymbols(m_uri, Schedule::Delayed);

    auto *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(Core::ItemViewFind::createSearchableWrapper(&m_view));
    setLayout(layout);
    m_model.setFilePath(editor->textDocument()->filePath());
    m_proxyModel.setSourceModel(&m_model);
    m_view.setModel(&m_proxyModel);
    m_view.setHeaderHidden(true);
    m_view.setExpandsOnDoubleClick(false);
    m_view.setFrameStyle(QFrame::NoFrame);
    m_view.setDragEnabled(true);
    m_view.setDragDropMode(QAbstractItemView::DragOnly);
    connect(&m_view, &QAbstractItemView::activated,
            this, &LanguageClientOutlineWidget::onItemActivated);
    connect(m_editor->editorWidget(), &TextEditor::TextEditorWidget::cursorPositionChanged,
            this, [this](){
        if (m_sync)
            updateSelectionInTree(m_editor->textCursor());
    });
    setFocusProxy(&m_view);
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

void LanguageClientOutlineWidget::setSorted(bool sorted)
{
    m_sorted = sorted;
    m_proxyModel.sort(sorted ? 0 : -1);
}

bool LanguageClientOutlineWidget::isSorted() const
{
    return m_sorted;
}

void LanguageClientOutlineWidget::restoreSettings(const QVariantMap &map)
{
    setSorted(map.value(QString("LspOutline.Sort"), false).toBool());
}

QVariantMap LanguageClientOutlineWidget::settings() const
{
    return {{QString("LspOutline.Sort"), m_sorted}};
}

void LanguageClientOutlineWidget::handleResponse(const DocumentUri &uri,
                                                 const DocumentSymbolsResult &result)
{
    if (uri != m_uri)
        return;
    if (std::holds_alternative<QList<SymbolInformation>>(result))
        m_model.setInfo(std::get<QList<SymbolInformation>>(result));
    else if (std::holds_alternative<QList<DocumentSymbol>>(result))
        m_model.setInfo(std::get<QList<DocumentSymbol>>(result));
    else
        m_model.clear();

    // The list has changed, update the current items
    updateSelectionInTree(m_editor->textCursor());
}

void LanguageClientOutlineWidget::updateTextCursor(const QModelIndex &proxyIndex)
{
    LanguageClientOutlineItem *item = m_model.itemForIndex(m_proxyModel.mapToSource(proxyIndex));
    const Position &pos = item->pos();
    // line has to be 1 based, column 0 based!
    m_editor->editorWidget()->gotoLine(pos.line() + 1, pos.character(), true, true);
}

static LanguageClientOutlineItem *itemForCursor(const LanguageClientOutlineModel &m_model,
                                                const QTextCursor &cursor)
{
    const Position pos(cursor);
    LanguageClientOutlineItem *result = nullptr;
    m_model.forAllItems([&](LanguageClientOutlineItem *candidate){
        if (!candidate->contains(pos))
            return;
        if (result && candidate->range().contains(result->range()))
            return; // skip item if the range is equal or bigger than the previous found range
        result = candidate;
    });
    return result;
}

void LanguageClientOutlineWidget::updateSelectionInTree(const QTextCursor &currentCursor)
{
    if (LanguageClientOutlineItem *item = itemForCursor(m_model, currentCursor)) {
        const QModelIndex index = m_proxyModel.mapFromSource(m_model.indexForItem(item));
        m_view.setCurrentIndex(index);
        m_view.scrollTo(index);
    } else {
        m_view.clearSelection();
    }
}

void LanguageClientOutlineWidget::onItemActivated(const QModelIndex &index)
{
    if (!index.isValid() || !m_editor)
        return;

    updateTextCursor(index);
    m_editor->widget()->setFocus();
}

bool LanguageClientOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document())) {
        if (Client *client = LanguageClientManager::clientForDocument(doc))
            return client->supportsDocumentSymbols(doc);
    }
    return false;
}

TextEditor::IOutlineWidget *LanguageClientOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QTC_ASSERT(textEditor, return nullptr);
    if (Client *client = LanguageClientManager::clientForDocument(textEditor->textDocument())) {
        if (client->supportsDocumentSymbols(textEditor->textDocument()))
            return new LanguageClientOutlineWidget(client, textEditor);
    }
    return nullptr;
}

class OutlineComboBox : public Utils::TreeViewComboBox
{
public:
    OutlineComboBox(Client *client, TextEditor::BaseTextEditor *editor);

private:
    void updateModel(const DocumentUri &resultUri, const DocumentSymbolsResult &result);
    void updateEntry();
    void activateEntry();
    void documentUpdated(TextEditor::TextDocument *document);
    void setSorted(bool sorted);

    LanguageClientOutlineModel m_model;
    QSortFilterProxyModel m_proxyModel;
    QPointer<Client> m_client;
    TextEditor::TextEditorWidget *m_editorWidget;
    const DocumentUri m_uri;
};

Utils::TreeViewComboBox *LanguageClientOutlineWidgetFactory::createComboBox(
    Client *client, TextEditor::BaseTextEditor *editor)
{
    if (client && client->supportsDocumentSymbols(editor->textDocument()))
        return new OutlineComboBox(client, editor);
    return nullptr;
}

OutlineComboBox::OutlineComboBox(Client *client, TextEditor::BaseTextEditor *editor)
    : m_model(client)
    , m_client(client)
    , m_editorWidget(editor->editorWidget())
    , m_uri(m_client->hostPathToServerUri(editor->document()->filePath()))
{
    m_proxyModel.setSourceModel(&m_model);
    const bool sorted = LanguageClientSettings::outlineComboBoxIsSorted();
    m_proxyModel.sort(sorted ? 0 : -1);
    setModel(&m_proxyModel);
    setMinimumContentsLength(13);
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    setSizePolicy(policy);
    setMaxVisibleItems(40);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    const QString sortActionText = ::TextEditor::Tr::tr("Sort Alphabetically");
    auto sortAction = new QAction(sortActionText, this);
    sortAction->setCheckable(true);
    sortAction->setChecked(sorted);
    addAction(sortAction);

    connect(client->documentSymbolCache(),
            &DocumentSymbolCache::gotSymbols,
            this,
            &OutlineComboBox::updateModel);
    connect(client, &Client::documentUpdated, this, &OutlineComboBox::documentUpdated);
    connect(m_editorWidget, &TextEditor::TextEditorWidget::cursorPositionChanged,
            this, &OutlineComboBox::updateEntry);
    connect(this, &QComboBox::activated, this, &OutlineComboBox::activateEntry);
    connect(sortAction, &QAction::toggled, this, &OutlineComboBox::setSorted);

    documentUpdated(editor->textDocument());
}

void OutlineComboBox::updateModel(const DocumentUri &resultUri, const DocumentSymbolsResult &result)
{
    if (m_uri != resultUri)
        return;
    if (std::holds_alternative<QList<SymbolInformation>>(result))
        m_model.setInfo(std::get<QList<SymbolInformation>>(result));
    else if (std::holds_alternative<QList<DocumentSymbol>>(result))
        m_model.setInfo(std::get<QList<DocumentSymbol>>(result));
    else
        m_model.clear();

    view()->expandAll();
    // The list has changed, update the current item
    updateEntry();
}

void OutlineComboBox::updateEntry()
{
    if (LanguageClientOutlineItem *item = itemForCursor(m_model, m_editorWidget->textCursor()))
        setCurrentIndex(m_proxyModel.mapFromSource(m_model.indexForItem(item)));
}

void OutlineComboBox::activateEntry()
{
    const QModelIndex modelIndex = m_proxyModel.mapToSource(view()->currentIndex());
    if (modelIndex.isValid()) {
        const Position &pos = m_model.itemForIndex(modelIndex)->pos();
        Core::EditorManager::cutForwardNavigationHistory();
        Core::EditorManager::addCurrentPositionToNavigationHistory();
        // line has to be 1 based, column 0 based!
        m_editorWidget->gotoLine(pos.line() + 1, pos.character(), true, true);
        emit m_editorWidget->activateEditor();
    }
}

void OutlineComboBox::documentUpdated(TextEditor::TextDocument *document)
{
    if (document == m_editorWidget->textDocument())
        m_client->documentSymbolCache()->requestSymbols(m_uri, Schedule::Delayed);
}

void OutlineComboBox::setSorted(bool sorted)
{
    LanguageClientSettings::setOutlineComboBoxSorted(sorted);
    m_proxyModel.sort(sorted ? 0 : -1);
}

LanguageClientOutlineItem::LanguageClientOutlineItem(const SymbolInformation &info)
    : m_name(info.name())
    , m_range(info.location().range())
    , m_type(info.kind())
{ }

LanguageClientOutlineItem::LanguageClientOutlineItem(Client *client, const DocumentSymbol &info)
    : m_client(client)
    , m_name(info.name())
    , m_detail(info.detail().value_or(QString()))
    , m_range(info.range())
    , m_selectionRange(info.selectionRange())
    , m_type(info.kind())
{
    const QList<LanguageServerProtocol::DocumentSymbol> children = sortedSymbols(
        info.children().value_or(QList<DocumentSymbol>()));
    for (const DocumentSymbol &child : children)
        appendChild(m_client->createOutlineItem(child));
}

QVariant LanguageClientOutlineItem::data(int column, int role) const
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
Qt::ItemFlags LanguageClientOutlineItem::flags(int column) const
{
    Q_UNUSED(column)
    return Utils::TypedTreeItem<LanguageClientOutlineItem>::flags(column) | Qt::ItemIsDragEnabled;
}
} // namespace LanguageClient
