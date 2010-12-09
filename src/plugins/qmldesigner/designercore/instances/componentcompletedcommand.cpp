#include "componentcompletedcommand.h"

namespace QmlDesigner {

ComponentCompletedCommand::ComponentCompletedCommand()
{
}

ComponentCompletedCommand::ComponentCompletedCommand(const QVector<qint32> &container)
    : m_instanceVector(container)
{
}

QVector<qint32> ComponentCompletedCommand::instances() const
{
    return m_instanceVector;
}

QDataStream &operator<<(QDataStream &out, const ComponentCompletedCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

} // namespace QmlDesigner

