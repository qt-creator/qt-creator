#include "removepropertiescommand.h"

namespace QmlDesigner {

RemovePropertiesCommand::RemovePropertiesCommand()
{
}

RemovePropertiesCommand::RemovePropertiesCommand(const QVector<PropertyAbstractContainer> &properties)
    : m_properties(properties)
{
}

QVector<PropertyAbstractContainer> RemovePropertiesCommand::properties() const
{
    return m_properties;
}

QDataStream &operator<<(QDataStream &out, const RemovePropertiesCommand &command)
{
    out << command.properties();

    return out;
}

QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command)
{
    in >> command.m_properties;

    return in;
}

}
