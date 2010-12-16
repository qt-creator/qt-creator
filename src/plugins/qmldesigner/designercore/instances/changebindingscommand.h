#ifndef CHANGEBINDINGSCOMMAND_H
#define CHANGEBINDINGSCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertybindingcontainer.h"

namespace QmlDesigner {

class ChangeBindingsCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeBindingsCommand &command);

public:
    ChangeBindingsCommand();
    ChangeBindingsCommand(const QVector<PropertyBindingContainer> &bindingChangeVector);

    QVector<PropertyBindingContainer> bindingChanges() const;

private:
    QVector<PropertyBindingContainer> m_bindingChangeVector;
};

QDataStream &operator<<(QDataStream &out, const ChangeBindingsCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeBindingsCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeBindingsCommand)

#endif // CHANGEBINDINGSCOMMAND_H
