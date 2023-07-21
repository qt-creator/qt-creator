// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <studioquickwidget.h>

#include <QStandardItemModel>

namespace QmlDesigner {

class AbstractProperty;
class AbstractView;
class BindingProperty;
class ModelNode;
class VariantProperty;

class DynamicPropertiesModelBackendDelegate;

class DynamicPropertiesModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum ColumnRoles {
        TargetModelNodeRow = 0,
        PropertyNameRow = 1,
        PropertyTypeRow = 2,
        PropertyValueRow = 3
    };

    enum UserRoles {
        InternalIdRole = Qt::UserRole + 2,
        TargetNameRole,
        PropertyNameRole,
        PropertyTypeRole,
        PropertyValueRole
    };

    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(DynamicPropertiesModelBackendDelegate *delegate READ delegate CONSTANT)

    DynamicPropertiesModel(bool explicitSelection, AbstractView *parent);

    void bindingPropertyChanged(const BindingProperty &bindingProperty);
    void abstractPropertyChanged(const AbstractProperty &bindingProperty);
    void variantPropertyChanged(const VariantProperty &variantProperty);
    void bindingRemoved(const BindingProperty &bindingProperty);
    void variantRemoved(const VariantProperty &variantProperty);
    void reset();
    void setSelectedNode(const ModelNode &node);
    const QList<ModelNode> selectedNodes() const;
    const ModelNode singleSelectedNode() const;

    AbstractView *view() const { return m_view; }
    AbstractProperty abstractPropertyForRow(int rowNumber) const;
    BindingProperty bindingPropertyForRow(int rowNumber) const;
    VariantProperty variantPropertyForRow(int rowNumber) const;
    QStringList possibleTargetProperties(const BindingProperty &bindingProperty) const;
    QStringList possibleSourceProperties(const BindingProperty &bindingProperty) const;
    void deleteDynamicPropertyByRow(int rowNumber);

    void updateDisplayRoleFromVariant(int row, int columns, const QVariant &variant);
    void addDynamicPropertyForCurrentNode();
    void resetModel();

    BindingProperty replaceVariantWithBinding(const PropertyName &name, bool copyValue = false);
    void resetProperty(const PropertyName &name);

    void dispatchPropertyChanges(const AbstractProperty &abstractProperty);

    PropertyName unusedProperty(const ModelNode &modelNode);

    static bool isValueType(const TypeName &type);
    static QVariant defaultValueForType(const TypeName &type);
    static QString defaultExpressionForType(const TypeName &type);

    Q_INVOKABLE void add();
    Q_INVOKABLE void remove(int row);

    int currentIndex() const;
    void setCurrentIndex(int i);

    int findRowForProperty(const AbstractProperty &abstractProperty) const;

signals:
    void currentIndexChanged();

protected:
    void addProperty(const QVariant &propertyValue,
                     const QString &propertyType,
                     const AbstractProperty &abstractProperty);
    void addBindingProperty(const BindingProperty &property);
    void addVariantProperty(const VariantProperty &property);
    void updateBindingProperty(int rowNumber);
    void updateVariantProperty(int rowNumber);
    void addModelNode(const ModelNode &modelNode);
    void updateValue(int row);
    void updatePropertyName(int rowNumber);
    void updatePropertyType(int rowNumber);
    ModelNode getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const;
    void updateCustomData(QStandardItem *item, const AbstractProperty &property);
    void updateCustomData(int row, const AbstractProperty &property);
    int findRowForBindingProperty(const BindingProperty &bindingProperty) const;
    int findRowForVariantProperty(const VariantProperty &variantProperty) const;

    bool getExpressionStrings(const BindingProperty &bindingProperty,
                              QString *sourceNode,
                              QString *sourceProperty);

    void updateDisplayRole(int row, int columns, const QString &string);

    QHash<int, QByteArray> roleNames() const override;

    DynamicPropertiesModelBackendDelegate *delegate() const;

private:
    void handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void handleException();

    AbstractView *m_view = nullptr;
    bool m_lock = false;
    bool m_handleDataChanged = false;
    QString m_exceptionError;
    QList<ModelNode> m_selectedNodes;
    bool m_explicitSelection = false;
    int m_currentIndex = 0;

    DynamicPropertiesModelBackendDelegate *m_delegate = nullptr;
};

class DynamicPropertiesModelBackendDelegate : public QObject
{
    Q_OBJECT

    Q_PROPERTY(StudioQmlComboBoxBackend *type READ type CONSTANT)
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)
    Q_PROPERTY(StudioQmlTextBackend *name READ name CONSTANT)
    Q_PROPERTY(StudioQmlTextBackend *value READ value CONSTANT)
    //Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)

public:
    DynamicPropertiesModelBackendDelegate(DynamicPropertiesModel *parent = nullptr);

signals:
    void currentRowChanged();
    void nameChanged();
    void valueChanged();

private:
    int currentRow() const;
    void setCurrentRow(int i);
    void handleTypeChanged();
    void handleNameChanged();
    void handleValueChanged();
    void handleException();
    QVariant variantValue() const;

    StudioQmlComboBoxBackend *type();

    StudioQmlTextBackend *name();
    StudioQmlTextBackend *value();

    StudioQmlComboBoxBackend m_type;
    StudioQmlTextBackend m_name;
    StudioQmlTextBackend m_value;
    int m_currentRow = -1;
    QString m_exceptionError;
};

} // namespace QmlDesigner
