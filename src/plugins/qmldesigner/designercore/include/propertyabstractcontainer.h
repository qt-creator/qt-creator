#ifndef PROPERTYABSTRACTCONTAINER_H
#define PROPERTYABSTRACTCONTAINER_H

#include <QDataStream>
#include <qmetatype.h>
#include <QString>


namespace QmlDesigner {

class PropertyAbstractContainer;

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);

class PropertyAbstractContainer
{

    friend QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
    friend QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);
public:
    PropertyAbstractContainer();
    PropertyAbstractContainer(qint32 instanceId, const QString &name, const QString &dynamicTypeName);

    qint32 instanceId() const;
    QString name() const;
    bool isDynamic() const;
    QString dynamicTypeName() const;

private:
    qint32 m_instanceId;
    QString m_name;
    QString m_dynamicTypeName;
};


} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyAbstractContainer)
#endif // PROPERTYABSTRACTCONTAINER_H
