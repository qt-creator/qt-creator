/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "abstractproperty.h"
#include "internalnode_p.h"
#include "internalproperty.h"
#include <model.h>
#include "model_p.h"
#include <modelnode.h>
#include <metainfo.h>
#include "invalidpropertyexception.h"
#include "variantproperty.h"
#include "bindingproperty.h"
#include "nodeproperty.h"
#include "nodeabstractproperty.h"
#include "nodelistproperty.h"
#include <QTextStream>
#include <qmlobjectnode.h>

namespace QmlDesigner {

 /*!
\class QmlDesigner::AbstractProperty
\ingroup CoreModel
\brief AbstractProperty is a value holder for a property
*/

AbstractProperty::AbstractProperty():
        m_internalNode(new Internal::InternalNode)
{
}

AbstractProperty::AbstractProperty(const QString &propertyName, const Internal::InternalNodePointer  &internalNode, Model* model,  AbstractView *view)
    : m_propertyName(propertyName),
    m_internalNode(internalNode),
    m_model(model),
    m_view(view)
{
    Q_ASSERT(!m_model || m_view);
}

AbstractProperty::AbstractProperty(const Internal::InternalPropertyPointer &property, Model* model,  AbstractView *view)
        : m_propertyName(property->name()),
        m_internalNode(property->propertyOwner()),
        m_model(model),
        m_view(view)
{
    Q_ASSERT(!m_model || m_view);
}

AbstractProperty::AbstractProperty(const AbstractProperty &property, AbstractView *view)
    : m_propertyName(property.name()),
      m_internalNode(property.internalNode()),
      m_model(property.model()),
      m_view(view)
{

}

AbstractProperty::~AbstractProperty()
{
}

AbstractProperty::AbstractProperty(const AbstractProperty &other)
    :  m_propertyName(other.m_propertyName),
    m_internalNode(other.m_internalNode),
    m_model(other.m_model),
    m_view(other.m_view)
{
}

AbstractProperty& AbstractProperty::operator=(const AbstractProperty &other)
{
    m_propertyName = other.m_propertyName;
    m_internalNode = other.m_internalNode;
    m_model = other.m_model;
    m_view = other.m_view;

    return *this;
}

Internal::InternalNodePointer AbstractProperty::internalNode() const
{
    return m_internalNode;
}

Model *AbstractProperty::model() const
{
    return m_model.data();
}

AbstractView *AbstractProperty::view() const
{
    return m_view.data();
}

 /*!
 \brief AbstractProperty is a value holder for a property
 \return the value of the property

 The QVariant is null if the property doesn't exists.
*/
QString AbstractProperty::name() const
{
    if (m_propertyName == "id") { // the ID for a node is independent of the state, so it has to be set with ModelNode::setId
        Q_ASSERT_X(0, Q_FUNC_INFO, "id is not a property in the model");
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, name());
    }
    return m_propertyName;
}

 /*!
 \brief Checks if the property is valid.

 A prooperty is valid if the belonging ModelNode
 is valid. This function is not overloaded for
 subclasses.

 \return the property is valid for true return value
*/
bool AbstractProperty::isValid() const
{
    return !m_internalNode.isNull() &&
            !m_model.isNull() &&
            m_internalNode->isValid() &&
            !m_propertyName.isEmpty();
}

//bool AbstractProperty::isValueAPropertyBinding() const
//{
//    const QVariant propertyValue = value();
//
//    return propertyValue.type() == QVariant::UserType && propertyValue.userType() == qMetaTypeId<QmlDesigner::PropertyBinding>();
//}
//
//PropertyBinding AbstractProperty::valueToPropertyBinding() const
//{
//    if (isValueAPropertyBinding())
//        return value().value<PropertyBinding>();
//    else
//        return PropertyBinding();
//}
//
//bool AbstractProperty::isValueAModelNode() const
//{
//    const QVariant propertyValue = value();
//
//    return propertyValue.type() == QVariant::UserType && propertyValue.userType() == ModelNode::variantUserType();
//}
//
//ModelNode AbstractProperty::valueToModelNode() const
//{
//    if (isValueAModelNode())
//        return value().value<ModelNode>();
//    else
//        return ModelNode();
//}
//
//bool AbstractProperty::isValueAList() const
//{
//    const QVariant propertyValue = value();
//
//    return propertyValue.type() == QVariant::List;
//}
//
//QVariantList AbstractProperty::valueToList() const
//{
//    if (isValueAList())
//        return value().toList();
//    else
//        return QVariantList();
//}
//
//ModelNode AbstractProperty::addModelNodeToValueList(const QString &type, int majorVersion, int minorVersion, const QList<QPair<QString, QVariant> > &propertyList)
//{
//    //if (isValueAList())
//    //    return m_model->addModelNode(state(), m_propertyName, type, majorVersion, minorVersion, propertyList);
//    //else
//     //   throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);
//    return ModelNode();
//}

 /*!
 \brief returns the ModelNode to which the property belongs
 \return node to which the property belongs
*/
ModelNode AbstractProperty::parentModelNode() const
{
    return ModelNode(m_internalNode, m_model.data(), view());
}

/*!
\brief returns the QmlObjectNode to which the property belongs
\return node to which the property belongs
*/
QmlObjectNode AbstractProperty::parentQmlObjectNode() const
{
    return QmlObjectNode(parentModelNode());
}
/*!
  \brief returns whether the property is the default property for the model node.
*/
bool AbstractProperty::isDefaultProperty() const
{
    return ModelNode(m_internalNode, m_model.data(), view()).metaInfo().defaultPropertyName() == m_propertyName;
}

VariantProperty AbstractProperty::toVariantProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    VariantProperty propertyVariant(name(), internalNode(), model(), view());

    if (propertyVariant.isVariantProperty())
        return propertyVariant;

    return VariantProperty();
}

NodeProperty AbstractProperty::toNodeProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    NodeProperty propertyNode(name(), internalNode(), model(), view());

    if (propertyNode.isNodeProperty())
        return propertyNode;

    return NodeProperty();
}

NodeListProperty AbstractProperty::toNodeListProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    NodeListProperty propertyNodeList(name(), internalNode(), model(), view());

    if (propertyNodeList.isNodeListProperty())
        return propertyNodeList;

    return NodeListProperty();
}

NodeAbstractProperty AbstractProperty::toNodeAbstractProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    NodeAbstractProperty propertyNodeAbstract(name(), internalNode(), model(), view());

    if (propertyNodeAbstract.isNodeAbstractProperty())
        return propertyNodeAbstract;

    return NodeAbstractProperty();
}

BindingProperty AbstractProperty::toBindingProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    BindingProperty propertyBinding(name(), internalNode(), model(), view());

    if (propertyBinding.isBindingProperty())
        return propertyBinding;

    return BindingProperty();
}

bool AbstractProperty::isVariantProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name())) {
        Q_ASSERT(internalNode()->property(name()));
        return internalNode()->property(name())->isVariantProperty();
    }

    return false;
}

bool AbstractProperty::isNodeAbstractProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name())) {
        Q_ASSERT(internalNode()->property(name()));
        return internalNode()->property(name())->isNodeAbstractProperty();
    }

    return false;
}

bool AbstractProperty::isNodeListProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name())) {
        Q_ASSERT(internalNode()->property(name()));
        return internalNode()->property(name())->isNodeListProperty();
    }

    return false;
}

bool AbstractProperty::isNodeProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name())) {
        Q_ASSERT(internalNode()->property(name()));
        return internalNode()->property(name())->isNodeProperty();
    }

    return false;
}


bool AbstractProperty::isBindingProperty() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name())) {
        Q_ASSERT(internalNode()->property(name()));
        return internalNode()->property(name())->isBindingProperty();
    }

    return false;
}

bool AbstractProperty::isDynamic() const
{
    return !dynamicTypeName().isEmpty();
}

QString AbstractProperty::dynamicTypeName() const
{
    if (!isValid())
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, m_propertyName);

    if (internalNode()->hasProperty(name()))
        return internalNode()->property(name())->dynamicTypeName();

    return QString();
}

/*!
  \brief Returns if the the two property handles reference the same property in the same node
*/
bool operator ==(const AbstractProperty &property1, const AbstractProperty &property2)
{
    return (property1.m_model == property2.m_model)
            && (property1.m_internalNode == property2.m_internalNode)
            && (property1.m_propertyName == property2.m_propertyName);
}

/*!
  \brief Returns if the the two property handles do not reference the same property in the same node
  */
bool operator !=(const AbstractProperty &property1, const AbstractProperty &property2)
{
    return !(property1 == property2);
}

uint qHash(const AbstractProperty &property)
{
    //### to do
    return ::qHash(property.m_internalNode.data())
            ^ ::qHash(property.m_propertyName);
}

QDebug operator<<(QDebug debug, const AbstractProperty &property)
{
    return debug.nospace() << "AbstractProperty(" << (property.isValid() ? property.name() : QLatin1String("invalid")) << ')';
}

QTextStream& operator<<(QTextStream &stream, const AbstractProperty &property)
{
    stream << "AbstractProperty(" << property.name() << ')';

    return stream;
}

} // namespace QmlDesigner

