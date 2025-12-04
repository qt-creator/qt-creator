// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingproperty.h"
#include "nodeproperty.h"

#include "internalnode_p.h"
#include "model_p.h"

#include <qmldesignerutils/stringutils.h>

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
{}

void BindingProperty::setExpression(const QString &expression, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property set expression",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return;

    Internal::WriteLocker locker(model());

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

const constinit QString null;

const QString &BindingProperty::expression(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property expression",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (isValid()) {
        if (auto property = internalNode()->bindingProperty(name()))
            return property->expression();
    }

    return null;
}

ModelNode BindingProperty::resolveBinding(QStringView binding, ModelNode currentNode) const
{
    int index = 0;
    auto elements = binding.split(QLatin1Char('.'));
    QStringView element = elements.front();
    while (!element.isEmpty()) {
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
            if (std::cmp_less(index, elements.size()))
                element = elements[index];
            else
                element = {};

        } else {
            return ModelNode();
        }
    }
    return currentNode;
}

ModelNode BindingProperty::resolveToModelNode(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property resolve to model node",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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

AbstractProperty BindingProperty::resolveToProperty(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property resolve to proprety",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return {};

    QStringView binding = expression();

    if (binding.isEmpty())
        return {};

    ModelNode node = parentModelNode();
#if 0
    // This is from before the qmldesigner merge
    QString element;
    if (binding.contains(QLatin1Char('.'))) {
        element = binding.split(QLatin1Char('.')).constLast();
        QString nodeBinding = binding;
        nodeBinding.chop(element.size());
        node = resolveBinding(nodeBinding, parentModelNode());
    } else {
        element = binding;
    }
#endif

    auto [nodeBinding, lastElement] = StringUtils::split_last(binding, u'.');
    if (nodeBinding.size())
        node = resolveBinding(nodeBinding, node);

    if (node.isValid() && !lastElement.contains(' '))
        return node.property(lastElement.toUtf8());

    return {};
}

bool BindingProperty::isList(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property is list",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    QStringView expression = this->expression();

    return expression.startsWith('[') && expression.endsWith(']');
}

QList<ModelNode> BindingProperty::resolveListToModelNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property reolve list to model nodes",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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
                returnList.emplace_back(internalNode, model(), view());
        }
    }
    return returnList;
}

QList<ModelNode> BindingProperty::resolveToModelNodes(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property resolve to model nodes",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    QString binding = expression();

    if (binding.isEmpty())
        return {};

    if (isList()) {
        QList<ModelNode> returnList;
        binding.chop(1);
        binding.remove(0, 1);
        const QStringList simplifiedList = commaSeparatedSimplifiedStringList(binding);
        for (const QString &nodeId : simplifiedList) {
            if (auto internalNode = privateModel()->nodeForId(nodeId))
                returnList.emplace_back(internalNode, model(), view());
        }
        return returnList;
    } else if (auto node = resolveBinding(binding, parentModelNode())) {
        return {node};
    }

    return {};
}

void BindingProperty::addModelNodeToArray(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property add model node to array",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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

void BindingProperty::removeModelNodeFromArray(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property remove model node from array",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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

QList<BindingProperty> BindingProperty::findAllReferencesTo(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property find all references to",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!modelNode.isValid())
        return {};

    QList<BindingProperty> list;
    for (const ModelNode &bindingNode : modelNode.view()->allModelNodes()) {
        for (const BindingProperty &bindingProperty : bindingNode.bindingProperties()) {
            if (bindingProperty.resolveToModelNodes().contains(modelNode))
                list.append(bindingProperty);
        }
    }
    return list;
}

void BindingProperty::deleteAllReferencesTo(const ModelNode &modelNode, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property delete all references to",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    for (BindingProperty &bindingProperty : findAllReferencesTo(modelNode)) {
        if (bindingProperty.isList())
            bindingProperty.removeModelNodeFromArray(modelNode);
        else
            bindingProperty.parentModelNode().removeProperty(bindingProperty.name());
    }
}

bool BindingProperty::canBeReference(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property can be reference",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    return !name().startsWith("anchors.");
}

bool BindingProperty::isAlias(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property is alias",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

    if (!isValid())
        return false;

    return isDynamic() && dynamicTypeName() == "alias" && !expression().isNull()
           && !expression().isEmpty()
           && parentModelNode().view()->modelNodeForId(expression()).isValid();
}

bool BindingProperty::isAliasExport(SL sl) const
{
    NanotraceHR::Tracer tracer{"binding property is alias export",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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

QVariant BindingProperty::convertToLiteral(const TypeName &typeName, const QString &testExpression, SL sl)
{
    NanotraceHR::Tracer tracer{"binding property convert to literal",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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
    } else if ("QVariant" == typeName || "variant" == typeName || "var" == typeName) {
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

void BindingProperty::setDynamicTypeNameAndExpression(const TypeName &typeName,
                                                      const QString &expression,
                                                      SL sl)
{
    NanotraceHR::Tracer tracer{"binding property set dynamic type name and expression",
                               ModelTracing::category(),
                               keyValue("caller location", sl)};

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
        return debug.nospace() << "BindingProperty(" << "invalid" << ')';
    else
        return debug.nospace() << "BindingProperty(" <<  property.name() << " " << property.expression() << ')';
}

QTextStream& operator<<(QTextStream &stream, const BindingProperty &property)
{
    if (!property.isValid())
        stream << "BindingProperty(" << "invalid" << ')';
    else
        stream << "BindingProperty(" << property.name().toByteArray() << " "
               << property.expression() << ')';

    return stream;
}

} // namespace QmlDesigner
