// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlitemnode.h>

#include <QDir>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml>

class ItemFilterModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter NOTIFY typeFilterChanged)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList itemModel READ itemModel NOTIFY itemModelChanged)
    Q_PROPERTY(bool selectionOnly READ selectionOnly WRITE setSelectionOnly NOTIFY selectionOnlyChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        IdAndNameRole
    };
    Q_ENUM(Roles)

    explicit ItemFilterModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setTypeFilter(const QString &typeFilter);
    void setSelectionOnly(bool value);
    QString typeFilter() const;
    bool selectionOnly() const;
    void setupModel();
    QStringList itemModel() const;

    static void registerDeclarativeType();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    virtual QHash<int, QByteArray> roleNames() const override;

signals:
    void typeFilterChanged();
    void modelNodeBackendChanged();
    void itemModelChanged();
    void selectionOnlyChanged();

private:
    QVariant modelNodeBackend() const;
    QmlDesigner::ModelNode modelNodeForRow(const int &row) const;

private:
    QString m_typeFilter;
    QList<qint32> m_modelInternalIds;
    QmlDesigner::ModelNode m_modelNode;
    bool m_selectionOnly;
    static QHash<int, QByteArray> m_roles;
};

QML_DECLARE_TYPE(ItemFilterModel)
