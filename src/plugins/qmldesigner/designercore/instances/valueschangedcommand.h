#ifndef VALUESCHANGEDCOMMAND_H
#define VALUESCHANGEDCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ValuesChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);

public:
    ValuesChangedCommand();
    ValuesChangedCommand(const QVector<PropertyValueContainer> &valueChangeVector);

    QVector<PropertyValueContainer> valueChanges() const;

private:
    QVector<PropertyValueContainer> m_valueChangeVector;
};

QDataStream &operator<<(QDataStream &out, const ValuesChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ValuesChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ValuesChangedCommand);


#endif // VALUESCHANGEDCOMMAND_H
