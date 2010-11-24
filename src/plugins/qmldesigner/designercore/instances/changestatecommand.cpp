#include "changestatecommand.h"

namespace QmlDesigner {

ChangeStateCommand::ChangeStateCommand()
    : m_stateInstanceId(-1)
{
}

ChangeStateCommand::ChangeStateCommand(qint32 stateInstanceId)
    : m_stateInstanceId(stateInstanceId)
{
}

qint32 ChangeStateCommand::stateInstanceId() const
{
    return m_stateInstanceId;
}

QDataStream &operator<<(QDataStream &out, const ChangeStateCommand &command)
{
    out << command.stateInstanceId();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeStateCommand &command)
{
    in >> command.m_stateInstanceId;

    return in;
}

} // namespace QmlDesigner
