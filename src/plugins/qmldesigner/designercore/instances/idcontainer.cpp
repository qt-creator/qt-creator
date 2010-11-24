#include "idcontainer.h"

namespace QmlDesigner {

IdContainer::IdContainer()
    : m_instanceId(-1)
{
}

IdContainer::IdContainer(qint32 instanceId, const QString &id)
    : m_instanceId(instanceId),
    m_id(id)
{
}

qint32 IdContainer::instanceId() const
{
    return m_instanceId;
}

QString IdContainer::id() const
{
    return m_id;
}

QDataStream &operator<<(QDataStream &out, const IdContainer &container)
{
    out << container.instanceId();
    out << container.id();

    return out;
}

QDataStream &operator>>(QDataStream &in, IdContainer &container)
{
    in >> container.m_instanceId;
    in >> container.m_id;

    return in;
}
} // namespace QmlDesigner
