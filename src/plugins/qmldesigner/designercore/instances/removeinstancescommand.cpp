#include "removeinstancescommand.h"

namespace QmlDesigner {

RemoveInstancesCommand::RemoveInstancesCommand()
{
}

RemoveInstancesCommand::RemoveInstancesCommand(const QVector<qint32> &idVector)
    : m_instanceIdVector(idVector)
{
}

QVector<qint32> RemoveInstancesCommand::instanceIds() const
{
    return m_instanceIdVector;
}

QDataStream &operator<<(QDataStream &out, const RemoveInstancesCommand &command)
{
    out << command.instanceIds();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemoveInstancesCommand &command)
{
    in >> command.m_instanceIdVector;

    return in;
}

} // namespace QmlDesigner
