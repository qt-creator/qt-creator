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
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend
                   NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList itemModel READ itemModel NOTIFY itemModelChanged)
    Q_PROPERTY(
        bool selectionOnly READ selectionOnly WRITE setSelectionOnly NOTIFY selectionOnlyChanged)
    Q_PROPERTY(QStringList selectedItems READ selectedItems WRITE setSelectedItems NOTIFY
                   selectedItemsChanged)

    Q_PROPERTY(QStringList validationRoles READ validationRoles WRITE setValidationRoles NOTIFY
                   validationRolesChanged)
    Q_PROPERTY(QStringList validationItems READ validationItems NOTIFY validationItemsChanged)

public:
    enum { IdRole = Qt::DisplayRole, NameRole = Qt::UserRole, IdAndNameRole, EnabledRole };

    explicit ItemFilterModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setTypeFilter(const QString &typeFilter);
    void setSelectionOnly(bool value);
    void setSelectedItems(const QStringList &selectedItems);
    void setValidationRoles(const QStringList &validationRoles);
    QString typeFilter() const;
    bool selectionOnly() const;
    QStringList selectedItems() const;
    QStringList itemModel() const;
    QStringList validationRoles() const;
    QStringList validationItems() const;

    static void registerDeclarativeType();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

signals:
    void typeFilterChanged();
    void modelNodeBackendChanged();
    void itemModelChanged();
    void selectionOnlyChanged();
    void selectedItemsChanged();
    void validationRolesChanged();
    void validationItemsChanged();

private:
    void setupModel();
    void setupValidationItems();
    QVariant modelNodeBackend() const;
    QmlDesigner::ModelNode modelNodeForRow(const int &row) const;

private:
    QString m_typeFilter;
    QList<qint32> m_modelInternalIds;
    QmlDesigner::ModelNode m_modelNode;
    bool m_selectionOnly;
    QStringList m_selectedItems;

    QStringList m_validationRoles;
    QStringList m_validationItems;
};

QML_DECLARE_TYPE(ItemFilterModel)
