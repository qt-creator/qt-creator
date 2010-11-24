#include "reparentinstancescommand.h"

namespace QmlDesigner {

ReparentInstancesCommand::ReparentInstancesCommand()
{
}

ReparentInstancesCommand::ReparentInstancesCommand(const QVector<ReparentContainer> &container)
    : m_reparentInstanceVector(container)
{
}

QVector<ReparentContainer> ReparentInstancesCommand::reparentInstances() const
{
    return m_reparentInstanceVector;
}


QDataStream &operator<<(QDataStream &out, const ReparentInstancesCommand &command)
{
    out << command.reparentInstances();

    return out;
}

QDataStream &operator>>(QDataStream &in, ReparentInstancesCommand &command)
{
    in >> command.m_reparentInstanceVector;

    return in;
}

} // namespace QmlDesigner
