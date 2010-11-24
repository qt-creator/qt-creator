#include "instancecontainer.h"

namespace QmlDesigner {

InstanceContainer::InstanceContainer()
    : m_instanceId(-1), m_majorNumber(-1), m_minorNumber(-1)
{
}

InstanceContainer::InstanceContainer(qint32 instanceId, const QString &type, int majorNumber, int minorNumber, const QString &componentPath)
    : m_instanceId(instanceId), m_type(type), m_majorNumber(majorNumber), m_minorNumber(minorNumber), m_componentPath(componentPath)
{
}

qint32 InstanceContainer::instanceId() const
{
    return m_instanceId;
}

QString InstanceContainer::type() const
{
    return m_type;
}

int InstanceContainer::majorNumber() const
{
    return m_majorNumber;
}

int InstanceContainer::minorNumber() const
{
    return m_minorNumber;
}

QString InstanceContainer::componentPath() const
{
    return m_componentPath;
}

QDataStream &operator<<(QDataStream &out, const InstanceContainer &container)
{
    out << container.instanceId();
    out << container.type();
    out << container.majorNumber();
    out << container.minorNumber();
    out << container.componentPath();

    return out;
}


QDataStream &operator>>(QDataStream &in, InstanceContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_type;
    in >> container.m_majorNumber;
    in >> container.m_minorNumber;
    in >> container.m_componentPath;

    return in;
}
} // namespace QmlDesigner
