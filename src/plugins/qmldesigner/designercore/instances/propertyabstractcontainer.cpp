#include "propertyabstractcontainer.h"

namespace QmlDesigner {

PropertyAbstractContainer::PropertyAbstractContainer()
    : m_instanceId(-1)
{
}

PropertyAbstractContainer::PropertyAbstractContainer(qint32 instanceId, const QString &name, const QString &dynamicTypeName)
    : m_instanceId(instanceId),
    m_name(name),
    m_dynamicTypeName(dynamicTypeName)
{
}

qint32 PropertyAbstractContainer::instanceId() const
{
    return m_instanceId;
}

QString PropertyAbstractContainer::name() const
{
    return m_name;
}

bool PropertyAbstractContainer::isDynamic() const
{
    return !m_dynamicTypeName.isEmpty();
}

QString PropertyAbstractContainer::dynamicTypeName() const
{
    return m_dynamicTypeName;
}

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container)
{
    out << container.instanceId();
    out << container.name();
    out << container.dynamicTypeName();

    return out;
}

QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_name;
    in >> container.m_dynamicTypeName;

    return in;
}

} // namespace QmlDesigner
