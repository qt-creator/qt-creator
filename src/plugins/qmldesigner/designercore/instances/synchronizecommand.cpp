#include "synchronizecommand.h"

namespace QmlDesigner {

SynchronizeCommand::SynchronizeCommand()
    : m_synchronizeId(-1)
{
}

SynchronizeCommand::SynchronizeCommand(int synchronizeId)
    : m_synchronizeId (synchronizeId)
{
}

int SynchronizeCommand::synchronizeId() const
{
    return m_synchronizeId;
}


QDataStream &operator<<(QDataStream &out, const SynchronizeCommand &command)
{
    out << command.synchronizeId();

    return out;
}

QDataStream &operator>>(QDataStream &in, SynchronizeCommand &command)
{
    in >> command.m_synchronizeId;

    return in;
}


} // namespace QmlDesigner
