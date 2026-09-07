// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "callandtypehierarchy.h"

#include "languageclientmanager.h"
#include "languageclienttr.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <languageserverprotocol/lspmessages.h>
#include <languageserverprotocol/lsputils.h>

#include <texteditor/texteditor.h>
#include <texteditor/typehierarchy.h>

#include <utils/delegates.h>
#include <utils/navigationtreeview.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QLayout>
#include <QMenu>
#include <QToolButton>

using namespace Core;
using namespace Utils;
using namespace TextEditor;
using namespace LanguageServerProtocol;

namespace LanguageClient {

namespace {
enum {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};
}

struct HierarchyPosition
{
    TextDocumentIdentifier textDocument;
    Position position;
};

static bool sorter(const TreeItem *a, const TreeItem *b)
{
    return a->data(0, Qt::DisplayRole).toString() < b->data(0, Qt::DisplayRole).toString();
}

template<class Item, class Request, class Result>
class HierarchyItem : public TreeItem
{
public:
    HierarchyItem(const Item &item, Client *client)
        : m_item(item)
        , m_client(client)
    {
        if (client) {
            const Position start = m_item.selectionRange().start();
            const FilePath path = client->filePathFor(m_item.uri());
            m_link = Link(path, start.line() + 1, start.character());
        }
    }

protected:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            if (hasTag(SymbolTag::Deprecated))
                return Utils::Icons::WARNING.icon();
            return symbolIcon(m_item.kind(), m_item.tags().value_or(QList<int>()));
        case Qt::DisplayRole:
            return m_item.name();
        case Qt::ToolTipRole:
            if (hasTag(SymbolTag::Deprecated))
                return Tr::tr("Deprecated");
            return {};
        case LinkRole:
            return QVariant::fromValue(m_link);
        case AnnotationRole: {
            QStringList result;
            if (const std::optional<QString> detail = m_item.detail())
                result << *detail;
            if (childCount() > 0)
                result << QString("[%1]").arg(childCount());
            return result.isEmpty() ? QVariant() : QVariant(result.join(' '));
        }
        default:
            return TreeItem::data(column, role);
        }
    }

private:
    bool canFetchMore() const override
    {
        if (m_client && !m_fetchedChildren)
            const_cast<HierarchyItem*>(this)->fetchMore();
        return false;
    }

    void fetchMore() override
    {
        m_fetchedChildren = true;
        if (!m_client)
            return;

        typename Request::Params params;
        params.item(m_item);
        m_client->template sendRequest<Request>(
            params, [this](const Utils::Result<typename Request::Result> &result) {
                if (!result)
                    return;
                if (const auto items = std::get_if<QList<Result>>(&*result)) {
                    for (const Result &item : *items)
                        insertOrderedChild(new HierarchyItem(getSourceItem(item), m_client), sorter);
                }
            });
    }

    Item getSourceItem(const Result &result)
    {
        if constexpr (std::is_same_v<Result, CallHierarchyIncomingCall>)
            return result.from();
        if constexpr (std::is_same_v<Result, CallHierarchyOutgoingCall>)
            return result.to();
        if constexpr (std::is_same_v<Result, TypeHierarchyItem>)
            return result;
    }

    bool hasTag(int tag) const
    {
        if (const std::optional<QList<int>> &tags = m_item.tags())
            return tags->contains(tag);
        return false;
    }

    const Item m_item;
    bool m_fetchedChildren = false;
    QPointer<Client> m_client;
    Link m_link;
};

class CallHierarchyIncomingItem
    : public HierarchyItem<CallHierarchyItem, CallHierarchyIncomingCallsRequest, CallHierarchyIncomingCall>
{
public:
    CallHierarchyIncomingItem(const CallHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Incoming");
        if (role == Qt::DecorationRole)
            return {};
        if (role == LinkRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class CallHierarchyOutgoingItem
    : public HierarchyItem<CallHierarchyItem, CallHierarchyOutgoingCallsRequest, CallHierarchyOutgoingCall>
{
public:
    CallHierarchyOutgoingItem(const CallHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Outgoing");
        if (role == Qt::DecorationRole)
            return {};
        if (role == LinkRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

template<class Item> class HierarchyRootItem : public TreeItem
{
public:
    HierarchyRootItem(const Item &item, Client *client)
        : m_item(item)
    {
        if (QTC_GUARD(client)) {
            const Position start = m_item.selectionRange().start();
            const FilePath path = client->filePathFor(m_item.uri());
            m_link = Link(path, start.line() + 1, start.character());
        }
    }

private:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            if (m_item.tags().value_or(QList<int>()).contains(SymbolTag::Deprecated))
                return Utils::Icons::WARNING.icon();
            return symbolIcon(m_item.kind(), m_item.tags().value_or(QList<int>()));
        case Qt::DisplayRole:
            return m_item.name();
        case LinkRole:
            return QVariant::fromValue(m_link);
        default:
            return TreeItem::data(column, role);
        }
    }

    const Item m_item;
    Link m_link;
};

class CallHierarchyRootItem : public HierarchyRootItem<CallHierarchyItem>
{
public:
    CallHierarchyRootItem(const CallHierarchyItem &item, Client *client)
        : HierarchyRootItem(item, client)
    {
        appendChild(new CallHierarchyIncomingItem(item, client));
        appendChild(new CallHierarchyOutgoingItem(item, client));
    }
};

class TypeHierarchyBasesItem
    : public HierarchyItem<TypeHierarchyItem, TypeHierarchySupertypesRequest, TypeHierarchyItem>
{
public:
    TypeHierarchyBasesItem(const TypeHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Bases");
        if (role == Qt::DecorationRole)
            return {};
        if (role == LinkRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class TypeHierarchyDerivedItem
    : public HierarchyItem<TypeHierarchyItem, TypeHierarchySubtypesRequest, TypeHierarchyItem>
{
public:
    TypeHierarchyDerivedItem(const TypeHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Derived");
        if (role == Qt::DecorationRole)
            return {};
        if (role == LinkRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class TypeHierarchyRootItem : public HierarchyRootItem<TypeHierarchyItem>
{
public:
    TypeHierarchyRootItem(const TypeHierarchyItem &item, Client *client)
        : HierarchyRootItem(item, client)
    {
        appendChild(new TypeHierarchyBasesItem(item, client));
        appendChild(new TypeHierarchyDerivedItem(item, client));
    }
};

class TreeView : public Utils::NavigationTreeView
{
public:
    explicit TreeView(const QString &name, QWidget *parent = nullptr)
        : Utils::NavigationTreeView(parent)
        , m_name(name)
    {}

    void contextMenuEvent(QContextMenuEvent *event) override
    {
        if (!event)
            return;

        QMenu contextMenu;

        QAction *action = contextMenu.addAction(Tr::tr("Open in Editor"));
        connect(action, &QAction::triggered, this, [this] () {
            emit activated(currentIndex());
        });
        action = contextMenu.addAction(Tr::tr("Open %1 Hierarchy").arg(m_name));
        connect(action, &QAction::triggered, this, [this] () {
            emit doubleClicked(currentIndex());
        });

        contextMenu.addSeparator();

        action = contextMenu.addAction(Tr::tr("Expand All"));
        connect(action, &QAction::triggered, this, &QTreeView::expandAll);
        action = contextMenu.addAction(Tr::tr("Collapse All"));
        connect(action, &QAction::triggered, this, &QTreeView::collapseAll);

        contextMenu.exec(event->globalPos());

        event->accept();
    }

private:
    const QString m_name;
};

class HierarchyWidgetHelper
{
public:
    HierarchyWidgetHelper(const QString &name, QWidget *theWidget) : m_view(new TreeView(name, theWidget))
    {
        m_delegate.setDelimiter(" ");
        m_delegate.setAnnotationRole(AnnotationRole);

        m_view->setModel(&m_model);
        m_view->setActivationMode(SingleClickActivation);
        m_view->setItemDelegate(&m_delegate);
        m_view->setUniformRowHeights(true);

        theWidget->setLayout(new QVBoxLayout);
        theWidget->layout()->addWidget(m_view);
        theWidget->layout()->setContentsMargins(0, 0, 0, 0);
        theWidget->layout()->setSpacing(0);

        QObject::connect(m_view, &NavigationTreeView::activated,
                         theWidget, [this](const QModelIndex &index) { onItemActivated(index); });
        QObject::connect(m_view, &QTreeView::doubleClicked,
                         theWidget, [this](const QModelIndex &index) { onItemDoubleClicked(index); });
    }

    ~HierarchyWidgetHelper()
    {
        if (m_runningRequest && m_runningRequest->first)
            m_runningRequest->first->cancelRequest(m_runningRequest->second);
    }

    void updateHierarchyAtCursorPosition()
    {
        m_model.clear();

        TextEditorWidget *editorWidget = TextEditorWidget::currentTextEditorWidget();
        if (!editorWidget)
            return;

        IDocument *document = editorWidget->textDocument();

        Client *client = LanguageClientManager::clientForFilePath(document->filePath());
        if (!client)
            return;

        HierarchyPosition position;
        position.textDocument = TextDocumentIdentifier().uri(client->uriFor(document->filePath()));
        position.position = positionOf(editorWidget->textCursor());
        sendRequest(client, position, document);
    }

protected:
    void addItem(TreeItem *item)
    {
        m_model.rootItem()->appendChild(item);
        m_view->expand(item->index());
        item->forChildrenAtLevel(1, [&](const TreeItem *child) { m_view->expand(child->index()); });
    }

    void setRunningRequest(Client *client, const MessageId &requestId)
    {
        m_runningRequest = std::make_pair(QPointer<Client>(client), requestId);
    }

    void resetRunningRequest()
    {
        m_runningRequest.reset();
    }

private:
    virtual void sendRequest(
        Client *client, const HierarchyPosition &position, const IDocument *document) = 0;

    void onItemDoubleClicked(const QModelIndex &index)
    {
        if (const auto link = index.data(LinkRole).value<Link>(); link.hasValidTarget())
            updateHierarchyAtCursorPosition();
    }

    void onItemActivated(const QModelIndex &index)
    {
        const auto link = index.data(LinkRole).value<Utils::Link>();
        if (link.hasValidTarget())
            EditorManager::openEditorAt(link);
    }

    AnnotatedItemDelegate m_delegate;
    NavigationTreeView * const m_view;
    std::optional<std::pair<QPointer<Client>, MessageId>> m_runningRequest;
    TreeModel<TreeItem> m_model;
};

class CallHierarchy : public QWidget, public HierarchyWidgetHelper
{
public:
    CallHierarchy() : HierarchyWidgetHelper(Tr::tr("Call"), this)
    {
        connect(LanguageClientManager::instance(), &LanguageClientManager::openCallHierarchy,
                this, [this] { updateHierarchyAtCursorPosition(); });
    }

private:
    void sendRequest(
        Client *client, const HierarchyPosition &position, const IDocument *document) override
    {
        if (!supportsCallHierarchy(client, document))
            return;

        CallHierarchyPrepareParams params;
        params.textDocument(position.textDocument);
        params.position(position.position);
        setRunningRequest(
            client,
            client->sendRequest<CallHierarchyPrepareRequest>(
                params,
                [this, client = QPointer<Client>(client)](
                    const Utils::Result<CallHierarchyPrepareRequestResult> &result) {
                    handlePrepareResult(client, result);
                }));
    }

    void handlePrepareResult(
        Client *client, const Utils::Result<CallHierarchyPrepareRequestResult> &result)
    {
        resetRunningRequest();
        if (!client)
            return;
        if (!result) {
            client->log(QtMsgType::QtCriticalMsg, result.error());
            return;
        }
        if (const auto items = std::get_if<QList<CallHierarchyItem>>(&*result)) {
            for (const CallHierarchyItem &item : *items)
                addItem(new CallHierarchyRootItem(item, client));
        }
    }
};

class TypeHierarchy : public TypeHierarchyWidget, public HierarchyWidgetHelper
{
public:
    TypeHierarchy() : HierarchyWidgetHelper(Tr::tr("Type"), this) {}

private:
    void reload() override
    {
        updateHierarchyAtCursorPosition();
    }

    void sendRequest(
        Client *client, const HierarchyPosition &position, const IDocument *document) override
    {
        if (!supportsTypeHierarchy(client, document))
            return;

        TypeHierarchyPrepareParams params;
        params.textDocument(position.textDocument);
        params.position(position.position);
        setRunningRequest(
            client,
            client->sendRequest<TypeHierarchyPrepareRequest>(
                params,
                [this, client = QPointer<Client>(client)](
                    const Utils::Result<TypeHierarchyPrepareRequestResult> &result) {
                    handlePrepareResult(client, result);
                }));
    }

    void handlePrepareResult(
        Client *client, const Utils::Result<TypeHierarchyPrepareRequestResult> &result)
    {
        resetRunningRequest();
        if (!client)
            return;
        if (!result) {
            client->log(QtMsgType::QtCriticalMsg, result.error());
            return;
        }
        if (const auto items = std::get_if<QList<TypeHierarchyItem>>(&*result)) {
            for (const TypeHierarchyItem &item : *items)
                addItem(new TypeHierarchyRootItem(item, client));
        }
    }
};

class CallHierarchyFactory : public INavigationWidgetFactory
{
public:
    CallHierarchyFactory()
    {
        setDisplayName(Tr::tr("Call Hierarchy"));
        setPriority(650);
        setId(Constants::CALL_HIERARCHY_FACTORY_ID);
    }

    NavigationView createWidget() final
    {
        auto h = new CallHierarchy;
        h->updateHierarchyAtCursorPosition();

        Icons::RELOAD_TOOLBAR.icon();
        auto button = new QToolButton;
        button->setIcon(Icons::RELOAD_TOOLBAR.icon());
        button->setToolTip(::LanguageClient::Tr::tr(
            "Reloads the call hierarchy for the symbol under cursor position."));
        connect(button, &QToolButton::clicked, this, [h] { h->updateHierarchyAtCursorPosition(); });
        return {h, {button}};
    }
};

class TypeHierarchyFactory final : public TypeHierarchyWidgetFactory
{
    TypeHierarchyWidget *createWidget(IEditor *editor) final
    {
        const auto editorWidget = TextEditorWidget::fromEditor(editor);
        if (!editorWidget)
            return nullptr;

        IDocument *document = editorWidget->textDocument();
        Client *const client = LanguageClientManager::clientForFilePath(document->filePath());
        if (!client || !supportsTypeHierarchy(client, document))
            return nullptr;

        return new TypeHierarchy;
    }
};

void setupCallHierarchyFactory()
{
    static CallHierarchyFactory theCallHierarchyFactory;
}

static bool supportsHierarchy(Client *client, const IDocument *document,
                              const QString &methodName, bool hasProvider)
{
    std::optional<bool> registered = client->dynamicCapabilities().isRegistered(methodName);
    bool supported = registered.value_or(false);
    if (registered) {
        if (supported) {
            supported = registrationApplies(client->dynamicCapabilities().option(methodName),
                                            document->filePath(),
                                            document->mimeType());
        }
    } else {
        supported = hasProvider;
    }
    return supported;
}

bool supportsCallHierarchy(Client *client, const IDocument *document)
{
    return supportsHierarchy(
        client,
        document,
        CallHierarchyPrepareRequest::method,
        client->capabilities().callHierarchyProvider().has_value());
}

void setupTypeHierarchyFactory()
{
    static TypeHierarchyFactory theTypeHierarchyFactory;
}

bool supportsTypeHierarchy(Client *client, const IDocument *document)
{
    return supportsHierarchy(
        client,
        document,
        TypeHierarchyPrepareRequest::method,
        client->capabilities().typeHierarchyProvider().has_value());
}

} // namespace LanguageClient
