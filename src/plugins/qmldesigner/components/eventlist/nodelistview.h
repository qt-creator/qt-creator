// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <abstractview.h>
#include <QStandardItemModel>

namespace QmlDesigner {

class NodeListModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Columns : unsigned int {
        idColumn = 0,
        typeColumn = 1,
        fromColumn = 2,
        toColumn = 3
    };

    enum Roles : unsigned int {
        internalIdRole = Qt::UserRole + 1,
        eventIdsRole = Qt::UserRole + 2
    };

    NodeListModel(QObject *parent = nullptr);
};


class NodeListView : public AbstractView
{
    Q_OBJECT

public:
    explicit NodeListView(ExternalDependenciesInterface &externalDependencies);
    ~NodeListView() override;

    int currentNode() const;
    QStandardItemModel *itemModel() const;

    void reset();
    void selectNode(int internalId);
    bool addEventId(int nodeId, const QString &event);
    bool removeEventIds(int nodeId, const QStringList &events);
    QString setNodeId(int internalId, const QString &id);

private:
    ModelNode compatibleModelNode(int nodeId);
    bool setEventIds(const ModelNode &node, const QStringList &events);

    QStandardItemModel *m_itemModel;
};

} // namespace QmlDesigner.
