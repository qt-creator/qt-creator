// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlitemnode.h>

#include <QDir>
#include <QObject>
#include <QStringList>
#include <QUrl>
#include <QtQml>

class ItemFilterModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString typeFilter READ typeFilter WRITE setTypeFilter)
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList itemModel READ itemModel NOTIFY itemModelChanged)
    Q_PROPERTY(bool selectionOnly READ selectionOnly WRITE setSelectionOnly NOTIFY selectionOnlyChanged)

public:
    explicit ItemFilterModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setTypeFilter(const QString &typeFilter);
    void setSelectionOnly(bool value);
    QString typeFilter() const;
    bool selectionOnly() const;
    void setupModel();
    QStringList itemModel() const;

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();
    void itemModelChanged();
    void selectionOnlyChanged();

private:
    QVariant modelNodeBackend() const;

private:
    QString m_typeFilter;
    bool m_lock;
    QStringList m_model;
    QmlDesigner::ModelNode m_modelNode;
    bool m_selectionOnly;
};

QML_DECLARE_TYPE(ItemFilterModel)
