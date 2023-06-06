// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QPointer>

#include <modelnode.h>

namespace QmlDesigner {

class StatesEditorView;

class PropertyChangesModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(bool propertyChangesVisible READ propertyChangesVisible NOTIFY
                   propertyChangesVisibleChanged)

    enum {
        Target = Qt::DisplayRole,
        Explicit = Qt::UserRole,
        RestoreEntryValues,
        PropertyModelNode
    };

public:
    PropertyChangesModel(QObject *parent = nullptr);
    ~PropertyChangesModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void reset();
    int count() const;

    Q_INVOKABLE void setPropertyChangesVisible(bool value);
    Q_INVOKABLE bool propertyChangesVisible() const;

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();
    void countChanged();
    void propertyChangesVisibleChanged();

private:
    QVariant modelNodeBackend() const;

private:
    ModelNode m_modelNode;
    QPointer<StatesEditorView> m_view;
};

} // namespace QmlDesigner
