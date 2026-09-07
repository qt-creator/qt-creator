// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdmemoryusagewidget.h"

#include "clangcodemodeltr.h"
#include "clangdclient.h"


#include <utils/itemviews.h>
#include <utils/treemodel.h>

#include <QHeaderView>
#include <QMenu>
#include <QPointer>
#include <QVBoxLayout>

using namespace LanguageServerProtocol;
using namespace Utils;

namespace ClangCodeModel::Internal {

// The memory usage clangd reports is a tree of arbitrarily named components,
// which no schema describes. See https://clangd.llvm.org/extensions#memory-usage
class MemoryTree
{
public:
    MemoryTree() = default;
    explicit MemoryTree(const QJsonObject &object) : m_object(object) {}

    // number of bytes used, including child components
    qint64 total() const { return qint64(m_object.value(totalKey).toDouble()); }

    // number of bytes used, excluding child components
    qint64 self() const { return qint64(m_object.value(selfKey).toDouble()); }

    // named child components
    using NamedComponent = std::pair<MemoryTree, QString>;
    QList<NamedComponent> children() const
    {
        QList<NamedComponent> components;
        for (auto it = m_object.begin(); it != m_object.end(); ++it) {
            if (it.key() == totalKey || it.key() == selfKey)
                continue;
            components.push_back({MemoryTree(it.value().toObject()), it.key()});
        }
        return components;
    }

private:
    static constexpr QLatin1StringView totalKey{"_total"};
    static constexpr QLatin1StringView selfKey{"_self"};

    QJsonObject m_object;
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
        d->client->cancelRequest(*d->currentRequest);
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
    qCDebug(clangdLog) << "sending memory usage request";
    currentRequest = client->sendRawRequest({}, "$/memoryUsage",
                                            [this](const QJsonObject &response) {
        currentRequest.reset();
        qCDebug(clangdLog) << "received memory usage response";
        model.update(MemoryTree(response.value("result").toObject()));
    }, ClangdClient::SendDocUpdates::Ignore);
}

} // namespace ClangCodeModel::Internal
