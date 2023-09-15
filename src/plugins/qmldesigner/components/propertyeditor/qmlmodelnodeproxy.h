// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmlitemnode.h>

#include <QObject>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT QmlModelNodeProxy : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlDesigner::ModelNode modelNode READ modelNode NOTIFY modelNodeChanged)
    Q_PROPERTY(bool multiSelection READ multiSelection NOTIFY modelNodeChanged)

public:
    explicit QmlModelNodeProxy(QObject *parent = nullptr);

    void setup(const QmlObjectNode &objectNode);

    static void registerDeclarativeType();

    void emitSelectionToBeChanged();
    void emitSelectionChanged();

    QmlObjectNode qmlObjectNode() const;

    ModelNode modelNode() const;

    bool multiSelection() const;

    QString nodeId() const;

    QString simplifiedTypeName() const;

signals:
    void modelNodeChanged();
    void selectionToBeChanged();
    void selectionChanged();

private:
    QmlObjectNode m_qmlObjectNode;
};

} //QmlDesigner
