#ifndef PROPERTYVALUECONTAINER_H
#define PROPERTYVALUECONTAINER_H

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

#include "commondefines.h"

namespace QmlDesigner {


class PropertyValueContainer
{
    friend QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container);

public:
    PropertyValueContainer();
    PropertyValueContainer(qint32 instanceId, const QString &name, const QVariant &value, const QString &dynamicTypeName);

    qint32 instanceId() const;
    QString name() const;
    QVariant value() const;
    bool isDynamic() const;
    QString dynamicTypeName() const;

private:
    qint32 m_instanceId;
    QString m_name;
    QVariant m_value;
    QString m_dynamicTypeName;
};

QDataStream &operator<<(QDataStream &out, const PropertyValueContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyValueContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyValueContainer);

#endif // PROPERTYVALUECONTAINER_H
