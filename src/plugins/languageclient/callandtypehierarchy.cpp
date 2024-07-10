// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "callandtypehierarchy.h"

#include "languageclientmanager.h"
#include "languageclienttr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/inavigationwidgetfactory.h>

#include <languageserverprotocol/callhierarchy.h>
#include <languageserverprotocol/typehierarchy.h>

#include <texteditor/texteditor.h>
#include <texteditor/typehierarchy.h>

#include <utils/delegates.h>
#include <utils/navigationtreeview.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QLayout>
#include <QToolButton>

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

template<class Item, class Params, class Request, class Result>
class HierarchyItem : public TreeItem
{
public:
    HierarchyItem(const Item &item, Client *client)
        : m_item(item)
        , m_client(client)
    {}

protected:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            if (hasTag(SymbolTag::Deprecated))
                return Utils::Icons::WARNING.icon();
            return symbolIcon(int(m_item.symbolKind()));
        case Qt::DisplayRole:
            return m_item.name();
        case Qt::ToolTipRole:
            if (hasTag(SymbolTag::Deprecated))
                return Tr::tr("Deprecated");
            return {};
        case LinkRole: {
            if (!m_client)
                return QVariant();
            const Position start = m_item.selectionRange().start();
            return QVariant::fromValue(
                Link(m_client->serverUriToHostPath(m_item.uri()), start.line() + 1, start.character()));
        }
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

        Params params;
        params.setItem(m_item);
        Request request(params);
        request.setResponseCallback(
            [this](const typename Request::Response &response) {
                const std::optional<LanguageClientArray<Result>> result = response.result();
                if (result && !result->isNull()) {
                    for (const Result &item : result->toList()) {
                        if (item.isValid())
                            appendChild(new HierarchyItem(getSourceItem(item), m_client));
                    }
                }
            });
        m_client->sendMessage(request);
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

    bool hasTag(const SymbolTag tag) const
    {
        if (const std::optional<QList<SymbolTag>> tags = m_item.symbolTags())
            return tags->contains(tag);
        return false;
    }

    const Item m_item;
    bool m_fetchedChildren = false;
    QPointer<Client> m_client;
};

class CallHierarchyIncomingItem : public HierarchyItem<CallHierarchyItem,
                                                       CallHierarchyCallsParams,
                                                       CallHierarchyIncomingCallsRequest,
                                                       CallHierarchyIncomingCall>
{
public:
    CallHierarchyIncomingItem(const LanguageServerProtocol::CallHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Incoming");
        if (role == Qt::DecorationRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class CallHierarchyOutgoingItem : public HierarchyItem<CallHierarchyItem,
                                                       CallHierarchyCallsParams,
                                                       CallHierarchyOutgoingCallsRequest,
                                                       CallHierarchyOutgoingCall>
{
public:
    CallHierarchyOutgoingItem(const LanguageServerProtocol::CallHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Outgoing");
        if (role == Qt::DecorationRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

template<class Item> class HierarchyRootItem : public TreeItem
{
public:
    HierarchyRootItem(const Item &item)
        : m_item(item)
    {}

private:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            if (m_item.symbolTags().value_or(QList<SymbolTag>()).contains(SymbolTag::Deprecated))
                return Utils::Icons::WARNING.icon();
            return symbolIcon(int(m_item.symbolKind()));
        case Qt::DisplayRole:
            return m_item.name();
        default:
            return TreeItem::data(column, role);
        }
    }

    const Item m_item;
};


class CallHierarchyRootItem : public HierarchyRootItem<LanguageServerProtocol::CallHierarchyItem>
{
public:
    CallHierarchyRootItem(const LanguageServerProtocol::CallHierarchyItem &item, Client *client)
        : HierarchyRootItem(item)
    {
        appendChild(new CallHierarchyIncomingItem(item, client));
        appendChild(new CallHierarchyOutgoingItem(item, client));
    }
};

class TypeHierarchyBasesItem : public HierarchyItem<TypeHierarchyItem,
                                                    TypeHierarchyParams,
                                                    TypeHierarchySupertypesRequest,
                                                    TypeHierarchyItem>
{
public:
    TypeHierarchyBasesItem(const LanguageServerProtocol::TypeHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Bases");
        if (role == Qt::DecorationRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class TypeHierarchyDerivedItem : public HierarchyItem<TypeHierarchyItem,
                                                      TypeHierarchyParams,
                                                      TypeHierarchySubtypesRequest,
                                                      TypeHierarchyItem>
{
public:
    TypeHierarchyDerivedItem(const LanguageServerProtocol::TypeHierarchyItem &item, Client *client)
        : HierarchyItem(item, client)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("Derived");
        if (role == Qt::DecorationRole)
            return {};
        return HierarchyItem::data(column, role);
    }
};

class TypeHierarchyRootItem : public HierarchyRootItem<LanguageServerProtocol::TypeHierarchyItem>
{
public:
    TypeHierarchyRootItem(const LanguageServerProtocol::TypeHierarchyItem &item, Client *client)
        : HierarchyRootItem(item)
    {
        appendChild(new TypeHierarchyBasesItem(item, client));
        appendChild(new TypeHierarchyDerivedItem(item, client));
    }
};

class HierarchyWidgetHelper
{
public:
    HierarchyWidgetHelper(QWidget *theWidget) : m_view(new NavigationTreeView(theWidget))
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

    void updateHierarchyAtCursorPosition()
    {
        m_model.clear();

        BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
        if (!editor)
            return;

        Core::IDocument *document = editor->document();

        Client *client = LanguageClientManager::clientForFilePath(document->filePath());
        if (!client)
            return;

        TextDocumentPositionParams params;
        params.setTextDocument(TextDocumentIdentifier(client->hostPathToServerUri(document->filePath())));
        params.setPosition(Position(editor->editorWidget()->textCursor()));
        sendRequest(client, params, document);
    }

protected:
    void addItem(TreeItem *item)
    {
        m_model.rootItem()->appendChild(item);
        m_view->expand(item->index());
        item->forChildrenAtLevel(1, [&](const TreeItem *child) { m_view->expand(child->index()); });
    }

private:
    virtual void sendRequest(Client *client, const TextDocumentPositionParams &params,
                             const Core::IDocument *document) = 0;

    void onItemDoubleClicked(const QModelIndex &index)
    {
        if (const auto link = index.data(LinkRole).value<Link>(); link.hasValidTarget())
            updateHierarchyAtCursorPosition();
    }

    void onItemActivated(const QModelIndex &index)
    {
        const auto link = index.data(LinkRole).value<Utils::Link>();
        if (link.hasValidTarget())
            Core::EditorManager::openEditorAt(link);
    }

    AnnotatedItemDelegate m_delegate;
    NavigationTreeView * const m_view;
    TreeModel<TreeItem> m_model;
};

class CallHierarchy : public QWidget, public HierarchyWidgetHelper
{
public:
    CallHierarchy() : HierarchyWidgetHelper(this)
    {
        connect(LanguageClientManager::instance(), &LanguageClientManager::openCallHierarchy,
                this, [this] { updateHierarchyAtCursorPosition(); });
    }

private:
    void sendRequest(Client *client, const TextDocumentPositionParams &params,
                     const Core::IDocument *document) override
    {
        if (!supportsCallHierarchy(client, document))
            return;

        PrepareCallHierarchyRequest request(params);
        request.setResponseCallback([this, client = QPointer<Client>(client)](
                                        const PrepareCallHierarchyRequest::Response &response) {
            handlePrepareResponse(client, response);
        });
        client->sendMessage(request);
    }

    void handlePrepareResponse(Client *client,
                               const PrepareCallHierarchyRequest::Response &response)
    {
        if (!client)
            return;
        const std::optional<PrepareCallHierarchyRequest::Response::Error> error = response.error();
        if (error)
            client->log(*error);

        const std::optional<LanguageClientArray<LanguageServerProtocol::CallHierarchyItem>>
            result = response.result();
        if (result && !result->isNull()) {
            for (const LanguageServerProtocol::CallHierarchyItem &item : result->toList())
                addItem(new CallHierarchyRootItem(item, client));
        }
    }
};

class TypeHierarchy : public TypeHierarchyWidget, public HierarchyWidgetHelper
{
public:
    TypeHierarchy() : HierarchyWidgetHelper(this) {}

private:
    void reload() override
    {
        updateHierarchyAtCursorPosition();
    }

    void sendRequest(Client *client, const TextDocumentPositionParams &params,
                     const Core::IDocument *document) override
    {
        if (!supportsTypeHierarchy(client, document))
            return;

        PrepareTypeHierarchyRequest request(params);
        request.setResponseCallback([this, client = QPointer<Client>(client)](
                                        const PrepareTypeHierarchyRequest::Response &response) {
            handlePrepareResponse(client, response);
        });
        client->sendMessage(request);
    }

    void handlePrepareResponse(Client *client,
                               const PrepareTypeHierarchyRequest::Response &response)
    {
        if (!client)
            return;
        const std::optional<PrepareTypeHierarchyRequest::Response::Error> error = response.error();
        if (error)
            client->log(*error);

        const std::optional<LanguageClientArray<LanguageServerProtocol::TypeHierarchyItem>>
            result = response.result();
        if (result && !result->isNull()) {
            for (const LanguageServerProtocol::TypeHierarchyItem &item : result->toList())
                addItem(new TypeHierarchyRootItem(item, client));
        }
    }
};

class CallHierarchyFactory : public Core::INavigationWidgetFactory
{
public:
    CallHierarchyFactory()
    {
        setDisplayName(Tr::tr("Call Hierarchy"));
        setPriority(650);
        setId(Constants::CALL_HIERARCHY_FACTORY_ID);
    }

    Core::NavigationView createWidget() final
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
    TypeHierarchyWidget *createWidget(Core::IEditor *editor) final
    {
        const auto textEditor = qobject_cast<BaseTextEditor *>(editor);
        if (!textEditor)
            return nullptr;

        Client *const client = LanguageClientManager::clientForFilePath(
            textEditor->document()->filePath());
        if (!client || !supportsTypeHierarchy(client, textEditor->document()))
            return nullptr;

        return new TypeHierarchy;
    }
};

void setupCallHierarchyFactory()
{
    static CallHierarchyFactory theCallHierarchyFactory;
}

static bool supportsHierarchy(
    Client *client,
    const Core::IDocument *document,
    const QString &methodName,
    const std::optional<std::variant<bool, WorkDoneProgressOptions>> &provider)
{
    std::optional<bool> registered = client->dynamicCapabilities().isRegistered(methodName);
    bool supported = registered.value_or(false);
    if (registered) {
        if (supported) {
            const QJsonValue &options = client->dynamicCapabilities().option(methodName);
            const TextDocumentRegistrationOptions docOptions(options);
            supported = docOptions.filterApplies(document->filePath(),
                                                 Utils::mimeTypeForName(document->mimeType()));
        }
    } else {
        supported = provider.has_value();
    }
    return supported;
}

bool supportsCallHierarchy(Client *client, const Core::IDocument *document)
{
    return supportsHierarchy(client,
                             document,
                             PrepareCallHierarchyRequest::methodName,
                             client->capabilities().callHierarchyProvider());
}

void setupTypeHierarchyFactory()
{
    static TypeHierarchyFactory theTypeHierarchyFactory;
}

bool supportsTypeHierarchy(Client *client, const Core::IDocument *document)
{
    return supportsHierarchy(client,
                             document,
                             PrepareTypeHierarchyRequest::methodName,
                             client->capabilities().typeHierarchyProvider());
}

} // namespace LanguageClient
