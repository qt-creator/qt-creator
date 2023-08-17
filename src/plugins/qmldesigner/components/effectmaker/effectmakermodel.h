// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMap>
#include <QStandardItemModel>

namespace QmlDesigner {

class CompositionNode;

class EffectMakerModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)

public:
    EffectMakerModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool isEmpty() const { return m_isEmpty; }

    void resetModel();

    void addNode(const QString &nodeQenPath);

    Q_INVOKABLE void selectEffect(int idx, bool force = false);
    Q_INVOKABLE void applyToSelected(qint64 internalId, bool add = false);

signals:
    void isEmptyChanged();
    void selectedIndexChanged(int idx);
    void hasModelSelectionChanged();

private:
    enum Roles {
        NameRole = Qt::UserRole + 1,
//        TODO
    };

    bool isValidIndex(int idx) const;

    QMap<int, CompositionNode *> m_nodes;

    int m_selectedIndex = -1;
    bool m_isEmpty = true;
};

} // namespace QmlDesigner
