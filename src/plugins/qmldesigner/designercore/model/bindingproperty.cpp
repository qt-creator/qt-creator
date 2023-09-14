// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingproperty.h"
#include "nodeproperty.h"
#include "internalproperty.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"

using namespace Qt::StringLiterals;

namespace QmlDesigner {

bool compareBindingProperties(const QmlDesigner::BindingProperty &bindingProperty01, const QmlDesigner::BindingProperty &bindingProperty02)
{
    if (bindingProperty01.parentModelNode() != bindingProperty02.parentModelNode())
        return false;
    if (bindingProperty01.name() != bindingProperty02.name())
        return false;
    return true;
}

BindingProperty::BindingProperty() = default;

BindingProperty::BindingProperty(const BindingProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNodeSharedPointer(), property.model(), view)
{
}


BindingProperty::BindingProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}


void BindingProperty::setExpression(const QString &expression)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (isDynamic())
        qWarning() << "Calling BindingProperty::setExpression on dynamic property.";

    if (name() == "id")
        return;

    if (expression.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto bindingProperty = internalProperty->to<PropertyType::Binding>();
        //check if oldValue != value
        if (bindingProperty && bindingProperty->expression() == expression)
            return;

        if (!bindingProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setBindingProperty(internalNodeSharedPointer(), name(), expression);
}

QString BindingProperty::expression() const
{
    if (isValid()) {
        if (auto property = internalNode()->bindingProperty(name()))
            return property->expression();
    }

    return QString();
}

ModelNode BindingProperty::resolveBinding(const QString &binding, ModelNode currentNode) const
{
    int index = 0;
    QString element = binding.split(QLatin1Char('.')).at(0);
    while (!element.isEmpty())
    {
        if (currentNode.isValid()) {
            if (element == QLatin1String("parent")) {
                if (currentNode.hasParentProperty())
                    currentNode = currentNode.parentProperty().toNodeAbstractProperty().parentModelNode();
                else
                    return ModelNode(); //binding not valid
            } else if (currentNode.hasProperty(element.toUtf8())) {
                if (currentNode.property(element.toUtf8()).isNodeProperty())
                    currentNode = currentNode.nodeProperty(element.toUtf8()).modelNode();
                else
                    return ModelNode(privateModel()->nodeForId(element), model(), view());

            } else {
                currentNode = ModelNode(privateModel()->nodeForId(element), model(), view());
            }
            index++;
            if (index < binding.split(QLatin1Char('.')).size())
                element = binding.split(QLatin1Char('.')).at(index);
            else
                element.clear();

        } else {
            return ModelNode();
        }
    }
    return currentNode;
}

ModelNode BindingProperty::resolveToModelNode() const
{
    if (!isValid())
        return {};

    QString binding = expression();

    if (binding.isEmpty())
        return {};

    return resolveBinding(binding, parentModelNode());
}

inline static QStringList commaSeparatedSimplifiedStringList(const QString &string)
{
    const QStringList stringList = string.split(","_L1);
    QStringList simpleList;
    for (const QString &simpleString : stringList)
        simpleList.append(simpleString.simplified());
    return simpleList;
}


AbstractProperty BindingProperty::resolveToProperty() const
{
    if (!isValid())
        return {};

    QString binding = expression();

    if (binding.isEmpty())
        return {};

    ModelNode node = parentModelNode();
    QString element;
    if (binding.contains(QLatin1Char('.'))) {
        element = binding.split(QLatin1Char('.')).constLast();
        QString nodeBinding = binding;
        nodeBinding.chop(element.length());
        node = resolveBinding(nodeBinding, parentModelNode());
    } else {
        element = binding;
    }

    if (node.isValid() && !element.contains(' '))
        return node.property(element.toUtf8());
    else
        return AbstractProperty();
}

bool BindingProperty::isList() const
{
    if (!isValid())
        return false;

    return expression().startsWith('[') && expression().endsWith(']');
}

QList<ModelNode> BindingProperty::resolveToModelNodeList() const
{
    if (!isValid())
        return {};

    QString binding = expression();

    if (binding.isEmpty())
        return {};

    QList<ModelNode> returnList;
    if (isList()) {
        binding.chop(1);
        binding.remove(0, 1);
        const QStringList simplifiedList = commaSeparatedSimplifiedStringList(binding);
        for (const QString &nodeId : simplifiedList) {
            if (auto internalNode = privateModel()->nodeForId(nodeId))
                returnList.append(ModelNode{internalNode, model(), view()});
        }
    }
    return returnList;
}

void BindingProperty::addModelNodeToArray(const ModelNode &modelNode)
{
    if (!isValid())
        return;

    if (isBindingProperty()) {
        QStringList simplifiedList;
        if (isList()) {
            QString string = expression();
            string.chop(1);
            string.remove(0, 1);
            simplifiedList = commaSeparatedSimplifiedStringList(string);
        } else {
            ModelNode currentNode = resolveToModelNode();
            if (currentNode.isValid())
                simplifiedList.append(currentNode.validId());
        }
        ModelNode node = modelNode;
        simplifiedList.append(node.validId());
        setExpression('[' + simplifiedList.join(',') + ']');
    } else if (exists()) {
        return;
    } else {
        ModelNode node = modelNode;
        setExpression('[' + node.validId() + ']');
    }
}

void BindingProperty::removeModelNodeFromArray(const ModelNode &modelNode)
{
    if (!isBindingProperty())
        return;

    if (isList() && modelNode.hasId()) {
        QString string = expression();
        string.chop(1);
        string.remove(0, 1);
        QStringList simplifiedList = commaSeparatedSimplifiedStringList(string);
        if (simplifiedList.contains(modelNode.id())) {
            simplifiedList.removeAll(modelNode.id());
            if (simplifiedList.isEmpty())
                parentModelNode().removeProperty(name());
            else
                setExpression('[' + simplifiedList.join(',') + ']');
        }
    }
}

QList<BindingProperty> BindingProperty::findAllReferencesTo(const ModelNode &modelNode)
{
    if (!modelNode.isValid())
        return {};

    QList<BindingProperty> list;
    for (const ModelNode &bindingNode : modelNode.view()->allModelNodes()) {
        for (const BindingProperty &bindingProperty : bindingNode.bindingProperties())
            if (bindingProperty.resolveToModelNode() == modelNode)
                list.append(bindingProperty);
            else if (bindingProperty.resolveToModelNodeList().contains(modelNode))
                list.append(bindingProperty);
    }
    return list;
}

void BindingProperty::deleteAllReferencesTo(const ModelNode &modelNode)
{
    for (BindingProperty &bindingProperty : findAllReferencesTo(modelNode)) {
        if (bindingProperty.isList())
            bindingProperty.removeModelNodeFromArray(modelNode);
        else
            bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
    }
}

bool BindingProperty::isAlias() const
{
    if (!isValid())
        return false;

    return isDynamic() && dynamicTypeName() == "alias" && !expression().isNull()
           && !expression().isEmpty()
           && parentModelNode().view()->modelNodeForId(expression()).isValid();
}

bool BindingProperty::isAliasExport() const
{
    if (!isValid())
        return false;
    return parentModelNode() == parentModelNode().model()->rootModelNode() && isDynamic()
           && dynamicTypeName() == "alias" && name() == expression().toUtf8()
           && parentModelNode().model()->modelNodeForId(expression()).isValid();
}

static bool isTrueFalseLiteral(const QString &expression)
{
    return (expression.compare("false", Qt::CaseInsensitive) == 0)
           || (expression.compare("true", Qt::CaseInsensitive) == 0);
}

QVariant BindingProperty::convertToLiteral(const TypeName &typeName, const QString &testExpression)
{
    if ("QColor" == typeName || "color" == typeName) {
        QString unquoted = testExpression;
        unquoted.remove('"');
        if (QColor(unquoted).isValid())
            return QColor(unquoted);
    } else if ("bool" == typeName) {
        if (isTrueFalseLiteral(testExpression)) {
            if (testExpression.compare("true", Qt::CaseInsensitive) == 0)
                return true;
            else
                return false;
        }
    } else if ("int" == typeName) {
        bool ok;
        int intValue = testExpression.toInt(&ok);
        if (ok)
            return intValue;
    } else if ("qreal" == typeName || "real" == typeName) {
        bool ok;
        qreal realValue = testExpression.toDouble(&ok);
        if (ok)
            return realValue;
    } else if ("QVariant" == typeName || "variant" == typeName) {
        bool ok;
        qreal realValue = testExpression.toDouble(&ok);
        if (ok) {
            return realValue;
        } else if (isTrueFalseLiteral(testExpression)) {
            if (testExpression.compare("true", Qt::CaseInsensitive) == 0)
                return true;
            else
                return false;
        }
    }

    return {};
}

void BindingProperty::setDynamicTypeNameAndExpression(const TypeName &typeName, const QString &expression)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        return;

    if (name() == "id")
        return;

    if (expression.isEmpty())
        return;

    if (typeName.isEmpty())
        return;

    if (auto internalProperty = internalNode()->property(name())) {
        auto bindingProperty = internalProperty->to<PropertyType::Binding>();
        //check if oldValue != value
        if (bindingProperty && bindingProperty->expression() == expression
            && internalProperty->dynamicTypeName() == typeName) {
            return;
        }

        if (!bindingProperty)
            privateModel()->removePropertyAndRelatedResources(internalProperty);
    }

    privateModel()->setDynamicBindingProperty(internalNodeSharedPointer(), name(), typeName, expression);
}

QDebug operator<<(QDebug debug, const BindingProperty &property)
{
    if (!property.isValid())
        return debug.nospace() << "BindingProperty(" << PropertyName("invalid") << ')';
    else
        return debug.nospace() << "BindingProperty(" <<  property.name() << " " << property.expression() << ')';
}

QTextStream& operator<<(QTextStream &stream, const BindingProperty &property)
{
    if (!property.isValid())
        stream << "BindingProperty(" << PropertyName("invalid") << ')';
    else
        stream << "BindingProperty(" <<  property.name() << " " << property.expression() << ')';

    return stream;
}

} // namespace QmlDesigner
