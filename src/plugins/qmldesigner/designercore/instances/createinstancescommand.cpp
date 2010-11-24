#include "createinstancescommand.h"

#include <QDataStream>

namespace QmlDesigner {

CreateInstancesCommand::CreateInstancesCommand()
{
}

CreateInstancesCommand::CreateInstancesCommand(const QVector<InstanceContainer> &container)
    : m_instanceVector(container)
{
}

QVector<InstanceContainer> CreateInstancesCommand::instances() const
{
    return m_instanceVector;
}

QDataStream &operator<<(QDataStream &out, const CreateInstancesCommand &command)
{
    out << command.instances();

    return out;
}

QDataStream &operator>>(QDataStream &in, CreateInstancesCommand &command)
{
    in >> command.m_instanceVector;

    return in;
}

} // namespace QmlDesigner
