/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "dynamicpropertiesmodel.h"

#include "connectionview.h"

#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <rewritingexception.h>
#include <rewritertransaction.h>

#include <utils/fileutils.h>

#include <QMessageBox>
#include <QTimer>
#include <QUrl>

namespace {

bool compareVariantProperties(const QmlDesigner::VariantProperty &variantProperty01, const QmlDesigner::VariantProperty &variantProperty02)
{
    if (variantProperty01.parentModelNode() != variantProperty02.parentModelNode())
        return false;
    if (variantProperty01.name() != variantProperty02.name())
        return false;
    return true;
}

QString idOrTypeNameForNode(const QmlDesigner::ModelNode &modelNode)
{
    QString idLabel = modelNode.id();
    if (idLabel.isEmpty())
        idLabel = modelNode.simplifiedTypeName();

    return idLabel;
}

QmlDesigner::PropertyName unusedProperty(const QmlDesigner::ModelNode &modelNode)
{
    QmlDesigner::PropertyName propertyName = "property";
    int i = 0;
    if (modelNode.metaInfo().isValid()) {
        while (true) {
            const QmlDesigner::PropertyName currentPropertyName = propertyName + QString::number(i).toLatin1();
            if (!modelNode.hasProperty(currentPropertyName) && !modelNode.metaInfo().hasProperty(currentPropertyName))
                return currentPropertyName;
            i++;
        }
    }

    return propertyName;
}

QVariant convertVariantForTypeName(const QVariant &variant, const QmlDesigner::TypeName &typeName)
{
    QVariant returnValue = variant;

    if (typeName == "int") {
        bool ok;
        returnValue = variant.toInt(&ok);
        if (!ok)
            returnValue = 0;
    } else if (typeName == "real") {
        bool ok;
        returnValue = variant.toReal(&ok);
        if (!ok)
            returnValue = 0.0;

    } else if (typeName == "string") {
        returnValue = variant.toString();

    } else if (typeName == "bool") {
        returnValue = variant.toBool();
    } else if (typeName == "url") {
        returnValue = variant.toUrl();
    } else if (typeName == "color") {
        if (QColor::isValidColor(variant.toString())) {
            returnValue = variant.toString();
        } else {
            returnValue = QColor(Qt::black);
        }
    } else if (typeName == "Item") {
        returnValue = 0;
    }

    return returnValue;
}

} //internal namespace

namespace QmlDesigner {

namespace Internal {

DynamicPropertiesModel::DynamicPropertiesModel(ConnectionView *parent)
    : QStandardItemModel(parent)
    , m_connectionView(parent)
{
    connect(this, &QStandardItemModel::dataChanged, this, &DynamicPropertiesModel::handleDataChanged);
}

void DynamicPropertiesModel::resetModel()
{
    beginResetModel();
    clear();
    setHorizontalHeaderLabels(QStringList({ tr("Item"), tr("Property"), tr("Property Type"),
                                            tr("Property Value") }));

    foreach (const ModelNode modelNode, m_selectedModelNodes)
        addModelNode(modelNode);

    endResetModel();
}

void DynamicPropertiesModel::bindingPropertyChanged(const BindingProperty &bindingProperty)
{
    if (!bindingProperty.isDynamic())
        return;

    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBindingProperty(bindingProperty);

        if (rowNumber == -1) {
            addBindingProperty(bindingProperty);
        } else {
            updateBindingProperty(rowNumber);
        }
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::variantPropertyChanged(const VariantProperty &variantProperty)
{
    if (!variantProperty.isDynamic())
        return;

    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(variantProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForVariantProperty(variantProperty);

        if (rowNumber == -1) {
            addVariantProperty(variantProperty);
        } else {
            updateVariantProperty(rowNumber);
        }
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::bindingRemoved(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    QList<ModelNode> selectedNodes = connectionView()->selectedModelNodes();
    if (!selectedNodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBindingProperty(bindingProperty);
        removeRow(rowNumber);
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::selectionChanged(const QList<ModelNode> &selectedNodes)
{
    m_handleDataChanged = false;
    m_selectedModelNodes = selectedNodes;
    resetModel();
    m_handleDataChanged = true;
}

ConnectionView *DynamicPropertiesModel::connectionView() const
{
    return m_connectionView;
}

BindingProperty DynamicPropertiesModel::bindingPropertyForRow(int rowNumber) const
{

    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = connectionView()->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.bindingProperty(targetPropertyName.toUtf8());

    return BindingProperty();
}

VariantProperty DynamicPropertiesModel::variantPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = connectionView()->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.variantProperty(targetPropertyName.toUtf8());

    return VariantProperty();
}

QStringList DynamicPropertiesModel::possibleTargetProperties(const BindingProperty &bindingProperty) const
{
    const ModelNode modelNode = bindingProperty.parentModelNode();

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleTargetPropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    if (metaInfo.isValid()) {
        QStringList possibleProperties;
        foreach (const PropertyName &propertyName, metaInfo.propertyNames()) {
            if (metaInfo.propertyIsWritable(propertyName))
                possibleProperties << QString::fromUtf8(propertyName);
        }

        return possibleProperties;
    }

    return QStringList();
}

void DynamicPropertiesModel::addDynamicPropertyForCurrentNode()
{
    if (connectionView()->selectedModelNodes().count() == 1) {
        ModelNode modelNode = connectionView()->selectedModelNodes().first();
        if (modelNode.isValid()) {
            try {
                modelNode.variantProperty(unusedProperty(modelNode)).setDynamicTypeNameAndValue("string", QLatin1String("none.none"));
            } catch (RewritingException &e) {
                m_exceptionError = e.description();
                QTimer::singleShot(200, this, SLOT(handleException()));
            }
        }
    } else {
        qWarning() << " BindingModel::addBindingForCurrentNode not one node selected";
    }
}

QStringList DynamicPropertiesModel::possibleSourceProperties(const BindingProperty &bindingProperty) const
{
    const QString expression = bindingProperty.expression();
    const QStringList stringlist = expression.split(QLatin1String("."));

    PropertyName typeName;

    if (bindingProperty.parentModelNode().metaInfo().isValid()) {
        typeName = bindingProperty.parentModelNode().metaInfo().propertyTypeName(bindingProperty.name());
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for target node";
    }

    const QString id = stringlist.first();

    ModelNode modelNode = getNodeByIdOrParent(id, bindingProperty.parentModelNode());

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    if (metaInfo.isValid())  {
        QStringList possibleProperties;
        foreach (const PropertyName &propertyName, metaInfo.propertyNames()) {
            if (metaInfo.propertyTypeName(propertyName) == typeName) //### todo proper check
                possibleProperties << QString::fromUtf8(propertyName);
        }

        return possibleProperties;
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for source node";
    }

    return QStringList();
}

void DynamicPropertiesModel::deleteDynamicPropertyByRow(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);
    if (bindingProperty.isValid()) {
        bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
    }

    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isValid()) {
        variantProperty.parentModelNode().removeProperty(variantProperty.name());
    }

    resetModel();
}

void DynamicPropertiesModel::addProperty(const QVariant &propertyValue,
                                         const QString &propertyType,
                                         const AbstractProperty &abstractProperty)
{
    QList<QStandardItem*> items;

    QStandardItem *idItem;
    QStandardItem *propertyNameItem;
    QStandardItem *propertyTypeItem;
    QStandardItem *propertyValueItem;

    idItem = new QStandardItem(idOrTypeNameForNode(abstractProperty.parentModelNode()));
    updateCustomData(idItem, abstractProperty);

    propertyNameItem = new QStandardItem(QString::fromUtf8(abstractProperty.name()));

    items.append(idItem);
    items.append(propertyNameItem);


    propertyTypeItem = new QStandardItem(propertyType);
    items.append(propertyTypeItem);

    propertyValueItem = new QStandardItem();
    propertyValueItem->setData(propertyValue, Qt::DisplayRole);
    items.append(propertyValueItem);

    appendRow(items);
}

void DynamicPropertiesModel::addBindingProperty(const BindingProperty &property)
{
    QVariant value = property.expression();
    QString type = QString::fromLatin1(property.dynamicTypeName());
    addProperty(value, type, property);
}

void DynamicPropertiesModel::addVariantProperty(const VariantProperty &property)
{
    QVariant value = property.value();
    QString type = QString::fromLatin1(property.dynamicTypeName());
    addProperty(value, type, property);
}

void DynamicPropertiesModel::updateBindingProperty(int rowNumber)
{
    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    if (bindingProperty.isValid()) {
        QString propertyName = QString::fromUtf8(bindingProperty.name());
        updateDisplayRole(rowNumber, PropertyNameRow, propertyName);
        QString value = bindingProperty.expression();
        QString type = QString::fromUtf8(bindingProperty.dynamicTypeName());
        updateDisplayRole(rowNumber, PropertyTypeRow, type);
        updateDisplayRole(rowNumber, PropertyValueRow, value);
    }
}

void DynamicPropertiesModel::updateVariantProperty(int rowNumber)
{
    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isValid()) {
        QString propertyName = QString::fromUtf8(variantProperty.name());
        updateDisplayRole(rowNumber, PropertyNameRow, propertyName);
        QVariant value = variantProperty.value();
        QString type = QString::fromUtf8(variantProperty.dynamicTypeName());
        updateDisplayRole(rowNumber, PropertyTypeRow, type);

        updateDisplayRoleFromVariant(rowNumber, PropertyValueRow, value);
    }
}

void DynamicPropertiesModel::addModelNode(const ModelNode &modelNode)
{
    foreach (const BindingProperty &bindingProperty, modelNode.bindingProperties()) {
        if (bindingProperty.isDynamic())
            addBindingProperty(bindingProperty);
    }

    foreach (const VariantProperty &variantProperty, modelNode.variantProperties()) {
        if (variantProperty.isDynamic())
            addVariantProperty(variantProperty);
    }
}

void DynamicPropertiesModel::updateValue(int row)
{
    BindingProperty bindingProperty = bindingPropertyForRow(row);

    if (bindingProperty.isBindingProperty()) {

        const QString sourceNode = data(index(row, PropertyTypeRow)).toString();
        const QString sourceProperty = data(index(row, PropertyValueRow)).toString();

        const QString expression = data(index(row, PropertyValueRow)).toString();

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updateValue"));
        try {
            bindingProperty.setDynamicTypeNameAndExpression(bindingProperty.dynamicTypeName(), expression);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) {
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, SLOT(handleException()));
        }
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(row);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = data(index(row, PropertyValueRow));

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updateValue"));
        try {
            variantProperty.setDynamicTypeNameAndValue(variantProperty.dynamicTypeName(), value);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) {
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, SLOT(handleException()));
        }
    }
}

void DynamicPropertiesModel::updatePropertyName(int rowNumber)
{
    const PropertyName newName = data(index(rowNumber, PropertyNameRow)).toString().toUtf8();
    if (newName.isEmpty()) {
        qWarning() << "DynamicPropertiesModel::updatePropertyName invalid property name";
        return;
    }

    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    if (bindingProperty.isBindingProperty()) {
        const QString expression = bindingProperty.expression();
        const PropertyName dynamicPropertyType = bindingProperty.dynamicTypeName();
        ModelNode targetNode = bindingProperty.parentModelNode();

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updatePropertyName"));
        try {
            targetNode.bindingProperty(newName).setDynamicTypeNameAndExpression(dynamicPropertyType, expression);
            targetNode.removeProperty(bindingProperty.name());
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

        updateCustomData(rowNumber, targetNode.bindingProperty(newName));
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = variantProperty.value();
        const PropertyName dynamicPropertyType = variantProperty.dynamicTypeName();
        ModelNode targetNode = variantProperty.parentModelNode();

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updatePropertyName"));
        try {
            targetNode.variantProperty(newName).setDynamicTypeNameAndValue(dynamicPropertyType, value);
            targetNode.removeProperty(variantProperty.name());
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

        updateCustomData(rowNumber, targetNode.variantProperty(newName));
    }
}

void DynamicPropertiesModel::updatePropertyType(int rowNumber)
{

    const TypeName newType = data(index(rowNumber, PropertyTypeRow)).toString().toLatin1();

    if (newType.isEmpty()) {
        qWarning() << "DynamicPropertiesModel::updatePropertyName invalid property type";
        return;
    }

    BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);

    if (bindingProperty.isBindingProperty()) {
        const QString expression = bindingProperty.expression();
        const PropertyName propertyName = bindingProperty.name();
        ModelNode targetNode = bindingProperty.parentModelNode();

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updatePropertyType"));
        try {
            targetNode.removeProperty(bindingProperty.name());
            targetNode.bindingProperty(propertyName).setDynamicTypeNameAndExpression(newType, expression);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

        updateCustomData(rowNumber, targetNode.bindingProperty(propertyName));
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = variantProperty.value();
        ModelNode targetNode = variantProperty.parentModelNode();
        const PropertyName propertyName = variantProperty.name();

        RewriterTransaction transaction = connectionView()->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updatePropertyType"));
        try {
            targetNode.removeProperty(variantProperty.name());
            if (newType == "alias") { //alias properties have to be bindings
                targetNode.bindingProperty(propertyName).setDynamicTypeNameAndExpression(newType, QLatin1String("none.none"));
            } else {
                targetNode.variantProperty(propertyName).setDynamicTypeNameAndValue(newType, convertVariantForTypeName(value, newType));
            }
            transaction.commit(); //committing in the try block
        } catch (Exception &e) { //better save then sorry
            QMessageBox::warning(0, tr("Error"), e.description());
        }

        updateCustomData(rowNumber, targetNode.variantProperty(propertyName));

        if (variantProperty.isVariantProperty()) {
            updateVariantProperty(rowNumber);
        } else if (bindingProperty.isBindingProperty()) {
            updateBindingProperty(rowNumber);
        }
    }
}

ModelNode DynamicPropertiesModel::getNodeByIdOrParent(const QString &id, const ModelNode &targetNode) const
{
    ModelNode modelNode;

    if (id != QLatin1String("parent")) {
        modelNode = connectionView()->modelNodeForId(id);
    } else {
        if (targetNode.hasParentProperty()) {
            modelNode = targetNode.parentProperty().parentModelNode();
        }
    }
    return modelNode;
}

void DynamicPropertiesModel::updateCustomData(QStandardItem *item, const AbstractProperty &property)
{
    item->setData(property.parentModelNode().internalId(), Qt::UserRole + 1);
    item->setData(property.name(), Qt::UserRole + 2);
}

void DynamicPropertiesModel::updateCustomData(int row, const AbstractProperty &property)
{
    QStandardItem* idItem = item(row, 0);
    updateCustomData(idItem, property);
}

int DynamicPropertiesModel::findRowForBindingProperty(const BindingProperty &bindingProperty) const
{
    for (int i=0; i < rowCount(); i++) {
        if (compareBindingProperties(bindingPropertyForRow(i), bindingProperty))
            return i;
    }
    //not found
    return -1;
}

int DynamicPropertiesModel::findRowForVariantProperty(const VariantProperty &variantProperty) const
{
    for (int i=0; i < rowCount(); i++) {
        if (compareVariantProperties(variantPropertyForRow(i), variantProperty))
            return i;
    }
    //not found
    return -1;
}

bool DynamicPropertiesModel::getExpressionStrings(const BindingProperty &bindingProperty, QString *sourceNode, QString *sourceProperty)
{
    //### todo we assume no expressions yet

    const QString expression = bindingProperty.expression();

    if (true) {
        const QStringList stringList = expression.split(QLatin1String("."));

        *sourceNode = stringList.first();

        QString propertyName;

        for (int i=1; i < stringList.count(); i++) {
            propertyName += stringList.at(i);
            if (i != stringList.count() - 1)
                propertyName += QLatin1String(".");
        }
        *sourceProperty = propertyName;
    }
    return true;
}

void DynamicPropertiesModel::updateDisplayRole(int row, int columns, const QString &string)
{
    QModelIndex modelIndex = index(row, columns);
    if (data(modelIndex).toString() != string)
        setData(modelIndex, string);
}

void DynamicPropertiesModel::updateDisplayRoleFromVariant(int row, int columns, const QVariant &variant)
{
    QModelIndex modelIndex = index(row, columns);
    if (data(modelIndex) != variant)
        setData(modelIndex, variant);
}


void DynamicPropertiesModel::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    if (!m_handleDataChanged)
        return;

    if (topLeft != bottomRight) {
        qWarning() << "BindingModel::handleDataChanged multi edit?";
        return;
    }

    m_lock = true;

    int currentColumn = topLeft.column();
    int currentRow = topLeft.row();

    switch (currentColumn) {
    case TargetModelNodeRow: {
        //updating user data
    } break;
    case PropertyNameRow: {
        updatePropertyName(currentRow);
    } break;
    case PropertyTypeRow: {
        updatePropertyType(currentRow);
    } break;
    case PropertyValueRow: {
        updateValue(currentRow);
    } break;

    default: qWarning() << "BindingModel::handleDataChanged column" << currentColumn;
    }

    m_lock = false;
}

void DynamicPropertiesModel::handleException()
{
    QMessageBox::warning(0, tr("Error"), m_exceptionError);
    resetModel();
}

} // namespace Internal

} // namespace QmlDesigner
