// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlitemnode.h>

#include <QObject>
#include <QtQml>

class TabViewIndexModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant modelNodeBackendProperty READ modelNodeBackend WRITE setModelNodeBackend NOTIFY modelNodeBackendChanged)
    Q_PROPERTY(QStringList tabViewIndexModel READ tabViewIndexModel NOTIFY modelNodeBackendChanged)

public:
    explicit TabViewIndexModel(QObject *parent = nullptr);

    void setModelNodeBackend(const QVariant &modelNodeBackend);
    void setModelNode(const QmlDesigner::ModelNode &modelNode);
    QStringList tabViewIndexModel() const;
    void setupModel();

    static void registerDeclarativeType();

signals:
    void modelNodeBackendChanged();

private:
    QVariant modelNodeBackend() const;

    QmlDesigner::ModelNode m_modelNode;
    QStringList m_tabViewIndexModel;

};

QML_DECLARE_TYPE(TabViewIndexModel)
