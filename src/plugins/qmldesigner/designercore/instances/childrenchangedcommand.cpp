#include "childrenchangedcommand.h"

namespace QmlDesigner {

ChildrenChangedCommand::ChildrenChangedCommand()
    : m_parentInstanceId(-1)
{
}

ChildrenChangedCommand::ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &children)
    : m_parentInstanceId(parentInstanceId),
      m_childrenVector(children)
{
}

QVector<qint32> ChildrenChangedCommand::childrenInstances() const
{
    return m_childrenVector;
}

qint32 ChildrenChangedCommand::parentInstanceId() const
{
    return m_parentInstanceId;
}

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command)
{
    out << command.parentInstanceId();
    out << command.childrenInstances();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command)
{
    in >> command.m_parentInstanceId;
    in >> command.m_childrenVector;

    return in;
}

} // namespace QmlDesigner
