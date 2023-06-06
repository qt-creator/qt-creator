// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QPointer>

#include <modelnode.h>

namespace QmlDesigner {

class PropertyModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(bool expanded READ expanded NOTIFY expandedChanged)

    enum { Name = Qt::DisplayRole, Value = Qt::UserRole, Type };

public:
    PropertyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setModelNodeBackend(const QVariant &modelNodeBackend);

    Q_INVOKABLE void setExplicit(bool value);
    Q_INVOKABLE void setRestoreEntryValues(bool value);
    Q_INVOKABLE void removeProperty(const QString &name);

    Q_INVOKABLE void setExpanded(bool value);
    Q_INVOKABLE bool expanded() const;

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();
    void expandedChanged();

private:
    QVariant modelNodeBackend() const;
    void setupModel();

private:
    ModelNode m_modelNode;
    QList<AbstractProperty> m_properties;
};

} // namespace QmlDesigner
