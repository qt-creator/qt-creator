#include "changevaluescommand.h"

namespace QmlDesigner {

ChangeValuesCommand::ChangeValuesCommand()
{
}

ChangeValuesCommand::ChangeValuesCommand(const QVector<PropertyValueContainer> &valueChangeVector)
    : m_valueChangeVector (valueChangeVector)
{
}

QVector<PropertyValueContainer> ChangeValuesCommand::valueChanges() const
{
    return m_valueChangeVector;
}

QDataStream &operator<<(QDataStream &out, const ChangeValuesCommand &command)
{
    out << command.valueChanges();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeValuesCommand &command)
{
    in >> command.m_valueChangeVector;

    return in;
}

} // namespace QmlDesigner
