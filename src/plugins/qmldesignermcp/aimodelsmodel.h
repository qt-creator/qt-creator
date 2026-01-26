// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>

#include "aiproviderconfig.h"

namespace QmlDesigner {

class AiModelsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(
        int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

public:
    explicit AiModelsModel(QObject *parent = nullptr);
    ~AiModelsModel();

    enum class Roles {
        Provider = Qt::UserRole + 1,
        ModelId,
        SourceIndex,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void update();

    AiModelInfo selectedInfo() const;
    int selectedIndex() const;
    void setSelectedIndex(int idx);

signals:
    void selectedIndexChanged();

private: // functions
    AiModelInfo at(int idx) const;
    int findIndex(const QString &providerName, const QString &modelId) const;

private: // variables
    QList<AiModelInfo> m_data;
    int m_selectedIndex = -1;
};

} // namespace QmlDesigner
