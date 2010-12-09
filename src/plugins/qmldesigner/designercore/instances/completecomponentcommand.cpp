#include "completecomponentcommand.h"

namespace QmlDesigner {

CompleteComponentCommand::CompleteComponentCommand()
{
}

CompleteComponentCommand::CompleteComponentCommand(const QVector<qint32> &container)
    : m_instanceVector(container)
{
}

QVector<qint32> CompleteComponentCommand::instances() const
{
    return m_instanceVector;
}

QDataStream &operator<<(QDataStream &out, const CompleteComponentCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

} // namespace QmlDesigner
