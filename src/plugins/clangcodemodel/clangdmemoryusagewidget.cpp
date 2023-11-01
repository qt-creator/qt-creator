// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdmemoryusagewidget.h"

#include "clangcodemodeltr.h"
#include "clangdclient.h"

#include <languageserverprotocol/jsonobject.h>

#include <utils/itemviews.h>
#include <utils/treemodel.h>

#include <QHeaderView>
#include <QMenu>
#include <QPointer>
#include <QVBoxLayout>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace ClangCodeModel::Internal {

class MemoryTree : public JsonObject
{
public:
    using JsonObject::JsonObject;

    // number of bytes used, including child components
    qint64 total() const { return qint64(typedValue<double>(totalKey)); }

    // number of bytes used, excluding child components
    qint64 self() const { return qint64(typedValue<double>(selfKey)); }

    // named child components
    using NamedComponent = std::pair<MemoryTree, QString>;
    QList<NamedComponent> children() const
    {
        QList<NamedComponent> components;
        const auto obj = operator const QJsonObject &();
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            if (it.key() == QLatin1String(totalKey) || it.key() == QLatin1String(selfKey))
                continue;
            components.push_back({MemoryTree(it.value()), it.key()});
        }
        return components;
    }

    static constexpr char totalKey[] = "_total";
    static constexpr char selfKey[] =  "_self";
};


class MemoryTreeItem : public TreeItem
{
public:
    MemoryTreeItem(const QString &displayName, const MemoryTree &tree)
        : m_displayName(displayName), m_bytesUsed(tree.total())
    {
        for (const MemoryTree::NamedComponent &component : tree.children())
            appendChild(new MemoryTreeItem(component.second, component.first));
    }

private:
    QVariant data(int column, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            if (column == 0)
                return m_displayName;
            return memString();
        case Qt::TextAlignmentRole:
            if (column == 1)
                return Qt::AlignRight;
            break;
        default:
            break;
        }
        return {};
    }

    QString memString() const
    {
        static const QList<std::pair<int, QString>> factors{{1000000000, "GB"},
                                                            {1000000, "MB"},
                                                            {1000, "KB"}};
        for (const auto &factor : factors) {
            if (m_bytesUsed > factor.first)
                return QString::number(qint64(std::round(double(m_bytesUsed) / factor.first)))
                        + ' ' + factor.second;
        }
        return QString::number(m_bytesUsed) + "  B";
    }

    const QString m_displayName;
    const qint64 m_bytesUsed;
};


class MemoryTreeModel : public BaseTreeModel
{
public:
    MemoryTreeModel()
    {
        setHeader({Tr::tr("Component"), Tr::tr("Total Memory")});
    }

    void update(const MemoryTree &tree)
    {
        setRootItem(new MemoryTreeItem({}, tree));
    }
};


class ClangdMemoryUsageWidget::Private
{
public:
    Private(ClangdMemoryUsageWidget *q, ClangdClient *client) : q(q), client(client)
    {
        setupUi();
        getMemoryTree();
    }

    void setupUi();
    void getMemoryTree();

    ClangdMemoryUsageWidget * const q;
    const QPointer<ClangdClient> client;
    MemoryTreeModel model;
    TreeView view;
    std::optional<MessageId> currentRequest;
};

ClangdMemoryUsageWidget::ClangdMemoryUsageWidget(ClangdClient *client)
    : d(new Private(this, client))
{
}

ClangdMemoryUsageWidget::~ClangdMemoryUsageWidget()
{
    if (d->client && d->currentRequest.has_value())
        d->client->cancelRequest(d->currentRequest.value());
    delete d;
}

void ClangdMemoryUsageWidget::Private::setupUi()
{
    const auto layout = new QVBoxLayout(q);
    view.setContextMenuPolicy(Qt::CustomContextMenu);
    view.header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view.header()->setStretchLastSection(false);
    view.setModel(&model);
    layout->addWidget(&view);
    QObject::connect(&view, &QWidget::customContextMenuRequested, q, [this](const QPoint &pos) {
        QMenu menu;
        menu.addAction(Tr::tr("Update"), [this] { getMemoryTree(); });
        menu.exec(view.mapToGlobal(pos));
    });
}

void ClangdMemoryUsageWidget::Private::getMemoryTree()
{
    Request<MemoryTree, std::nullptr_t, JsonObject> request("$/memoryUsage", {});
    request.setResponseCallback([this](decltype(request)::Response response) {
        currentRequest.reset();
        qCDebug(clangdLog) << "received memory usage response";
        if (const auto result = response.result())
            model.update(*result);
    });
    qCDebug(clangdLog) << "sending memory usage request";
    currentRequest = request.id();
    client->sendMessage(request, ClangdClient::SendDocUpdates::Ignore);
}

} // namespace ClangCodeModel::Internal
