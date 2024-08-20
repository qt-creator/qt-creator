// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "propertyeditorvalue.h"

#include <qmlitemnode.h>

#include <QObject>

namespace QmlDesigner {

class QMLDESIGNER_EXPORT QmlModelNodeProxy : public QObject
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
    void refresh();

    QmlObjectNode qmlObjectNode() const;

    ModelNode modelNode() const;

    bool multiSelection() const;

    QString nodeId() const;

    QString simplifiedTypeName() const;

    Q_INVOKABLE QList<int> allChildren(int internalId = -1) const;
    Q_INVOKABLE QList<int> allChildrenOfType(const QString &typeName, int internalId = -1) const;

    Q_INVOKABLE QString simplifiedTypeName(int internalId) const;

    Q_INVOKABLE PropertyEditorSubSelectionWrapper *registerSubSelectionWrapper(int internalId);

    Q_INVOKABLE void createModelNode(int internalIdParent,
                                     const QString &property,
                                     const QString &typeName,
                                     const QString &requiredImport = {});

    Q_INVOKABLE void moveNode(int internalIdParent,
                              const QString &propertyName,
                              int fromIndex,
                              int toIndex);

    Q_INVOKABLE bool isInstanceOf(const QString &typeName, int internalId = -1) const;

    Q_INVOKABLE void changeType(int internalId, const QString &typeName);

    void handleInstancePropertyChanged(const ModelNode &modelNode, PropertyNameView propertyName);

    void handleBindingPropertyChanged(const BindingProperty &property);
    void handleVariantPropertyChanged(const VariantProperty &property);
    void handlePropertiesRemoved(const AbstractProperty &property);

signals:
    void modelNodeChanged();
    void selectionToBeChanged();
    void selectionChanged();
    void refreshRequired();

private:
    QList<int> allChildren(const ModelNode &modelNode) const;
    QList<int> allChildrenOfType(const ModelNode &modelNode, const QString &typeName) const;
    PropertyEditorSubSelectionWrapper *findWrapper(int internalId) const;

    QmlObjectNode m_qmlObjectNode;
    QList<QSharedPointer<PropertyEditorSubSelectionWrapper>> m_subselection;
};

} //QmlDesigner
