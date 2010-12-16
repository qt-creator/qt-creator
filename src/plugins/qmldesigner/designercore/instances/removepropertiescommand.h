#ifndef REMOVEPROPERTIESCOMMAND_H
#define REMOVEPROPERTIESCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "propertyabstractcontainer.h"

namespace QmlDesigner {

class RemovePropertiesCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command);
public:
    RemovePropertiesCommand();
    RemovePropertiesCommand(const QVector<PropertyAbstractContainer> &properties);

    QVector<PropertyAbstractContainer> properties() const;

private:
    QVector<PropertyAbstractContainer> m_properties;
};

QDataStream &operator<<(QDataStream &out, const RemovePropertiesCommand &command);
QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command);

}

Q_DECLARE_METATYPE(QmlDesigner::RemovePropertiesCommand)

#endif // REMOVEPROPERTIESCOMMAND_H
