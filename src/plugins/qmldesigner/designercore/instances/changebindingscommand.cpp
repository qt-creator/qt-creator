#include "changebindingscommand.h"

namespace QmlDesigner {

ChangeBindingsCommand::ChangeBindingsCommand()
{
}

ChangeBindingsCommand::ChangeBindingsCommand(const QVector<PropertyBindingContainer> &bindingChangeVector)
    : m_bindingChangeVector (bindingChangeVector)
{
}

QVector<PropertyBindingContainer> ChangeBindingsCommand::bindingChanges() const
{
    return m_bindingChangeVector;
}

QDataStream &operator<<(QDataStream &out, const ChangeBindingsCommand &command)
{
    out << command.bindingChanges();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeBindingsCommand &command)
{
    in >> command.m_bindingChangeVector;

    return in;
}
} // namespace QmlDesigner
