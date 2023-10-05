// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStandardItemModel>

namespace EffectMaker {

class Uniform;

class EffectMakerUniformsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    EffectMakerUniformsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    void resetModel();

    void addUniform(Uniform *uniform);

    QList<Uniform *> uniforms() const;

private:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        DescriptionRole,
        ValueRole,
        BackendValueRole,
        DefaultValueRole,
        MaxValueRole,
        MinValueRole,
        TypeRole,
    };

    QList<Uniform *> m_uniforms;
};

} // namespace EffectMaker

