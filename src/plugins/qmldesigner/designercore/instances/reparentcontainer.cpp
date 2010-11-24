#include "reparentcontainer.h"

namespace QmlDesigner {

ReparentContainer::ReparentContainer()
    : m_instanceId(-1),
    m_oldParentInstanceId(-1),
    m_newParentInstanceId(-1)
{
}

ReparentContainer::ReparentContainer(qint32 instanceId,
                  qint32 oldParentInstanceId,
                  const QString &oldParentProperty,
                  qint32 newParentInstanceId,
                  const QString &newParentProperty)
    : m_instanceId(instanceId),
    m_oldParentInstanceId(oldParentInstanceId),
    m_oldParentProperty(oldParentProperty),
    m_newParentInstanceId(newParentInstanceId),
    m_newParentProperty(newParentProperty)
{
}

qint32 ReparentContainer::instanceId() const
{
    return m_instanceId;
}

qint32 ReparentContainer::oldParentInstanceId() const
{
    return m_oldParentInstanceId;
}

QString ReparentContainer::oldParentProperty() const
{
    return m_oldParentProperty;
}

qint32 ReparentContainer::newParentInstanceId() const
{
    return m_newParentInstanceId;
}

QString ReparentContainer::newParentProperty() const
{
    return m_newParentProperty;
}

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container)
{
    out << container.instanceId();
    out << container.oldParentInstanceId();
    out << container.oldParentProperty();
    out << container.newParentInstanceId();
    out << container.newParentProperty();

    return out;
}

QDataStream &operator>>(QDataStream &in, ReparentContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_oldParentInstanceId;
    in >> container.m_oldParentProperty;
    in >> container.m_newParentInstanceId;
    in >> container.m_newParentProperty;

    return in;
}

} // namespace QmlDesigner
