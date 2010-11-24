#include "valueschangedcommand.h"

namespace QmlDesigner {

ValuesChangedCommand::ValuesChangedCommand()
{
}

ValuesChangedCommand::ValuesChangedCommand(const QVector<PropertyValueContainer> &valueChangeVector)
    : m_valueChangeVector (valueChangeVector)
{
}

QVector<PropertyValueContainer> ValuesChangedCommand::valueChanges() const
{
    return m_valueChangeVector;
}

QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command)
{
    out << command.valueChanges();

    return out;
}

QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command)
{
    in >> command.m_valueChangeVector;

    return in;
}

} // namespace QmlDesigner
