#include "addimportcommand.h"

namespace QmlDesigner {

AddImportCommand::AddImportCommand()
{
}

AddImportCommand::AddImportCommand(const AddImportContainer &container)
    : m_importContainer(container)
{
}

AddImportContainer AddImportCommand::import() const
{
    return m_importContainer;
}


QDataStream &operator<<(QDataStream &out, const AddImportCommand &command)
{
    out << command.import();

    return out;
}

QDataStream &operator>>(QDataStream &in, AddImportCommand &command)
{
    in >> command.m_importContainer;

    return in;
}

} // namespace QmlDesigner
