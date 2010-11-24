#include "propertyvaluecontainer.h"

namespace QmlDesigner {

PropertyValueContainer::PropertyValueContainer()
    : m_instanceId(-1)
{
}

PropertyValueContainer::PropertyValueContainer(qint32 instanceId, const QString &name, const QVariant &value, const QString &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_value(value),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyValueContainer::instanceId() const
{
    return m_instanceId;
}

QString PropertyValueContainer::name() const
{
    return m_name;
}

QVariant PropertyValueContainer::value() const
{
    return m_value;
}

bool PropertyValueContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

QString PropertyValueContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.value();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_value;
    in >> container.m_dynamicTypeName;

    return in;
}

} // namespace QmlDesigner
