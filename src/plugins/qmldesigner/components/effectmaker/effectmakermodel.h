// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "shaderfeatures.h"

#include <QMap>
#include <QStandardItemModel>

namespace QmlDesigner {

class CompositionNode;
class Uniform;

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

    void addNode(const QString &nodeQenPath);

    Q_INVOKABLE void removeNode(int idx);

signals:
    void isEmptyChanged();
    void selectedIndexChanged(int idx);

private:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        UniformsRole
    };

    bool isValidIndex(int idx) const;

    const QList<Uniform *> allUniforms();

    const QString getBufUniform();
    const QString getVSUniforms();
    const QString getFSUniforms();

    QList<CompositionNode *> m_nodes;

    int m_selectedIndex = -1;
    bool m_isEmpty = true;

    ShaderFeatures m_shaderFeatures;
};

} // namespace QmlDesigner
