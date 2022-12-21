// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dynamicpropertiesmodel.h"

#include "connectionview.h"

#include <bindingproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

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
    } else if (typeName == "vector2d") {
        returnValue = "Qt.vector2d(0, 0)";
    } else if (typeName == "vector3d") {
        returnValue = "Qt.vector3d(0, 0, 0)";
    } else if (typeName == "vector4d") {
        returnValue = "Qt.vector4d(0, 0, 0 ,0)";
    } else if (typeName == "TextureInput") {
        returnValue = "null";
    } else if (typeName == "alias") {
        returnValue = "null";
    } else if (typeName == "Item") {
        returnValue = "null";
    }

    return returnValue;
}

} //internal namespace

namespace QmlDesigner {

namespace Internal {

QmlDesigner::PropertyName DynamicPropertiesModel::unusedProperty(const QmlDesigner::ModelNode &modelNode)
{
    QmlDesigner::PropertyName propertyName = "property";
    int i = 0;
    if (modelNode.isValid() && modelNode.metaInfo().isValid()) {
        while (true) {
            const QmlDesigner::PropertyName currentPropertyName = propertyName + QString::number(i).toLatin1();
            if (!modelNode.hasProperty(currentPropertyName) && !modelNode.metaInfo().hasProperty(currentPropertyName))
                return currentPropertyName;
            i++;
        }
    }

    return propertyName;
}

bool DynamicPropertiesModel::isValueType(const TypeName &type)
{
    // "variant" is considered value type as it is initialized as one.
    // This may need to change if we provide any kind of proper editor for it.
    static const QSet<TypeName> valueTypes {"int", "real", "color", "string", "bool", "url", "variant"};
    return valueTypes.contains(type);
}

QVariant DynamicPropertiesModel::defaultValueForType(const TypeName &type)
{
    QVariant value;
    if (type == "int")
        value = 0;
    else if (type == "real")
        value = 0.0;
    else if (type == "color")
        value = QColor(255, 255, 255);
    else if (type == "string")
        value = "This is a string";
    else if (type == "bool")
        value = false;
    else if (type == "url")
        value = "";
    else if (type == "variant")
        value = "";

    return value;
}

QString DynamicPropertiesModel::defaultExpressionForType(const TypeName &type)
{
    QString expression;
    if (type == "alias")
        expression = "null";
    else if (type == "TextureInput")
        expression = "null";
    else if (type == "vector2d")
        expression = "Qt.vector2d(0, 0)";
    else if (type == "vector3d")
        expression = "Qt.vector3d(0, 0, 0)";
    else if (type == "vector4d")
        expression = "Qt.vector4d(0, 0, 0 ,0)";

    return expression;
}

DynamicPropertiesModel::DynamicPropertiesModel(bool explicitSelection, AbstractView *parent)
    : QStandardItemModel(parent)
    , m_view(parent)
    , m_explicitSelection(explicitSelection)
{
    connect(this, &QStandardItemModel::dataChanged, this, &DynamicPropertiesModel::handleDataChanged);
}

void DynamicPropertiesModel::resetModel()
{
    beginResetModel();
    clear();
    setHorizontalHeaderLabels(
        QStringList({tr("Item"), tr("Property"), tr("Property Type"), tr("Property Value")}));

    if (m_view->isAttached()) {
        const auto nodes = selectedNodes();
        for (const ModelNode &modelNode : nodes)
            addModelNode(modelNode);
    }

    endResetModel();
}


//Method creates dynamic BindingProperty with the same name and type as old VariantProperty
//Value copying is optional
BindingProperty DynamicPropertiesModel::replaceVariantWithBinding(const PropertyName &name, bool copyValue)
{
    if (selectedNodes().count() == 1) {
        const ModelNode modelNode = selectedNodes().constFirst();
        if (modelNode.isValid()) {
            if (modelNode.hasVariantProperty(name)) {
                try {
                    VariantProperty vprop = modelNode.variantProperty(name);
                    TypeName oldType = vprop.dynamicTypeName();
                    QVariant oldValue = vprop.value();

                    modelNode.removeProperty(name);

                    BindingProperty bprop = modelNode.bindingProperty(name);
                    if (bprop.isValid()) {
                        if (copyValue)
                            bprop.setDynamicTypeNameAndExpression(oldType, oldValue.toString());
                        return bprop;
                    }
                } catch (RewritingException &e) {
                    m_exceptionError = e.description();
                    QTimer::singleShot(200, this, &DynamicPropertiesModel::handleException);
                }
            }
        }
    } else {
        qWarning() << "DynamicPropertiesModel::replaceVariantWithBinding: no selected nodes";
    }

    return BindingProperty();
}


//Finds selected property, and changes it to empty value (QVariant())
//If it's a BindingProperty, then replaces it with empty VariantProperty
void DynamicPropertiesModel::resetProperty(const PropertyName &name)
{
    if (selectedNodes().count() == 1) {
        const ModelNode modelNode = selectedNodes().constFirst();
        if (modelNode.isValid()) {
            if (modelNode.hasProperty(name)) {
                try {
                    AbstractProperty abProp = modelNode.property(name);

                    if (abProp.isVariantProperty()) {
                        VariantProperty property = abProp.toVariantProperty();
                        QVariant newValue = convertVariantForTypeName({}, property.dynamicTypeName());
                        property.setDynamicTypeNameAndValue(property.dynamicTypeName(),
                                                            newValue);
                    } else if (abProp.isBindingProperty()) {
                        BindingProperty property = abProp.toBindingProperty();
                        TypeName oldType = property.dynamicTypeName();

                        //removing old property, to create the new one with the same name:
                        modelNode.removeProperty(name);

                        VariantProperty newProperty = modelNode.variantProperty(name);
                        QVariant newValue = convertVariantForTypeName({}, oldType);
                        newProperty.setDynamicTypeNameAndValue(oldType, newValue);
                    }

                } catch (RewritingException &e) {
                    m_exceptionError = e.description();
                    QTimer::singleShot(200, this, &DynamicPropertiesModel::handleException);
                }
            }
        }
    }
    else {
        qWarning() << "DynamicPropertiesModel::resetProperty: no selected nodes";
    }
}

void DynamicPropertiesModel::dispatchPropertyChanges(const AbstractProperty &abstractProperty)
{
    if (abstractProperty.parentModelNode().simplifiedTypeName() == "PropertyChanges") {
        QmlPropertyChanges changes(abstractProperty.parentModelNode());
        if (changes.target().isValid()) {
            const ModelNode target = changes.target();
            const PropertyName propertyName = abstractProperty.name();
            const AbstractProperty targetProperty = target.variantProperty(propertyName);
            if (target.hasProperty(propertyName) && targetProperty.isDynamic())
                abstractPropertyChanged(targetProperty);
        }
    }
}

void DynamicPropertiesModel::bindingPropertyChanged(const BindingProperty &bindingProperty)
{
    if (!bindingProperty.isDynamic())
        return;

    m_handleDataChanged = false;

    const QList<ModelNode> nodes = selectedNodes();
    if (!nodes.contains(bindingProperty.parentModelNode()))
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

void DynamicPropertiesModel::abstractPropertyChanged(const AbstractProperty &property)
{
    if (!property.isDynamic())
        return;

    m_handleDataChanged = false;

    const QList<ModelNode> nodes = selectedNodes();
    if (!nodes.contains(property.parentModelNode()))
        return;
    int rowNumber = findRowForProperty(property);
    if (rowNumber > -1) {
        if (property.isVariantProperty())
            updateVariantProperty(rowNumber);
        else
            updateBindingProperty(rowNumber);
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::variantPropertyChanged(const VariantProperty &variantProperty)
{
    if (!variantProperty.isDynamic())
        return;

    m_handleDataChanged = false;

    const QList<ModelNode> nodes = selectedNodes();
    if (!nodes.contains(variantProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForVariantProperty(variantProperty);

        if (rowNumber == -1)
            addVariantProperty(variantProperty);
        else
            updateVariantProperty(rowNumber);
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::bindingRemoved(const BindingProperty &bindingProperty)
{
    m_handleDataChanged = false;

    const QList<ModelNode> nodes = selectedNodes();
    if (!nodes.contains(bindingProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForBindingProperty(bindingProperty);
        removeRow(rowNumber);
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::variantRemoved(const VariantProperty &variantProperty)
{
    m_handleDataChanged = false;

    const QList<ModelNode> nodes = selectedNodes();
    if (!nodes.contains(variantProperty.parentModelNode()))
        return;
    if (!m_lock) {
        int rowNumber = findRowForVariantProperty(variantProperty);
        removeRow(rowNumber);
    }

    m_handleDataChanged = true;
}

void DynamicPropertiesModel::reset()
{
    m_handleDataChanged = false;
    resetModel();
    m_handleDataChanged = true;
}

void DynamicPropertiesModel::setSelectedNode(const ModelNode &node)
{
    QTC_ASSERT(m_explicitSelection, return);
    QTC_ASSERT(node.isValid(), return);

    m_selectedNodes.clear();
    m_selectedNodes.append(node);
    reset();
}

AbstractProperty DynamicPropertiesModel::abstractPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2)
                                           .toString();

    if (!m_view->isAttached())
        return AbstractProperty();

    ModelNode modelNode = m_view->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.property(targetPropertyName.toUtf8());

    return AbstractProperty();
}

BindingProperty DynamicPropertiesModel::bindingPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = m_view->modelNodeForInternalId(internalId);

    if (modelNode.isValid())
        return modelNode.bindingProperty(targetPropertyName.toUtf8());

    return BindingProperty();
}

VariantProperty DynamicPropertiesModel::variantPropertyForRow(int rowNumber) const
{
    const int internalId = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 1).toInt();
    const QString targetPropertyName = data(index(rowNumber, TargetModelNodeRow), Qt::UserRole + 2).toString();

    ModelNode  modelNode = m_view->modelNodeForInternalId(internalId);

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
        for (const auto &property : metaInfo.properties()) {
            if (property.isWritable())
                possibleProperties.push_back(QString::fromUtf8(property.name()));
        }

        return possibleProperties;
    }

    return QStringList();
}

void DynamicPropertiesModel::addDynamicPropertyForCurrentNode()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_PROPERTY_ADDED);

    if (selectedNodes().count() == 1) {
        const ModelNode modelNode = selectedNodes().constFirst();
        if (modelNode.isValid()) {
            try {
                modelNode.variantProperty(unusedProperty(modelNode)).setDynamicTypeNameAndValue("string", "This is a string");
            } catch (RewritingException &e) {
                m_exceptionError = e.description();
                QTimer::singleShot(200, this, &DynamicPropertiesModel::handleException);
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

    NodeMetaInfo type;

    if (auto metaInfo = bindingProperty.parentModelNode().metaInfo(); metaInfo.isValid()) {
        type = metaInfo.property(bindingProperty.name()).propertyType();
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for target node";
    }

    const QString &id = stringlist.constFirst();

    ModelNode modelNode = getNodeByIdOrParent(id, bindingProperty.parentModelNode());

    if (!modelNode.isValid()) {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow invalid model node";
        return QStringList();
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();

    if (metaInfo.isValid())  {
        QStringList possibleProperties;
        for (const auto &property : metaInfo.properties()) {
            if (property.propertyType() == type) //### todo proper check
                possibleProperties.push_back(QString::fromUtf8(property.name()));
        }
        return possibleProperties;
    } else {
        qWarning() << " BindingModel::possibleSourcePropertiesForRow no meta info for source node";
    }

    return QStringList();
}

void DynamicPropertiesModel::deleteDynamicPropertyByRow(int rowNumber)
{
    m_view->executeInTransaction(
        "DynamicPropertiesModel::deleteDynamicPropertyByRow", [this, rowNumber]() {
            const AbstractProperty property = abstractPropertyForRow(rowNumber);
            const PropertyName propertyName = property.name();
            BindingProperty bindingProperty = bindingPropertyForRow(rowNumber);
            if (bindingProperty.isValid()) {
                bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
            } else {
                VariantProperty variantProperty = variantPropertyForRow(rowNumber);
                if (variantProperty.isValid())
                    variantProperty.parentModelNode().removeProperty(variantProperty.name());
            }

            if (property.isValid()) {
                QmlObjectNode objectNode = QmlObjectNode(property.parentModelNode());
                const auto stateOperations = objectNode.allAffectingStatesOperations();
                for (const QmlModelStateOperation &stateOperation : stateOperations) {
                    if (stateOperation.modelNode().hasProperty(propertyName))
                        stateOperation.modelNode().removeProperty(propertyName);
                }

                const auto timelineNodes = objectNode.allTimelines();
                for (auto &timelineNode : timelineNodes) {
                    QmlTimeline timeline(timelineNode);
                    timeline.removeKeyframesForTargetAndProperty(objectNode.modelNode(),
                                                                 propertyName);
                }
            }
        });

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

        const QmlObjectNode objectNode = QmlObjectNode(bindingProperty.parentModelNode());
        if (objectNode.isValid() && !objectNode.view()->currentState().isBaseState())
            value = objectNode.expression(bindingProperty.name());

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
        const QmlObjectNode objectNode = QmlObjectNode(variantProperty.parentModelNode());
        if (objectNode.isValid() && !objectNode.view()->currentState().isBaseState())
            value = objectNode.modelValue(variantProperty.name());


        updateDisplayRoleFromVariant(rowNumber, PropertyValueRow, value);
    }
}

void DynamicPropertiesModel::addModelNode(const ModelNode &modelNode)
{
    if (!modelNode.isValid())
        return;

    auto properties = modelNode.properties();

    auto dynamicProperties = Utils::filtered(properties, [](const AbstractProperty &p) {
        return p.isDynamic();
    });

    Utils::sort(dynamicProperties, [](const AbstractProperty &a, const AbstractProperty &b) {
        return a.name() < b.name();
    });

    for (const AbstractProperty &property : std::as_const(dynamicProperties)) {
        if (property.isBindingProperty())
            addBindingProperty(property.toBindingProperty());
        else if (property.isVariantProperty())
            addVariantProperty(property.toVariantProperty());
    }
}

void DynamicPropertiesModel::updateValue(int row)
{
    BindingProperty bindingProperty = bindingPropertyForRow(row);

    if (bindingProperty.isBindingProperty()) {
        const QString expression = data(index(row, PropertyValueRow)).toString();

        RewriterTransaction transaction = m_view->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updateValue"));
        try {
            bindingProperty.setDynamicTypeNameAndExpression(bindingProperty.dynamicTypeName(), expression);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) {
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, &DynamicPropertiesModel::handleException);
        }
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(row);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = data(index(row, PropertyValueRow));

        RewriterTransaction transaction = m_view->beginRewriterTransaction(QByteArrayLiteral("DynamicPropertiesModel::updateValue"));
        try {
            variantProperty.setDynamicTypeNameAndValue(variantProperty.dynamicTypeName(), value);
            transaction.commit(); //committing in the try block
        } catch (Exception &e) {
            m_exceptionError = e.description();
            QTimer::singleShot(200, this, &DynamicPropertiesModel::handleException);
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

    ModelNode targetNode = bindingProperty.parentModelNode();

    if (bindingProperty.isBindingProperty()) {
        m_view->executeInTransaction("DynamicPropertiesModel::updatePropertyName", [bindingProperty, newName, &targetNode](){
            const QString expression = bindingProperty.expression();
            const PropertyName dynamicPropertyType = bindingProperty.dynamicTypeName();

            targetNode.bindingProperty(newName).setDynamicTypeNameAndExpression(dynamicPropertyType, expression);
            targetNode.removeProperty(bindingProperty.name());
        });

        updateCustomData(rowNumber, targetNode.bindingProperty(newName));
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = variantProperty.value();
        const PropertyName dynamicPropertyType = variantProperty.dynamicTypeName();
        ModelNode targetNode = variantProperty.parentModelNode();

        m_view->executeInTransaction("DynamicPropertiesModel::updatePropertyName", [=](){
            targetNode.variantProperty(newName).setDynamicTypeNameAndValue(dynamicPropertyType, value);
            targetNode.removeProperty(variantProperty.name());
        });

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

        m_view->executeInTransaction("DynamicPropertiesModel::updatePropertyType", [=](){
            targetNode.removeProperty(bindingProperty.name());
            targetNode.bindingProperty(propertyName).setDynamicTypeNameAndExpression(newType, expression);
        });

        updateCustomData(rowNumber, targetNode.bindingProperty(propertyName));
        return;
    }

    VariantProperty variantProperty = variantPropertyForRow(rowNumber);

    if (variantProperty.isVariantProperty()) {
        const QVariant value = variantProperty.value();
        ModelNode targetNode = variantProperty.parentModelNode();
        const PropertyName propertyName = variantProperty.name();

        m_view->executeInTransaction("DynamicPropertiesModel::updatePropertyType", [=](){
            targetNode.removeProperty(variantProperty.name());
            if (!isValueType(newType)) {
                targetNode.bindingProperty(propertyName).setDynamicTypeNameAndExpression(
                            newType, convertVariantForTypeName({}, newType).toString());
            } else {
                targetNode.variantProperty(propertyName).setDynamicTypeNameAndValue(
                            newType, convertVariantForTypeName(value, newType));
            }
        });

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
        modelNode = m_view->modelNodeForId(id);
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

int DynamicPropertiesModel::findRowForProperty(const AbstractProperty &abstractProperty) const
{
    for (int i = 0; i < rowCount(); i++) {
        if ((abstractPropertyForRow(i).name() == abstractProperty.name()))
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

        *sourceNode = stringList.constFirst();

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
    QMessageBox::warning(nullptr, tr("Error"), m_exceptionError);
    resetModel();
}

const QList<ModelNode> DynamicPropertiesModel::selectedNodes() const
{
    // If selected nodes are explicitly set, return those.
    // Otherwise return actual selected nodes of the model.
    if (m_explicitSelection)
        return m_selectedNodes;
    else
        return m_view->selectedModelNodes();
}

const ModelNode DynamicPropertiesModel::singleSelectedNode() const
{
    if (m_explicitSelection)
        return m_selectedNodes.first();
    else
        return m_view->singleSelectedModelNode();
}

} // namespace Internal

} // namespace QmlDesigner
