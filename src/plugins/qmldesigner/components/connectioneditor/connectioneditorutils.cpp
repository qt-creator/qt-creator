// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "connectioneditorutils.h"

#include <abstractproperty.h>
#include <abstractview.h>
#include <bindingproperty.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodemetainfo.h>
#include <rewriterview.h>
#include <rewritingexception.h>
#include <type_traits>
#include <variantproperty.h>
#include <qmldesignertr.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>

namespace QmlDesigner {

void callLater(const std::function<void()> &fun)
{
    QTimer::singleShot(0, fun);
}

void showErrorMessage(const QString &text)
{
    callLater([text]() { QMessageBox::warning(nullptr, Tr::tr("Error"), text); });
}

QString idOrTypeName(const ModelNode &modelNode)
{
    QString idLabel = modelNode.id();
    if (idLabel.isEmpty())
        idLabel = modelNode.simplifiedTypeName();
    return idLabel;
}

PropertyName uniquePropertyName(const PropertyName &suggestion, const ModelNode &modelNode)
{
    PropertyName name = suggestion;
    if (!modelNode.isValid() || !modelNode.metaInfo().isValid())
        return name;

    int i = 0;
    while (true) {
        if (!modelNode.hasProperty(name) && !modelNode.metaInfo().hasProperty(name))
            return name;
        name = suggestion + QString::number(i++).toLatin1();
    }
    return {};
}

NodeMetaInfo dynamicTypeNameToNodeMetaInfo(const TypeName &typeName, Model *model)
{
    // Note: Uses old mechanism to create the NodeMetaInfo and supports
    // only types we care about in the connection editor.
    // TODO: Support all possible AbstractProperty types and move to the
    // AbstractProperty class.
    if (typeName == "bool")
        return model->boolMetaInfo();
    else if (typeName == "int")
        return model->metaInfo("QML.int");
    else if (typeName == "real")
        return model->metaInfo("QML.real");
    else if (typeName == "color")
        return model->metaInfo("QML.color");
    else if (typeName == "string")
        return model->metaInfo("QML.string");
    else if (typeName == "url")
        return model->metaInfo("QML.url");
    else if (typeName == "variant")
        return model->metaInfo("QML.variant");
    else
        qWarning() << __FUNCTION__ << " type " << typeName << "not found";
    return {};
}

NodeMetaInfo dynamicTypeMetaInfo(const AbstractProperty &property)
{
    return dynamicTypeNameToNodeMetaInfo(property.dynamicTypeName(), property.model());
}

bool metaInfoIsCompatibleUnsafe(const NodeMetaInfo &target, const NodeMetaInfo &source)
{
    if (target.isVariant())
        return true;

    if (target == source)
        return true;

    if (target.isBool() && source.isBool())
        return true;

    if (target.isNumber() && source.isNumber())
        return true;

    if (target.isString() && source.isString())
        return true;

    if (target.isUrl() && source.isUrl())
        return true;

    if (target.isColor() && source.isColor())
        return true;

    return false;
}

bool metaInfoIsCompatible(const NodeMetaInfo &targetType, const PropertyMetaInfo &sourceInfo)
{
    NodeMetaInfo sourceType = sourceInfo.propertyType();
    return metaInfoIsCompatibleUnsafe(targetType, sourceType);
}

QVariant typeConvertVariant(const QVariant &variant, const QmlDesigner::TypeName &typeName)
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
        if (QColor::isValidColor(variant.toString()))
            returnValue = variant.toString();
        else
            returnValue = QColor(Qt::black);
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

template<typename T>
void convertPropertyType(const T &property, const QVariant &value)
{
    if (!property.isValid())
        return;

    ModelNode node = property.parentModelNode();
    if (!node.isValid())
        return;

    PropertyName name = property.name();
    TypeName type = property.dynamicTypeName();
    node.removeProperty(name);

    if constexpr (std::is_same_v<T, VariantProperty>) {
        BindingProperty newProperty = node.bindingProperty(name);
        if (newProperty.isValid())
            newProperty.setDynamicTypeNameAndExpression(type, value.toString());
    } else if constexpr (std::is_same_v<T, BindingProperty>) {
        VariantProperty newProperty = node.variantProperty(name);
        if (newProperty.isValid())
            newProperty.setDynamicTypeNameAndValue(type, value);
    }
}

void convertVariantToBindingProperty(const VariantProperty &property, const QVariant &value)
{
    convertPropertyType(property, value);
}

void convertBindingToVariantProperty(const BindingProperty &property, const QVariant &value)
{
    convertPropertyType(property, value);
}

bool isBindingExpression(const QVariant &value)
{
    if (value.metaType().id() != QMetaType::QString)
        return false;

    QRegularExpression regexp("^[a-z_]\\w*|^[A-Z]\\w*\\.{1}([a-z_]\\w*\\.?)+");
    QRegularExpressionMatch match = regexp.match(value.toString());
    return match.hasMatch();
}

bool isDynamicVariantPropertyType(const TypeName &type)
{
    // "variant" is considered value type as it is initialized as one.
    // This may need to change if we provide any kind of proper editor for it.
    static const QSet<TypeName> valueTypes{"int", "real", "color", "string", "bool", "url", "variant"};
    return valueTypes.contains(type);
}

QVariant defaultValueForType(const TypeName &type)
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

QString defaultExpressionForType(const TypeName &type)
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

std::pair<QString, QString> splitExpression(const QString &expression)
{
    // ### Todo from original code (getExpressionStrings):
    //      We assume no expressions yet
    const QStringList stringList = expression.split(QLatin1String("."));

    QString sourceNode = stringList.constFirst();
    QString propertyName;
    for (int i = 1; i < stringList.size(); ++i) {
        propertyName += stringList.at(i);
        if (i != stringList.size() - 1)
            propertyName += QLatin1String(".");
    }
    if (propertyName.isEmpty())
        std::swap(sourceNode, propertyName);

    return {sourceNode, propertyName};
}

QStringList singletonsFromView(AbstractView *view)
{
    RewriterView *rv = view->rewriterView();
    if (!rv)
        return {};

    QStringList out;
    for (const QmlTypeData &data : rv->getQMLTypes()) {
        if (data.isSingleton && !data.typeName.isEmpty())
            out.push_back(data.typeName);
    }
    return out;
}

std::vector<PropertyMetaInfo> propertiesFromSingleton(const QString &name, AbstractView *view)
{
    Model *model = view->model();
    QTC_ASSERT(model, return {});

    if (NodeMetaInfo metaInfo = model->metaInfo(name.toUtf8()); metaInfo.isValid())
        return metaInfo.properties();

    return {};
}

QList<AbstractProperty> dynamicPropertiesFromNode(const ModelNode &node)
{
    auto isDynamic = [](const AbstractProperty &p) { return p.isDynamic(); };
    auto byName = [](const AbstractProperty &a, const AbstractProperty &b) {
        return a.name() < b.name();
    };

    QList<AbstractProperty> dynamicProperties = Utils::filtered(node.properties(), isDynamic);
    Utils::sort(dynamicProperties, byName);
    return dynamicProperties;
}

QStringList availableSources(AbstractView *view)
{
    QStringList sourceNodes;
    for (const ModelNode &modelNode : view->allModelNodes()) {
        if (!modelNode.id().isEmpty())
            sourceNodes.append(modelNode.id());
    }
    std::sort(sourceNodes.begin(), sourceNodes.end());
    return singletonsFromView(view) + sourceNodes;
}

QStringList availableTargetProperties(const BindingProperty &bindingProperty)
{
    const ModelNode modelNode = bindingProperty.parentModelNode();
    if (!modelNode.isValid()) {
        qWarning() << __FUNCTION__ << " invalid model node";
        return {};
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();
    if (metaInfo.isValid()) {
        const auto properties = metaInfo.properties();
        QStringList writableProperties;
        writableProperties.reserve(static_cast<int>(properties.size()));
        for (const auto &property : properties) {
            if (property.isWritable())
                writableProperties.push_back(QString::fromUtf8(property.name()));
        }

        return writableProperties;
    }
    return {};
}

ModelNode getNodeByIdOrParent(AbstractView *view, const QString &id, const ModelNode &targetNode)
{
    if (id != QLatin1String("parent"))
        return view->modelNodeForId(id);

    if (targetNode.hasParentProperty())
        return targetNode.parentProperty().parentModelNode();

    return {};
}

QStringList availableSourceProperties(const QString &id,
                                      const BindingProperty &targetProperty,
                                      AbstractView *view)
{
    ModelNode modelNode = getNodeByIdOrParent(view, id, targetProperty.parentModelNode());

    NodeMetaInfo targetType;
    if (targetProperty.isDynamic()) {
        targetType = dynamicTypeMetaInfo(targetProperty);
    } else if (auto metaInfo = targetProperty.parentModelNode().metaInfo(); metaInfo.isValid()) {
        targetType = metaInfo.property(targetProperty.name()).propertyType();
    } else
        qWarning() << __FUNCTION__ << " no meta info for target node";

    QStringList possibleProperties;
    if (!modelNode.isValid()) {
        QStringList singletons = singletonsFromView(view);
        if (singletons.contains(id)) {
            for (const auto &property : propertiesFromSingleton(id, view)) {
                if (metaInfoIsCompatible(targetType, property))
                    possibleProperties.push_back(QString::fromUtf8(property.name()));
            }
            return possibleProperties;
        }
        qWarning() << __FUNCTION__ << " invalid model node: " << id;
        return {};
    }

    auto isCompatible = [targetType](const AbstractProperty &other) {
        auto otherType = dynamicTypeMetaInfo(other);
        return metaInfoIsCompatibleUnsafe(targetType, otherType);
    };

    for (const VariantProperty &variantProperty : modelNode.variantProperties()) {
        if (variantProperty.isDynamic() && isCompatible(variantProperty))
            possibleProperties << QString::fromUtf8(variantProperty.name());
    }

    for (const BindingProperty &bindingProperty : modelNode.bindingProperties()) {
        if (bindingProperty.isDynamic() && isCompatible(bindingProperty))
            possibleProperties << QString::fromUtf8((bindingProperty.name()));
    }

    NodeMetaInfo metaInfo = modelNode.metaInfo();
    if (metaInfo.isValid()) {
        for (const auto &property : metaInfo.properties()) {
            if (metaInfoIsCompatible(targetType, property))
                possibleProperties.push_back(QString::fromUtf8(property.name()));
        }
    } else {
        qWarning() << __FUNCTION__ << " no meta info for source node";
    }

    return possibleProperties;
}

} // namespace QmlDesigner
