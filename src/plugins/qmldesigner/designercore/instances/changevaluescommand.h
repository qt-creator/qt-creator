#ifndef CHANGEVALUESCOMMAND_H
#define CHANGEVALUESCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyvaluecontainer.h"

namespace QmlDesigner {

class ChangeValuesCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeValuesCommand &command);

public:
    ChangeValuesCommand();
    ChangeValuesCommand(const QVector<PropertyValueContainer> &valueChangeVector);

    QVector<PropertyValueContainer> valueChanges() const;

private:
    QVector<PropertyValueContainer> m_valueChangeVector;
};

QDataStream &operator<<(QDataStream &out, const ChangeValuesCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeValuesCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeValuesCommand)

#endif // CHANGEVALUESCOMMAND_H
