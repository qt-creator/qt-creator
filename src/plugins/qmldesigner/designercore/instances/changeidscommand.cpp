#include "changeidscommand.h"

namespace QmlDesigner {

ChangeIdsCommand::ChangeIdsCommand()
{
}

ChangeIdsCommand::ChangeIdsCommand(const QVector<IdContainer> &idVector)
    : m_idVector(idVector)
{
}

QVector<IdContainer> ChangeIdsCommand::ids() const
{
    return m_idVector;
}

QDataStream &operator<<(QDataStream &out, const ChangeIdsCommand &command)
{
    out << command.ids();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeIdsCommand &command)
{
    in >> command.m_idVector;

    return in;
}

} // namespace QmlDesigner
