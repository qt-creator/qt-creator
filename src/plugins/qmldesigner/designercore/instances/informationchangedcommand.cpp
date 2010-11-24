#include "informationchangedcommand.h"

#include <QMetaType>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

InformationChangedCommand::InformationChangedCommand()
{
}

InformationChangedCommand::InformationChangedCommand(const QVector<InformationContainer> &informationVector)
    : m_informationVector(informationVector)
{
}

QVector<InformationContainer> InformationChangedCommand::informations() const
{
    return m_informationVector;
}

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command)
{
    out << command.informations();

    return out;
}

QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command)
{
    in >> command.m_informationVector;

    return in;
}

} // namespace QmlDesigner
