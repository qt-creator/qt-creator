// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>

#include "effectnodescategory.h"

namespace QmlDesigner {

class EffectMakerModel : public QAbstractListModel
{
    Q_OBJECT

    enum Roles {
        CategoryRole = Qt::UserRole + 1,
        EffectsRole
    };

    Q_PROPERTY(bool isEmpty MEMBER m_isEmpty NOTIFY isEmptyChanged)
    Q_PROPERTY(int selectedIndex MEMBER m_selectedIndex NOTIFY selectedIndexChanged)

public:
    EffectMakerModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    bool isEmpty() const { return m_isEmpty; }

    void resetModel();

    QList<EffectNodesCategory *> categories() { return  m_categories; }

    Q_INVOKABLE void selectEffect(int idx, bool force = false);
    Q_INVOKABLE void applyToSelected(qint64 internalId, bool add = false);

signals:
    void isEmptyChanged();
    void selectedIndexChanged(int idx);
    void hasModelSelectionChanged();

private:
    bool isValidIndex(int idx) const;

    QList<EffectNodesCategory *> m_categories;

    int m_selectedIndex = -1;
    bool m_isEmpty = true;
    bool m_hasModelSelection = false;
};

} // namespace QmlDesigner
