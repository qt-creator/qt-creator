#include "propertybindingcontainer.h"


namespace QmlDesigner {

PropertyBindingContainer::PropertyBindingContainer()
    : m_instanceId(-1)
{
}

PropertyBindingContainer::PropertyBindingContainer(qint32 instanceId, const QString &name, const QString &expression, const QString &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_expression(expression),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyBindingContainer::instanceId() const
{
    return m_instanceId;
}

QString PropertyBindingContainer::name() const
{
    return m_name;
}

QString PropertyBindingContainer::expression() const
{
    return m_expression;
}

bool PropertyBindingContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

QString PropertyBindingContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyBindingContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.expression();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyBindingContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_expression;
    in >> container.m_dynamicTypeName;

    return in;
}

} // namespace QmlDesigner
