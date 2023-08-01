// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "callhierarchy.h"

#include "languageclientmanager.h"
#include "languageclienttr.h"

#include <QToolButton>
#include <coreplugin/editormanager/editormanager.h>
#include <languageserverprotocol/callhierarchy.h>
#include <texteditor/texteditor.h>
#include <utils/delegates.h>
#include <utils/navigationtreeview.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QLayout>

using namespace Utils;
using namespace TextEditor;
using namespace LanguageServerProtocol;

namespace LanguageClient {

namespace {
enum Direction { Incoming, Outgoing };

enum {
    AnnotationRole = Qt::UserRole + 1,
    LinkRole
};
}

class CallHierarchyItem : public TreeItem
{
public:
    CallHierarchyItem(const LanguageServerProtocol::CallHierarchyItem &item,
                      const Direction direction,
                      Client *client)
        : m_item(item)
        , m_direction(direction)
        , m_client(client)
    {
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            return symbolIcon(int(m_item.symbolKind()));
        case Qt::DisplayRole:
            return m_item.name();
        case LinkRole: {
            if (!m_client)
                return QVariant();
            const Position start = m_item.selectionRange().start();
            return QVariant::fromValue(
                Link(m_client->serverUriToHostPath(m_item.uri()), start.line() + 1, start.character()));
        }
        case AnnotationRole:
            if (const std::optional<QString> detail = m_item.detail())
                return *detail;
            return {};
        default:
            return TreeItem::data(column, role);
        }
    }
    bool canFetchMore() const override
    {
        return m_client && !m_fetchedChildren;
    }

    void fetchMore() override
    {
        m_fetchedChildren = true;
        if (!m_client)
            return;

        CallHierarchyCallsParams params;
        params.setItem(m_item);

        if (m_direction == Incoming) {
            CallHierarchyIncomingCallsRequest request(params);
            request.setResponseCallback(
                        [this](const CallHierarchyIncomingCallsRequest::Response &response) {
                const std::optional<LanguageClientArray<CallHierarchyIncomingCall>> result
                        = response.result();
                if (result && !result->isNull()) {
                    for (const CallHierarchyIncomingCall &item : result->toList()) {
                        if (item.isValid())
                            appendChild(new CallHierarchyItem(item.from(), m_direction, m_client));
                    }
                }
                if (!hasChildren())
                    update();
            });
            m_client->sendMessage(request);
        } else {
            CallHierarchyOutgoingCallsRequest request(params);
            request.setResponseCallback(
                        [this](const CallHierarchyOutgoingCallsRequest::Response &response) {
                const std::optional<LanguageClientArray<CallHierarchyOutgoingCall>> result
                        = response.result();
                if (result && !result->isNull()) {
                    for (const CallHierarchyOutgoingCall &item : result->toList()) {
                        if (item.isValid())
                            appendChild(new CallHierarchyItem(item.to(), m_direction, m_client));
                    }
                }
                if (!hasChildren())
                    update();
                });
            m_client->sendMessage(request);
        }
    }

protected:
    const LanguageServerProtocol::CallHierarchyItem m_item;
    const Direction m_direction;
    bool m_fetchedChildren = false;
    QPointer<Client> m_client;
};

class CallHierarchyDirectionItem : public CallHierarchyItem
{
public:
    CallHierarchyDirectionItem(const LanguageServerProtocol::CallHierarchyItem &item,
                               const Direction direction,
                               Client *client)
        : CallHierarchyItem(item, direction, client)
    {}

    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole)
            return m_direction == Incoming ? Tr::tr("Incoming") : Tr::tr("Outgoing");
        return CallHierarchyItem::data(column, role);
    }
};


class CallHierarchyRootItem : public TreeItem
{
public:
    CallHierarchyRootItem(const LanguageServerProtocol::CallHierarchyItem &item, Client *client)
        : m_item(item)
    {
        appendChild(new CallHierarchyDirectionItem(m_item, Incoming, client));
        appendChild(new CallHierarchyDirectionItem(m_item, Outgoing, client));
    }

    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DecorationRole:
            return symbolIcon(int(m_item.symbolKind()));
        case Qt::DisplayRole:
            return m_item.name();
        default:
            return TreeItem::data(column, role);
        }
    }

private:
    const LanguageServerProtocol::CallHierarchyItem m_item;
};

class CallHierarchy : public QWidget
{
public:
    CallHierarchy() : m_view(new NavigationTreeView(this))
    {
        m_delegate.setDelimiter(" ");
        m_delegate.setAnnotationRole(AnnotationRole);

        m_view->setModel(&m_model);
        m_view->setActivationMode(SingleClickActivation);
        m_view->setItemDelegate(&m_delegate);

        setLayout(new QVBoxLayout);
        layout()->addWidget(m_view);
        layout()->setContentsMargins(0, 0, 0, 0);
        layout()->setSpacing(0);

        connect(m_view, &NavigationTreeView::activated, this, &CallHierarchy::onItemActivated);

        connect(LanguageClientManager::instance(), &LanguageClientManager::openCallHierarchy,
                this, &CallHierarchy::updateHierarchyAtCursorPosition);
    }

    void onItemActivated(const QModelIndex &index)
    {
        const auto link = index.data(LinkRole).value<Utils::Link>();
        if (link.hasValidTarget())
            Core::EditorManager::openEditorAt(link);
    }

    void updateHierarchyAtCursorPosition();
    void handlePrepareResponse(Client *client,
                               const PrepareCallHierarchyRequest::Response &response);

    AnnotatedItemDelegate m_delegate;
    NavigationTreeView *m_view;
    TreeModel<TreeItem, CallHierarchyRootItem, CallHierarchyDirectionItem, CallHierarchyItem> m_model;
};

void CallHierarchy::updateHierarchyAtCursorPosition()
{
    m_model.clear();

    BaseTextEditor *editor = BaseTextEditor::currentTextEditor();
    if (!editor)
        return;

    Core::IDocument *document = editor->document();

    Client *client = LanguageClientManager::clientForFilePath(document->filePath());
    if (!client)
        return;

    if (!CallHierarchyFactory::supportsCallHierarchy(client, document))
        return;

    TextDocumentPositionParams params;
    params.setTextDocument(TextDocumentIdentifier(client->hostPathToServerUri(document->filePath())));
    params.setPosition(Position(editor->editorWidget()->textCursor()));

    PrepareCallHierarchyRequest request(params);
    request.setResponseCallback([this, client = QPointer<Client>(client)](
                                    const PrepareCallHierarchyRequest::Response &response) {
        handlePrepareResponse(client, response);
    });

    client->sendMessage(request);
}

void CallHierarchy::handlePrepareResponse(Client *client,
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
        for (const LanguageServerProtocol::CallHierarchyItem &item : result->toList()) {
            auto newItem = new CallHierarchyRootItem(item, client);
            m_model.rootItem()->appendChild(newItem);
            m_view->expand(newItem->index());
            newItem->forChildrenAtLevel(1, [&](const TreeItem *child) {
                m_view->expand(child->index());
            });
        }
    }
}

CallHierarchyFactory::CallHierarchyFactory()
{
    setDisplayName(Tr::tr("Call Hierarchy"));
    setPriority(650);
    setId(Constants::CALL_HIERARCHY_FACTORY_ID);
}

bool CallHierarchyFactory::supportsCallHierarchy(Client *client, const Core::IDocument *document)
{
    const QString methodName = PrepareCallHierarchyRequest::methodName;
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
        supported = client->capabilities().callHierarchyProvider().has_value();
    }
    return supported;
}

Core::NavigationView CallHierarchyFactory::createWidget()
{
    auto h = new CallHierarchy;
    h->updateHierarchyAtCursorPosition();

    Icons::RELOAD_TOOLBAR.icon();
    auto button = new QToolButton;
    button->setIcon(Icons::RELOAD_TOOLBAR.icon());
    button->setToolTip(Tr::tr("Reloads the call hierarchy for the symbol under cursor position."));
    connect(button, &QToolButton::clicked, this, [h] { h->updateHierarchyAtCursorPosition(); });
    return {h, {button}};
}

} // namespace LanguageClient
